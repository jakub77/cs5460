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

// Adds a single block to the extents of an inode.
// return -1 on error, and the number of blocks (count, not index)
// of blocks represented.
// blk_adr is set to the be address of the block that was last added
// when an error does not occur.
static inline int add_single_block_to_extent(struct inode *inode, int *blk_adr)
{
  block_t *idata = i_data(inode);
  int cur_ext;
  int ext_adr;
  int ext_cnt;
  int ext_idx;
  int blk_cnt = 0;
  int nr;
  //printk("In add single block to extent\n");

  // Determine which extent we can add a block to.
  for(ext_idx = 0; ext_idx < 10; ext_idx++)
    {
      // Get the ith extent.
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);

      //printk("looking at extent: %i, a: %i, c: %i\n", cur_ext, ext_adr, ext_cnt);
      
      // Add the number of blocks this extent represents to the total number of blocks represented.
      blk_cnt += ext_cnt;
      //printk("Updated blk_cnt to %i\n", blk_cnt);

      if(ext_cnt == 255) // If this extent cannot fit more blocks, go to next extent.
	{
	  //printk("ext_cnt was 255, go to next extent\n");
	  continue;
	}

      if(ext_idx == 9) // If this is the 10th (index 9) extent, there is no 11th to add to.
	{
	  //printk("this is extent 9, add to this extent\n");
	  goto add_blk;
	}

      // If the next extent is empty, we can try to add to this extent without breaking data.
      //printk("looking at next extent %i, a: %i, c: %i\n", idata[ext_idx+1], get_ext_pos(idata[ext_idx+1]),get_ext_cnt(idata[ext_idx+1])); 
      if(get_ext_pos(idata[ext_idx+1]) == 0)
	{
	  //printk("next extent is empty, let's add to this extent\n");
	  goto add_blk;
	}
      // If we are here, we saw an extent, but there is at least one valid extent after this we need to add to instead.
    }

  // If we get here, we have used all 10 extents, and the last extent has a size of 255, our file is max size.
  //printk("finished loop, coudln't add to any extents, return -1\n");
  return -1;

 add_blk:
  // Allocate a new block.
  nr = minix_new_block(inode);
  if (!nr)
      return -1;
  //printk("requested new block returned %i\n", nr);

  // if our extent is of size 0, we allocate this new block as the start of the extent
  if(ext_cnt == 0)
    {
      //printk("ext_cnt was 0, set THIS extent\n");
      cur_ext = set_ext(nr, 1);
      goto set_blk;
    }

  // Otherwise, we are adding to a non-empty extent, let's check if our new block is contiguous.
  if(nr == (ext_adr + ext_cnt))
    {
      // If it is, let's make this extent one larger.
      //printk("nr was found to be contigious, increment this extent\n");
      //printk("pre : %i, %i, %i\n", cur_ext, get_ext_pos(cur_ext), get_ext_cnt(cur_ext));
      cur_ext = set_ext(ext_adr, ext_cnt + 1);
      printk("post: %i, %i, %i\n", cur_ext, get_ext_pos(cur_ext), get_ext_cnt(cur_ext));
      goto set_blk;
    }

  // In this case, the block is not contigious and needs to be put into the next extent if possible.
  if(ext_idx == 9)
    return -1;
  //printk("nr was not contiguous, set NEXT extent\n");
  cur_ext = set_ext(nr, 1);
  ext_idx++;

 set_blk:
  idata[ext_idx] = cur_ext;
  *blk_adr = nr;
  mark_inode_dirty(inode);
  //printk("Final extent: %i, %i, %i\n", cur_ext, get_ext_pos(cur_ext), get_ext_cnt(cur_ext));
  return blk_cnt + 1;
}

// Adds blocks to the extents until we get to block number block.
// Returns the last block address on success, and -1 on error.
static inline int add_to_extents(int block, struct inode * inode)
{
  int blk_cnt;
  int blk_adr;
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
// returns the block #, or -1 if not found.
static inline int get_matching_block(sector_t block, struct inode * inode)
{
  //printk("In get_macthing_block, block: %i\n", (int)block);
  block_t *idata = i_data(inode);
  int i;
  int blk_cnt = 0;
  int cur_ext = 0;
  int ext_cnt = 0;
  int ext_adr = 0;
  int blk_pos = 0;
  for(i = 0; i < 10; i++)
    {
      cur_ext = idata[i];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      //printk("Loop cur_ext: %i, adr: %i, cnt: %i, blk_cnt: %i\n", cur_ext, ext_adr, ext_cnt, blk_cnt);
      if(ext_adr == 0)
	{
	  //printk("Didn't find the block, ext_cnt was 0\n");
	  return -1;
	}
      if(block < blk_cnt + ext_cnt)
	{
	  blk_pos = ext_adr + (block - blk_cnt);
	  //printk("Block should be in this extent, blk_pos is %i\n", blk_pos);
	  return blk_pos;
	}
      blk_cnt += ext_cnt;
    }
  return -1;
}

static inline void print_extents(struct inode * inode)
{
  int i;
  block_t *idata = i_data(inode);
  printk("index\tval\tadr\tcnt\n");
  for(i = 0; i < 10; i++)
    printk("E%i:\t %i\t%i\t%i\n", i, idata[i], get_ext_pos(idata[i]), get_ext_cnt(idata[i]));
}

// Request a block of data from the filesystem.
// Loop through extents till we find the right block.
// Map it into the buffer cache using map_bh.
static inline int get_block(struct inode * inode, sector_t block,
			struct buffer_head *bh, int create)
{
  int res;
  int err = 0;

  printk("Inode %li request for block %i\n", inode->i_ino, (int)block);
  //printk("In get_block, these are the extents seen:\n");
  print_extents(inode);

  res = get_matching_block(block, inode);
  printk("get_matching_block returned %i\n", res);
  
  // In this case, we got the block, map it in, and return.
  if(res != -1)
    {
      map_bh(bh, inode->i_sb, block_to_cpu(res));
      err = 0;
      printk("mapped in %i to bh going to done to ret %i\n", res, err);
      goto done;
    }

  // If create is not set, return an error, since at this point we need to create blocks.
  if(!create)
    {
      printk("create was not set, returning -EIO\n");
      err = -EIO;
      goto done;
    }

  printk("Calling add_to_extents to get to block: %i\n", (int)block);
  res = add_to_extents(block, inode);
  printk("Add_to_extents returned %i\n", res);
  if(res == -1)
    {
      printk("res was -1, so an error occured, return -EIO\n");
      err = -EIO;
      goto done;
    }
  
  map_bh(bh, inode->i_sb, block_to_cpu(res));
  err = 0;
  printk("Our final location to map this new block to is %i\n", res);
  goto done;

 done:
  //printk("Got to done in get_block, will return %i\n", err);
  printk("These are the extents as get_block is about to return\n");
  print_extents(inode);
  printk("\n\n");
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

// Resize a file by changing the number of blocks it will use,
// potentially deleting a block.
// New size in inode->isize.
static inline void truncate (struct inode * inode)
{
  int ext_idx;
  int cur_ext;
  int ext_adr;
  int ext_cnt;
  int blk_cnt = 0;
  int del_ext_idx;
  int ext_off;
  int target_size = inode->i_size;
  block_t *idata = i_data(inode);
  int junk;
  int j;

  printk("Inode %li request for truncate to size: %i\n", inode->i_ino, target_size);
  //printk("In truncate, these are the extents seen:\n");
  print_extents(inode);

  for(ext_idx = 0; ext_idx < 10; ext_idx++)
    {
      // Get the ith extent.
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);

      if(target_size == 0)
	{
	  del_ext_idx = ext_idx;
	  goto rmv_blks;
	}

      if(ext_cnt == 0)
	{
	  goto add_blks;
	}
      
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

      // If we are not yet at the right extent, goto the next entents.
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

  // Remove all blocks starting at del_ext_idx + ext_offset into that extent.
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

  // remove any remaining entire extents.
  for(ext_idx = del_ext_idx + 1; ext_idx < 10; ext_idx++)
    {
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      for(j = 0; j < ext_cnt; j++)
	  minix_free_block(inode, block_to_cpu(ext_adr + j));
      idata[ext_idx] = set_ext(0, 0);
    }

  goto edone;

 edone:
  //printk("Got to edone in truncate\n");
  printk("These are the extents as truncate is about to return\n");
  print_extents(inode);
  printk("\n\n");
  return;

 /*  struct super_block *sb = inode->i_sb; */
 /*  block_t *idata = i_data(inode); */
 /*  int offsets[DEPTH]; */
 /*  Indirect chain[DEPTH]; */
 /*  Indirect *partial; */
 /*  block_t nr = 0; */
 /*  int n; */
 /*  int first_whole; */
 /*  long iblock; */
  
 /*  iblock = (inode->i_size + sb->s_blocksize -1) >> sb->s_blocksize_bits; */
 /*  block_truncate_page(inode->i_mapping, inode->i_size, get_block); */
  
 /*  n = block_to_path(inode, iblock, offsets); */
 /*  if (!n) */
 /*    return; */
  
 /*  if (n == 1) { */
 /*    free_data(inode, idata+offsets[0], idata + DIRECT); */
 /*    first_whole = 0; */
 /*    goto do_indirects; */
 /*  } */
  
 /*  first_whole = offsets[0] + 1 - DIRECT; */
 /*  partial = find_shared(inode, n, offsets, chain, &nr); */
 /*  if (nr) { */
 /*    if (partial == chain) */
 /*      mark_inode_dirty(inode); */
 /*    else */
 /*      mark_buffer_dirty_inode(partial->bh, inode); */
 /*    free_branches(inode, &nr, &nr+1, (chain+n-1) - partial); */
 /*  } */
 /*  /\* Clear the ends of indirect blocks on the shared branch *\/ */
 /*  while (partial > chain) { */
 /*    free_branches(inode, partial->p + 1, block_end(partial->bh), */
 /* 		  (chain+n-1) - partial); */
 /*    mark_buffer_dirty_inode(partial->bh, inode); */
 /*    brelse (partial->bh); */
 /*    partial--; */
 /*  } */
 /* do_indirects: */
 /*  /\* Kill the remaining (whole) subtrees *\/ */
 /*  while (first_whole < DEPTH-1) { */
 /*    nr = idata[DIRECT+first_whole]; */
 /*    if (nr) { */
 /*      idata[DIRECT+first_whole] = 0; */
 /*      mark_inode_dirty(inode); */
 /*      free_branches(inode, &nr, &nr+1, first_whole+1); */
 /*    } */
 /*    first_whole++; */
 /*  } */
 /*  inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC; */
 /*  mark_inode_dirty(inode); */
}

static inline unsigned nblocks(loff_t size, struct super_block *sb)
{
  int k = sb->s_blocksize_bits - 10;
  unsigned blocks, res, direct = DIRECT, i = DEPTH;
  blocks = (size + sb->s_blocksize - 1) >> (BLOCK_SIZE_BITS + k);
  res = blocks;
  while (--i && blocks > direct) {
    blocks -= direct;
    blocks += sb->s_blocksize/sizeof(block_t) - 1;
    blocks /= sb->s_blocksize/sizeof(block_t);
    res += blocks;
    direct = 1;
  }
  return res;
}
