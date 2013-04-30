#include <stdio.h>
#include <stdlib.h>

int *i_data_global;
int *ext_addresses;
int used_extents;

static inline int get_ext_pos(int x)
{
  return x & 0xFFFFFF;
  //return ((x >> 8) & 0xFFFFFF);
}

static inline int get_ext_cnt(int x)
{
  int cnt = ((x >> 24) & 0xFF) + 1;
  if(cnt == 256)
    return 0;
  return cnt;
  //return x & 0xFF;
}
static inline int set_ext(int pos, int count)
{
  count = ((count - 1) & 0xFF) << 24;
  pos = pos & 0xFFFFFF;
  return pos | count;
}

// Create a list of 10 extents.
void create_extents()
{
  ext_addresses = malloc(10*sizeof(int));
  i_data_global = malloc(10*sizeof(int));
  ext_addresses[0] = 100;
  ext_addresses[1] = 110;
  ext_addresses[2] = 120;
  ext_addresses[3] = 130;
  ext_addresses[4] = 150;
  ext_addresses[5] = 180;
  ext_addresses[6] = 250;
  ext_addresses[7] = 300;
  ext_addresses[8] = 350;
  ext_addresses[9] = 400;
  i_data_global[0] = 1279;
  i_data_global[1] = 0;
  i_data_global[2] = 0;
  i_data_global[3] = 0;
  i_data_global[4] = 0;
  i_data_global[5] = 0;
  i_data_global[6] = 0;
  i_data_global[7] = 0;
  i_data_global[8] = 0;
  i_data_global[9] = 0;
  used_extents = 0;
  return;
}

int minix_new_block()
{
  int ret;
  printf("Enter block number for minix_new_block() to use\n");
  scanf("%i", &ret);
  return ret;
}


// Adds a single block to the extents of an inode.
// return -1 on error, and the number of blocks (count, not index)
// of blocks represented.
// blk_adr is set to the be address of the block that was last added
// when an error does not occur.
static inline int add_single_block_to_extent(int *blk_adr)
{
  int *idata = i_data_global;
  int cur_ext;
  int ext_adr;
  int ext_cnt;
  int ext_idx;
  int blk_cnt = 0;
  int nr;

  // Determine which extent we can add a block to.
  for(ext_idx = 0; ext_idx < 10; ext_idx++)
    {
      // Get the ith extent.
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      
      // Add the number of blocks this extent represents to the total number of blocks represented.
      blk_cnt += ext_cnt;

      if(ext_cnt == 255) // If this extent cannot fit more blocks, go to next extent.
	  continue;
      if(ext_idx == 9) // If this is the 10th (index 9) extent, there is no 11th to add to.
	goto add_blk;
      if(get_ext_cnt(idata[ext_idx+1]) == 1 && get_ext_pos(idata[ext_idx+1]) == 0) // If the next extent is empty, we can try to add to this extent without breaking data.
	goto add_blk;
      // If we are here, we saw an extent, but there is at least one valid extent after this we need to add to instead.
    }

  // If we get here, we have used all 10 extents, and the last extent has a size of 255, our file is max size.
  return -1;

 add_blk:
  // Allocate a new block.
  nr = minix_new_block();
  if (!nr)
      return -1;

  // if our extent is of size 0, we allocate this new block as the start of the extent
  if(ext_cnt == 0)
    {
      cur_ext = set_ext(nr, 1);
      goto set_blk;
    }

  // Otherwise, we are adding to a non-empty extent, let's check if our new block is contiguous.
  if(nr == (ext_adr + ext_cnt))
    {
      // If it is, let's make this extent one larger.
      cur_ext = set_ext(ext_adr, ++ext_cnt);
      goto set_blk;
    }

  // In this case, the block is not contigious and needs to be put into the next extent if possible.
  if(ext_idx == 9)
    return -1;
  cur_ext = set_ext(nr, 1);
  ext_idx++;

 set_blk:
  i_data_global[ext_idx] = cur_ext;
  *blk_adr = nr;
  //mark_inode_dirty(inode);
  return blk_cnt + 1;
}

// Adds blocks to the extents until we get to block number block.
// Returns the last block address on success, and -1 on error.
static inline int add_to_extents(int block)
{
  int blk_cnt;
  int blk_adr;
  for(;;)
    {
      // Try to add a single block to the inode.
      blk_cnt = add_single_block_to_extent(&blk_adr);
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
static inline int get_matching_block(int block)
{
  //printk("In get_macthing_block, block: %i\n", (int)block);
  int *idata = i_data_global;
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
      printf("Loop cur_ext: %i, adr: %i, cnt: %i, blk_cnt: %i\n", cur_ext, ext_adr, ext_cnt, blk_cnt);
      if(ext_cnt == 1 && ext_adr == 0)
	{
	  printf("Didn't find the block, ext_cnt was 0\n");
	  return -1;
	}
      if(block < blk_cnt + ext_cnt)
	{
	  blk_pos = ext_adr + (block - blk_cnt);
	  printf("Block should be in this extent, blk_pos is %i\n", blk_pos);
	  return blk_pos;
	}
      blk_cnt += ext_cnt;
    }
  return -1;
}

static inline void print_extents()
{
  int i;
  int *idata = i_data_global;
  printf("index\tval\t\tadr\tcnt\n");
  for(i = 0; i < 10; i++)
    printf("E%i:\t %i\t%i\t%i\n", i, idata[i], get_ext_pos(idata[i]), get_ext_cnt(idata[i]));
}

static inline int get_block(int block)
{
  int res;
  int err = -1;

  printf("The current extents before anything are:\n");
  print_extents();

  res = get_matching_block(block);
  printf("get_matching_blocks returned %i\n", res);

  if(res != -1)
    {
      err = res;
      printf("Mappying %i to bh, returning %i\n", res, err);
      goto done;
    }
  
  printf("Calling add_to_extents to get to block %i\n", block);
  res = add_to_extents(block);
  printf("add_to_exents returned %i\n", res);
  if(res == -1)
    {
      printf("res of -1 means error, return it\n");
      err = -1;
      goto done;
    }

  err = res;
  printf("Our final locaiton to map is %i\n", res);

 done:
  printf("At done, will return %i\n", err);
  printf("Final extents as quitting are:\n");
  print_extents();
  return err;

}

static inline void truncate (int target_size)
{
  int ext_idx;
  int cur_ext;
  int ext_adr;
  int ext_cnt;
  int blk_cnt = 0;
  int del_ext_idx;
  int ext_off;
  // int target_size = inode->i_size;
  int *idata = i_data_global;
  int junk;
  int j;

  printf("Inode request for truncate to size: %i\n", target_size);
  printf("In truncate, these are the extents seen:\n");
  print_extents();

  for(ext_idx = 0; ext_idx < 10; ext_idx++)
    {
      // Get the ith extent.
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);

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
      blk_cnt = add_single_block_to_extent(&junk);
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
      //minix_free_block(inode, block_to_cpu(ext_adr + j));
      blk_cnt--;
    }
  if(ext_off == 0)
    ext_adr = 0;
  idata[del_ext_idx] = set_ext(ext_adr, ext_off); ///////////////////////////////////////////////////

  // remove any remaining entire extents.
  for(ext_idx = del_ext_idx + 1; ext_idx < 10; ext_idx++)
    {
      cur_ext = idata[ext_idx];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      for(j = 0; j < ext_cnt; j++)
	{
	  //minix_free_block(inode, block_to_cpu(ext_adr + j));
	}
      idata[ext_idx] = set_ext(0, 0);
    }

  goto edone;

 edone:
  printf("Got to edone in truncate");
  printf("These are the extents as truncate is about to return\n");
  print_extents();
  printf("\n\n");
  return;
}

int main(int argc, char* argv[])
{
  char buffer[100];
  int mode;
  int num;
  int res;
  create_extents();
  for(;;)
    {
      printf("\n---------------------------\n");
      printf("Enter input:\n");
      scanf("%s", buffer);
      if(buffer[0] == 'g')
	mode = 0;
      else if(buffer[0] == 't')
	mode = 1;
      else
	{
	  printf("You entered something invalid\n");
	  return 0;
	}
      num = atoi(&buffer[1]);
      if(mode == 0)
	{
	  res = get_block(num);
	  printf("Res from get_block: %i\n", res);
	}
      else if(mode == 1)
	{
	  truncate(num);
	}
    }
}








