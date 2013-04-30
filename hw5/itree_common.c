// Jakub Szpunar CS5460 HW5 Extent modified minix.
// This is the only file where code was added. However itree_v1, and itree_v2 had
// unused code removed.
//
// Modified functions:
// get_block(), truncate(), nblock()
//
// Added functions:
// get_ext_pos(), get_ext_cnt(), set_ext(), add_single_block_to_extent(), 
// add_to_extents(), get_matching_block(), 

// Get the block address out of an int represeting an extent.
static inline int get_ext_pos(int x)
{
  return x & 0xFFFFFF;
}

// Get the count out of an int representing an extent, special
// case to make root node work by offsetting count by one when
// stored. returns 0 if the block address of the extent is not set.
static inline int get_ext_cnt(int x)
{
  int cnt;
  if(get_ext_pos(x) == 0)
    return 0;
  cnt = ((x >> 24) & 0xFF);
  if(cnt == 255)
    cnt = -1;
  cnt++;
  return cnt;
}

// Give a block address and count to receive a packaged
// int that represents an extent. 
static inline int set_ext(int pos, int count)
{
  count--;
  if(count == -1)
    count = 255;
  count = (count & 0xFF) << 24;
  pos = pos & 0xFFFFFF;
  return pos | count;
}

/* Generic part */

typedef struct {
	block_t	*p;
	block_t	key;
	struct buffer_head *bh;
} Indirect;

static DEFINE_RWLOCK(pointers_lock);

static inline void add_chain(Indirect *p, struct buffer_head *bh, block_t *v)
{
	p->key = *(p->p = v);
	p->bh = bh;
}

static inline int verify_chain(Indirect *from, Indirect *to)
{
	while (from <= to && from->key == *from->p)
		from++;
	return (from > to);
}

static inline block_t *block_end(struct buffer_head *bh)
{
	return (block_t *)((char*)bh->b_data + bh->b_size);
}

static inline Indirect *get_branch(struct inode *inode,
					int depth,
					int *offsets,
					Indirect chain[DEPTH],
					int *err)
{
	struct super_block *sb = inode->i_sb;
	Indirect *p = chain;
	struct buffer_head *bh;

	*err = 0;
	/* i_data is not going away, no lock needed */
	add_chain (chain, NULL, i_data(inode) + *offsets);
	if (!p->key)
		goto no_block;
	while (--depth) {
		bh = sb_bread(sb, block_to_cpu(p->key));
		if (!bh)
			goto failure;
		read_lock(&pointers_lock);
		if (!verify_chain(chain, p))
			goto changed;
		add_chain(++p, bh, (block_t *)bh->b_data + *++offsets);
		read_unlock(&pointers_lock);
		if (!p->key)
			goto no_block;
	}
	return NULL;

changed:
	read_unlock(&pointers_lock);
	brelse(bh);
	*err = -EAGAIN;
	goto no_block;
failure:
	*err = -EIO;
no_block:
	return p;
}

static inline int splice_branch(struct inode *inode,
				     Indirect chain[DEPTH],
				     Indirect *where,
				     int num)
{
	int i;

	write_lock(&pointers_lock);

	/* Verify that place we are splicing to is still there and vacant */
	if (!verify_chain(chain, where-1) || *where->p)
		goto changed;

	*where->p = where->key;

	write_unlock(&pointers_lock);

	/* We are done with atomic stuff, now do the rest of housekeeping */

	inode->i_ctime = CURRENT_TIME_SEC;

	/* had we spliced it onto indirect block? */
	if (where->bh)
		mark_buffer_dirty_inode(where->bh, inode);

	mark_inode_dirty(inode);
	return 0;

changed:
	write_unlock(&pointers_lock);
	for (i = 1; i < num; i++)
		bforget(where[i].bh);
	for (i = 0; i < num; i++)
		minix_free_block(inode, block_to_cpu(where[i].key));
	return -EAGAIN;
}

// Add a block to the extents of an inode. Returns -1 on error,
// or the total number of blocks in the file otherwise.
// *blk_adr is set to the be address of the block that was last added
// when an error does not occur.
static inline int add_single_block_to_extent(struct inode *inode, int *blk_adr)
{
  block_t *idata = i_data(inode);
  int cur_ext, ext_adr, ext_cnt, ext_idx, nr, blk_cnt = 0;
 
  // Loop through up to 10 extents to find where to add a block to.
  for(ext_idx = 0; ext_idx < 10; ext_idx++)
    {
      // Get data for extent at this index.
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      
      // Update block count.
      blk_cnt += ext_cnt;

      // If this extent is full, move to the next extent.
      if(ext_cnt == 255)
	  continue;

      // If this is the last extent, this is the only extent left to add to, do so.
      if(ext_idx == 9)
	  goto add_blk;

      // If the next extent is empty, we can safely add to this extent.
      //if(get_ext_pos(idata[ext_idx+1]) == 0)
      if(get_ext_cnt(idata[ext_idx+1]) == 0)
	goto add_blk;
    }

  // If we get here, we looped through all extents and couln't valid room for another block.
  return -1;

 add_blk:
  // Request a new block.
  nr = minix_new_block(inode);
  if (!nr)
      return -1;

  // If the extent we are trying to is of size 0, set it to point to this block.
  if(ext_cnt == 0)
    {
      cur_ext = set_ext(nr, 1);
      goto set_blk;
    }

  // Otherwise, check to see if new block is contiguous with this extent.
  if(nr == (ext_adr + ext_cnt))
    {
      cur_ext = set_ext(ext_adr, ext_cnt + 1);
      goto set_blk;
    }

  // Finally, if here, not contigious, check to see if we have another extent to 
  // add to after this one, if so, add this block to it.
  if(ext_idx == 9)
    return -1;
  cur_ext = set_ext(nr, 1);
  ext_idx++;

 set_blk:
  // Update data structures, set block address, and mark dirty.
  idata[ext_idx] = cur_ext;
  *blk_adr = nr;
  mark_inode_dirty(inode);
  return blk_cnt + 1;
}

// Adds blocks to the extents until we get to block number block.
// Returns the last block address on success, and -1 on error.
static inline int add_to_extents(int block, struct inode * inode)
{
  int blk_cnt, blk_adr;
  for(;;)
    {
      // Try to add a single block to the inode.
      blk_cnt = add_single_block_to_extent(inode, &blk_adr);
      // If adding a block failed, return failure.
      if(blk_cnt == -1)
	return -1;
      // Check to see if we added enough blocks, if so, return the address of this block.
      if(blk_cnt == block + 1)
	return blk_adr;
    }
}

// Loop through the extents looking for the block.
// returns the block # if found, -1 if not.
static inline int get_matching_block(sector_t block, struct inode * inode)
{
  block_t *idata = i_data(inode);
  int i, blk_cnt = 0, cur_ext, ext_cnt, ext_adr, blk_pos;
  // Loop through up to all the extents.
  for(i = 0; i < 10; i++)
    {
      // Get info from this extent.
      cur_ext = idata[i];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);

      // If the extent is empty, return not found.
      //if(ext_adr == 0)
      if(ext_cnt == 0)
	  return -1;
      // If the block # if inside this extent, extract it and return.
      if(block < blk_cnt + ext_cnt)
	{
	  blk_pos = ext_adr + (block - blk_cnt);
	  return blk_pos;
	}
      // Otherwise, update blk_cnt and move to next extent.
      blk_cnt += ext_cnt;
    }
  return -1;
}

// Print out all the extents with printk.
static inline void print_extents(struct inode * inode)
{
  int i;
  block_t *idata = i_data(inode);
  printk("index\tval\tadr\tcnt\n");
  for(i = 0; i < 10; i++)
    printk("E%i:\t %i\t%i\t%i\n", i, idata[i], get_ext_pos(idata[i]), get_ext_cnt(idata[i]));
}

// Request a block from the filesystem. If we have it, we map it into the
// buffer cache. If we do not (and create is set), we create it.
// Returns an error code if an error occurs.
static inline int get_block(struct inode * inode, sector_t block,
			struct buffer_head *bh, int create)
{
  int res, err = 0;

  // See if we have the block requested.
  res = get_matching_block(block, inode);
  
  // If we did, map it in to the buffer cache and get ready to quit.
  if(res != -1)
    {
      map_bh(bh, inode->i_sb, block_to_cpu(res));
      err = 0;
      goto done;
    }

  // If we didn't have it, and create isn't set, nothing we can do...
  if(!create)
    {
      err = -EIO;
      goto done;
    }

  // Since create is set, let's try to add enough blocks.
  res = add_to_extents(block, inode);
  // If we got an error, we'll get ready to return it.
  if(res == -1)
    {
      err = -EIO;
      goto done;
    }
  
  // Here we know add_to_extents succeded, so let's map to buffer cache.
  map_bh(bh, inode->i_sb, block_to_cpu(res));
  err = 0;

  // Simply return the error, or lack thereof.
 done:
  return err;
}

static inline int all_zeroes(block_t *p, block_t *q)
{
	while (p < q)
		if (*p++)
			return 0;
	return 1;
}

static inline void free_data(struct inode *inode, block_t *p, block_t *q)
{
	unsigned long nr;

	for ( ; p < q ; p++) {
		nr = block_to_cpu(*p);
		if (nr) {
			*p = 0;
			minix_free_block(inode, nr);
		}
	}
}

// Resize a file to the size provided in inode->size.
// Will either make the file smaller, enlarge the file, or leave it be if
// it already is that size.
static inline void truncate (struct inode * inode)
{
  int ext_idx, cur_ext, ext_adr, ext_cnt, blk_cnt = 0, del_ext_idx, ext_off, junk, j, target_size = inode->i_size;
  block_t *idata = i_data(inode);

  // Loop through the extents.
  for(ext_idx = 0; ext_idx < 10; ext_idx++)
    {
      // Get the ith extent.
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      
      // If we are wanting a size of 0, just to go removing blocks.
      if(target_size == 0)
	{
	  del_ext_idx = ext_idx;
	  goto rmv_blks;
	}

      // If the count of blocks in here is zero, goto add blocks to get to the size > 0.
      if(ext_cnt == 0)
	  goto add_blks;
      
      // If keeping this extent and discarding everything after is what we need, do so.
      if(blk_cnt + ext_cnt == target_size)
	{
	  del_ext_idx = ext_idx + 1;
	  ext_off = 0;
	  blk_cnt += ext_cnt;
	  goto rmv_blks;
	}

      // If we need to delete stuff half way through this extent, set up to do so.
      if(blk_cnt + ext_cnt > target_size)
	{
	  del_ext_idx = ext_idx;
	  ext_off = target_size - blk_cnt;
	  blk_cnt += ext_cnt;
	  goto rmv_blks;
	}

      // If we are not yet at the right extent, loop to next extents.
      blk_cnt += ext_cnt;
    }

  // If we fall through, we didn't get to the extent count we wanted, let's add blocks until we do.
 add_blks:
  for(;;)
    {
      // Try to add a single block to the inode.
      blk_cnt = add_single_block_to_extent(inode, &junk);
      // If adding a block failed, return failure.
      if(blk_cnt == -1)
	goto edone;
      // Check to see if we added enough blocks, if so, return the address of this block.
      if(blk_cnt == target_size)
	goto edone;
    }  

  // First remove blocks from the first extent that potentially is not all to be removed.
 rmv_blks:
  // Remove the first potentially offset extent.
  cur_ext = idata[del_ext_idx];
  ext_adr = get_ext_pos(cur_ext);
  ext_cnt = get_ext_cnt(cur_ext);
  for(j = ext_off; blk_cnt > target_size; j++)
    {
      minix_free_block(inode, block_to_cpu(ext_adr + j));
      blk_cnt--;
    }
  if(ext_off == 0)
    ext_adr = 0;
  idata[del_ext_idx] = set_ext(ext_adr, ext_off);

  // Now clear out all the rest of the extents.
  for(ext_idx = del_ext_idx + 1; ext_idx < 10; ext_idx++)
    {
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      if(ext_cnt > 0)
	{
	  for(j = 0; j < ext_cnt; j++)
	    minix_free_block(inode, block_to_cpu(ext_adr + j));
	  idata[ext_idx] = set_ext(0, 0);
	}
    }

  goto edone;

 edone:
  return;
}

// Return the number of blocks it takes to represent a file of
// the given size.
static inline unsigned nblocks(loff_t size, struct super_block *sb)
{
  int res = 0;
  res = (int)size / sb->s_blocksize;
  if((int)size % sb->s_blocksize > 0)
    res++;
  return res;
}
