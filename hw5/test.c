#include <stdio.h>
#include <stdlib.h>

int *i_data_global;
int *ext_addresses;
int used_extents;

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
  i_data_global[0] = set_ext(0, 0);
  i_data_global[1] = set_ext(0, 0);
  i_data_global[2] = set_ext(0, 0);
  i_data_global[3] = set_ext(0, 0);
  i_data_global[4] = set_ext(0, 0);
  i_data_global[5] = set_ext(0, 0);
  i_data_global[6] = set_ext(0, 0);
  i_data_global[7] = set_ext(0, 0);
  i_data_global[8] = set_ext(0, 0);
  i_data_global[9] = set_ext(0, 0);
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
      if(get_ext_cnt(idata[ext_idx+1]) == 0) // If the next extent is empty, we can try to add to this extent without breaking data.
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
      //printk("Loop cur_ext: %i, adr: %i, cnt: %i, blk_cnt: %i\n", cur_ext, ext_adr, ext_cnt, blk_cnt);
      if(ext_cnt == 0)
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
  //printk("Should not have returned to end of get_matching_block, ret -1\n");
  return -1;
}

static inline void print_extents()
{
  int i;
  int *idata = i_data_global;
  printf("index\tval\tadr\tcnt\n");
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

int main(int argc, char* argv[])
{
  int block;
  int res;
  create_extents();
  for(;;)
    {
      printf("\n---------------------------\n");
      printf("Enter block you are looking for:\n");
      scanf("%i", &block);
      if(block < 0)
	exit(0);
      res = get_block(block);
      printf("Res from get_block: %i\n", res);
    }
}








