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
  printk("________In get_block__________ block: %i, create %i\n", (int)block, create);
  
  int err = -EIO;

  // Get the zones, know we have up to 10 extents in all.
  block_t *idata = i_data(inode);
  block_t cur_ext;
  int ext_idx = 0;
  int ext_cnt = 0;
  int ext_pos = 0;
  int blk_cnt = 0;
  block_t blk_pos = 0;
  

  while(1)
    {
      // If we ran out of extents, we are done.
      if(ext_idx == 10)
	{
	  printk("Ran out of extents.\n");
	goto done;
	}
      
      // Get the extent at this index.
      cur_ext = idata[ext_idx];
      printk("cur_ext at index %i: %i\n", ext_idx, cur_ext);
      
      // Get this extent's position, and the number
      // of blocks associated with this extent.
      ext_pos = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      printk("Ext pos: %i, count: %i\n", ext_pos, ext_cnt);

      // If this extent holds no blocks, we won't find our block.
      if(ext_cnt == 0)
	{
	  printk("Ext holds no blocks, let's goto add blocks.\n");
	  goto add_blocks;
	}
      
      if(block < blk_cnt + ext_cnt)
	{
	  // In this case, the block is found in this extent.
	  // Compute the blocks address based on extent's address + offset.
	  blk_pos = ext_pos + (block - blk_cnt);
	  printk("Block should lie in this extent, determined blk_pos to be %i\n", blk_pos);
	  // Map the block into the buffer cache and goto done w/o error.
	  map_bh(bh, inode->i_sb, block_to_cpu(blk_pos));
	  err = 0;
	  goto done;
	}
      
      // Add the number of blocks from this extent to our running
      // count of which blocks we have seen.
      blk_cnt += ext_cnt;
      ext_idx++;
      printk("First loop done, index: %i, blk_cnt: %i\n", blk_cnt, ext_idx);
    }
  
 add_blocks:
  printk("______Now at add blocks______\n");
  // Determine which extent to add to, see if we can add to the previous one.
  // Determine whether this block was empty.
  if(ext_cnt == 0)
    {
      printk("Determined ext_cnt to be zero\n");
      // If it was, check to see if there is a previous block to add to.
      if(ext_idx > 0)
	{
	  printk("Determined ext_idx to be > 0: %i\n", ext_idx);
	  // Check to see if this previous block has room.
	  if(get_ext_cnt(idata[ext_idx - 1]) < 255)
	    {
	      printk("Determined cnt to be < 255: %i\n",get_ext_cnt(idata[ext_idx - 1]));
	      // If it did have room, change our extent to add to it.
	      ext_idx--;
	      cur_ext = idata[ext_idx];
	      ext_pos = get_ext_pos(cur_ext);
	      ext_cnt = get_ext_cnt(cur_ext);
	      printk("New ext idx: %i, pos: %i, cnt: %i\n", ext_idx, ext_pos, ext_cnt);
	    }
	}
    }

  // We have the extent to add to, now add to it.
  int nr;
  int count = 0;
  
  while(1)
    {
      printk("Checking to break: %i, %i\n", blk_cnt, (int)block);
      if(blk_cnt == block + 1)
	{
	  printk("blk_cnt == block, %i == %i, break\n", blk_cnt, (int)block);
	  break;
	}

      if(count > 20)
	{
	  printk("Count was > 20\n");
	  goto done;
	}
      count++;
      // Allocate a new block, check for errors.
      nr = minix_new_block(inode);
      printk("New nr: %i\n", nr);
      if (!nr)
	{
	  printk("!nr == 0 !\n");
	  err = -EIO;
	  goto done;
	}
      if(nr == (ext_pos + ext_cnt))
	{
	  printk("We have a contigious block pos: %i, cnt: %i\n", ext_pos, ext_cnt);
	  // We have a contiguous block.
	  cur_ext = set_ext(ext_pos, ++ext_cnt);
	  blk_cnt++;
	  continue;
	}
      // Check to see if we can allocate a new extent.
      if(ext_idx == 9)
	{
	  printk("Whoops, we are out of extents!\n");
	  // We can't create a new extent, file too large.
	  err = -EIO;
	  goto done;
	}
      ext_idx++;
      cur_ext = set_ext(nr, 1);
      ext_pos = nr;
      ext_cnt = 1;
      blk_cnt++;
      idata[ext_idx] = cur_ext;
      printk("Created new extent: pos %i, cnt %i, index %i, blk_cnt %i\n", ext_pos, ext_cnt, ext_idx, blk_cnt);
    }

  blk_pos = ext_pos + (block - (blk_cnt - 1));
  printk("Out of loop, mapping %i into bc\n", blk_pos);
  map_bh(bh, inode->i_sb, block_to_cpu(blk_pos));
  
 done:
  printk("Arrived at done\n");
  return err;
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
  return;


/* 	struct super_block *sb = inode->i_sb; */
/* 	block_t *idata = i_data(inode); */
/* 	int offsets[DEPTH]; */
/* 	Indirect chain[DEPTH]; */
/* 	Indirect *partial; */
/* 	block_t nr = 0; */
/* 	int n; */
/* 	int first_whole; */
/* 	long iblock; */

/* 	iblock = (inode->i_size + sb->s_blocksize -1) >> sb->s_blocksize_bits; */
/* 	block_truncate_page(inode->i_mapping, inode->i_size, get_block); */

/* 	n = block_to_path(inode, iblock, offsets); */
/* 	if (!n) */
/* 		return; */

/* 	if (n == 1) { */
/* 		free_data(inode, idata+offsets[0], idata + DIRECT); */
/* 		first_whole = 0; */
/* 		goto do_indirects; */
/* 	} */

/* 	first_whole = offsets[0] + 1 - DIRECT; */
/* 	partial = find_shared(inode, n, offsets, chain, &nr); */
/* 	if (nr) { */
/* 		if (partial == chain) */
/* 			mark_inode_dirty(inode); */
/* 		else */
/* 			mark_buffer_dirty_inode(partial->bh, inode); */
/* 		free_branches(inode, &nr, &nr+1, (chain+n-1) - partial); */
/* 	} */
/* 	/\* Clear the ends of indirect blocks on the shared branch *\/ */
/* 	while (partial > chain) { */
/* 		free_branches(inode, partial->p + 1, block_end(partial->bh), */
/* 				(chain+n-1) - partial); */
/* 		mark_buffer_dirty_inode(partial->bh, inode); */
/* 		brelse (partial->bh); */
/* 		partial--; */
/* 	} */
/* do_indirects: */
/* 	/\* Kill the remaining (whole) subtrees *\/ */
/* 	while (first_whole < DEPTH-1) { */
/* 		nr = idata[DIRECT+first_whole]; */
/* 		if (nr) { */
/* 			idata[DIRECT+first_whole] = 0; */
/* 			mark_inode_dirty(inode); */
/* 			free_branches(inode, &nr, &nr+1, first_whole+1); */
/* 		} */
/* 		first_whole++; */
/* 	} */
/* 	inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC; */
/* 	mark_inode_dirty(inode); */
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
