static inline int get_ext_pos(int x)
{
  return ((x >> 8) & 0xFFFFFF);
}

static inline int get_ext_cnt(int x)
{
  return x & 0xFF;
}
static inline int set_ext(int pos, int count)
{
  count = count & 0xFF;
  pos = (pos & 0xFFFFFF) << 8;
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

static int alloc_branch(struct inode *inode,
			     int num,
			     int *offsets,
			     Indirect *branch)
{
	int n = 0;
	int i;
	int parent = minix_new_block(inode);

	branch[0].key = cpu_to_block(parent);
	if (parent) for (n = 1; n < num; n++) {
		struct buffer_head *bh;
		/* Allocate the next block */
		int nr = minix_new_block(inode);
		if (!nr)
			break;
		branch[n].key = cpu_to_block(nr);
		bh = sb_getblk(inode->i_sb, parent);
		lock_buffer(bh);
		memset(bh->b_data, 0, bh->b_size);
		branch[n].bh = bh;
		branch[n].p = (block_t*) bh->b_data + offsets[n];
		*branch[n].p = branch[n].key;
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
		mark_buffer_dirty_inode(bh, inode);
		parent = nr;
	}
	if (n == num)
		return 0;

	/* Allocation failed, free what we already allocated */
	for (i = 1; i < n; i++)
		bforget(branch[i].bh);
	for (i = 0; i < n; i++)
		minix_free_block(inode, block_to_cpu(branch[i].key));
	return -ENOSPC;
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

// Adds a new block to the extents, returns the number of blocks
// represented by the extents, or -1 if we can't.
static inline int add_to_extents(struct inode * inode)
{

  block_t *idata;
  //int num_extents = 0;
  int blk_cnt = 0;
  int cur_ext = 0;
  int ext_cnt = 0;
  int ext_adr = 0;
  //int blk_pos = 0;
  int i = 0;
  int nr;

  idata = i_data(inode);

  printk("In add_to_extents\n");

  // Find the extent we wish to add to
  for(i = 0; i < 10; i++)
    {
      cur_ext = idata[i];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      printk("L1 cur_ext: %i, adr: %i, cnt: %i, blk_cnt: %i\n", cur_ext, ext_adr, ext_cnt, blk_cnt);

      blk_cnt += ext_cnt;

      if(ext_cnt == 255)
	{
	  printk("This ext was full, let's move to next\n");
	  continue;
	}

      // See if there is an extent that follows this, if it's count is 0, 
      // try to add to this extent.
      if(i < 9)
	{
	  printk("i = %i < 9\n", i);
	  if(get_ext_cnt(idata[i+1]) == 0)
	    {
	      printk("Next ext has count of 0, let's add to this ext\n");
	      goto add_to_extent;
	    }
	}
    }

  return -1;

  //return (block_t *)minix_i(inode)->u.i2_data;


 add_to_extent:  
  printk("ATE i %i, cur_ext %i, adr %i, cnt %i, blk_cnt %i\n", i, cur_ext, ext_adr, ext_cnt, blk_cnt);
  nr = minix_new_block(inode);
  printk("NR returned as %i\n", nr);
  if (!nr)
    {
      printk("!nr == 0 ! Returning -1.\n");
      return -1;
    }
  
  if(nr == (ext_adr + ext_cnt))
    {
      printk("new block is contiguous, add to it\n");
      cur_ext = set_ext(ext_adr, ++ext_cnt);
      minix_i(inode)->u.i2_data[i] = cur_ext;
      return blk_cnt;
    }
  
  printk("new block is not contiguous, create new extent\n");
  cur_ext = set_ext(nr, 1);
  minix_i(inode)->u.i2_data[i+1] = cur_ext;
  return blk_cnt;
}

// Loop through the extents looking for the block.
// returns the block #, or -1 if not found.
static inline int get_matching_block(sector_t block, struct inode * inode)
{
  printk("In get_macthing_block, block: %i\n", (int)block);
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
      printk("Loop cur_ext: %i, adr: %i, cnt: %i, blk_cnt: %i\n", cur_ext, ext_adr, ext_cnt, blk_cnt);
      if(ext_cnt == 0)
	{
	  printk("Didn't find the block, ext_cnt was 0\n");
	  return -1;
	}
      if(block < blk_cnt + ext_cnt)
	{
	  blk_pos = ext_adr + (block - blk_cnt);
	  printk("Block should be in this extent, blk_pos is %i\n", blk_pos);
	  return blk_pos;
	}
      blk_cnt += ext_cnt;
    }
  printk("Should not have returned to end of get_matching_block, ret -1\n");
  return -1;
}

// Request a block of data from the filesystem.
// Loop through extents till we find the right block.
// Map it into the buffer cache using map_bh.

// inode:       The inode that represents this file.
// block:       which block to return in the inode, each block is 1024B
// buffer_head: used by map_bh to get to the buffer cache.
// create:      whether to create the block if it doesn't exist. 
static inline int get_block(struct inode * inode, sector_t block,
			struct buffer_head *bh, int create)
{
  int res;
  int err = 0;
  printk("Calling get_matching_block with a block of %i\n", (int)block);
  res = get_matching_block(block, inode);
  printk("get_macthing_block returned %i\n", res);
  
  // In this case, we got the block, map it in, and return.
  if(res != -1)
    {
      map_bh(bh, inode->i_sb, block_to_cpu(res));
      mark_inode_dirty(inode);
      printk("mapped in %i to bh, marked as dirty, returning %i\n", res, err);
      return err;
    }

  // If create is not set, return an error, since at this point we need to create blocks.
  if(!create)
    {
      printk("create was not set, returning -EIO\n");
      return -EIO;
    }

  // keep adding blocks until we either get what we wanted, or return an error.
  printk("Start to loop for add_to_extents() etc\n");
  for(;;)
    {
      res = add_to_extents(inode);
      printk("Add_to_extents() returned %i\n", res);
      if(res == -1)
	{
	  printk("res was -1, so an error occured, return -EIO\n");
	  return -EIO;
	}
      if(res == block)
	{
	  printk("res: %i, was equal to block: %i, now let's get the addr, and map/ret\n", res, (int)block);
	  res = get_matching_block(block, inode);
	  printk("Call to get_macthing_block(%i) returned %i\n", (int)block, res);
	  if(res == -1)
	    {
	      printk("Res was -1, return an error\n");
	      return -EIO;
	    }
	  map_bh(bh, inode->i_sb, block_to_cpu(res));
	  mark_inode_dirty(inode);
	  printk("mapped in %i to bh, marked as dirty, returning %i\n", res, err);
	  return err;
	}
    }

  printk("Somehow got past last for(;;) loop, error\n");
  return -EIO;
}

static inline int all_zeroes(block_t *p, block_t *q)
{
	while (p < q)
		if (*p++)
			return 0;
	return 1;
}

static Indirect *find_shared(struct inode *inode,
				int depth,
				int offsets[DEPTH],
				Indirect chain[DEPTH],
				block_t *top)
{
	Indirect *partial, *p;
	int k, err;

	*top = 0;
	for (k = depth; k > 1 && !offsets[k-1]; k--)
		;
	partial = get_branch(inode, k, offsets, chain, &err);

	write_lock(&pointers_lock);
	if (!partial)
		partial = chain + k-1;
	if (!partial->key && *partial->p) {
		write_unlock(&pointers_lock);
		goto no_top;
	}
	for (p=partial;p>chain && all_zeroes((block_t*)p->bh->b_data,p->p);p--)
		;
	if (p == chain + k - 1 && p > chain) {
		p->p--;
	} else {
		*top = *p->p;
		*p->p = 0;
	}
	write_unlock(&pointers_lock);

	while(partial > p)
	{
		brelse(partial->bh);
		partial--;
	}
no_top:
	return partial;
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

static void free_branches(struct inode *inode, block_t *p, block_t *q, int depth)
{
	struct buffer_head * bh;
	unsigned long nr;

	if (depth--) {
		for ( ; p < q ; p++) {
			nr = block_to_cpu(*p);
			if (!nr)
				continue;
			*p = 0;
			bh = sb_bread(inode->i_sb, nr);
			if (!bh)
				continue;
			free_branches(inode, (block_t*)bh->b_data,
				      block_end(bh), depth);
			bforget(bh);
			minix_free_block(inode, nr);
			mark_inode_dirty(inode);
		}
	} else
		free_data(inode, p, q);
}

// Resize a file by changing the number of blocks it will use,
// potentially deleting a block.
// New size in inode->isize.
static inline void truncate (struct inode * inode)
{
  struct super_block *sb = inode->i_sb;
  block_t *idata = i_data(inode);
  int offsets[DEPTH];
  Indirect chain[DEPTH];
  Indirect *partial;
  block_t nr = 0;
  int n;
  int first_whole;
  long iblock;
  
  iblock = (inode->i_size + sb->s_blocksize -1) >> sb->s_blocksize_bits;
  block_truncate_page(inode->i_mapping, inode->i_size, get_block);
  
  n = block_to_path(inode, iblock, offsets);
  if (!n)
    return;
  
  if (n == 1) {
    free_data(inode, idata+offsets[0], idata + DIRECT);
    first_whole = 0;
    goto do_indirects;
  }
  
  first_whole = offsets[0] + 1 - DIRECT;
  partial = find_shared(inode, n, offsets, chain, &nr);
  if (nr) {
    if (partial == chain)
      mark_inode_dirty(inode);
    else
      mark_buffer_dirty_inode(partial->bh, inode);
    free_branches(inode, &nr, &nr+1, (chain+n-1) - partial);
  }
  /* Clear the ends of indirect blocks on the shared branch */
  while (partial > chain) {
    free_branches(inode, partial->p + 1, block_end(partial->bh),
		  (chain+n-1) - partial);
    mark_buffer_dirty_inode(partial->bh, inode);
    brelse (partial->bh);
    partial--;
  }
 do_indirects:
  /* Kill the remaining (whole) subtrees */
  while (first_whole < DEPTH-1) {
    nr = idata[DIRECT+first_whole];
    if (nr) {
      idata[DIRECT+first_whole] = 0;
      mark_inode_dirty(inode);
      free_branches(inode, &nr, &nr+1, first_whole+1);
    }
    first_whole++;
  }
  inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
  mark_inode_dirty(inode);
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
