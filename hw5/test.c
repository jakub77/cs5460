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
void create_extent()
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
  i_data_global[0] = set_ext(ext_addresses[0], 1);
  i_data_global[1] = set_ext(ext_addresses[1], 2);
  i_data_global[2] = set_ext(ext_addresses[2], 3);
  i_data_global[3] = set_ext(ext_addresses[3], 0);
  i_data_global[4] = set_ext(ext_addresses[4], 0);
  i_data_global[5] = set_ext(ext_addresses[5], 0);
  i_data_global[6] = set_ext(ext_addresses[6], 0);
  i_data_global[7] = set_ext(ext_addresses[7], 0);
  i_data_global[8] = set_ext(ext_addresses[8], 0);
  i_data_global[9] = set_ext(ext_addresses[9], 0);
  used_extents = 3;
  return;
}

// Adds a new block to the extents, returns the number of blocks
// represented by the extents, or -1 if we can't.
int add_to_extents()
{
  printf("In add_to_extents\n");
  int *idata = i_data_global;
  int num_extents = 0;
  int blk_cnt = 0;
  int cur_ext = 0;
  int ext_cnt = 0;
  int ext_adr = 0;
  int blk_pos = 0;
  int i = 0;

  // Find the extent we wish to add to
  for(i = 0; i < 10; i++)
    {
      cur_ext = idata[i];
      ext_adr = get_ext_pos(cur_ext);
      ext_cnt = get_ext_cnt(cur_ext);
      printf("L1 cur_ext: %i, adr: %i, cnt: %i, blk_cnt: %i\n", cur_ext, ext_adr, ext_cnt, blk_cnt);

      blk_cnt += ext_cnt;

      if(ext_cnt == 255)
	{
	  printf("This ext was full, let's move to next\n");
	  continue;
	}

      // See if there is an extent that follows this, if it's count is 0, 
      // try to add to that extent.
      if(i < 9)
	{
	  printf("i = %i < 9\n", i);
	  if(get_ext_cnt(idata[i+1]) == 0)
	    {
	      printf("Next ext has count of 0, let's add to this ext\n");
	      goto add_to_extent;
	    }
	}
    }

  return -1;

 add_to_extent:  
  printf("ATE i %i, cur_ext %i, adr %i, cnt %i, blk_cnt %i\n", i, cur_ext, ext_adr, ext_cnt, blk_cnt);
  if(ext_cnt != 0)
    idata[i] = set_ext(ext_adr, ext_cnt+1);
  else
    idata[i] = set_ext(ext_addresses[used_extents], 1);
  printf("ATE i %i, cur_ext %i, adr %i, cnt %i, blk_cnt %i\n", i, cur_ext, ext_adr, ext_cnt, blk_cnt);

  return blk_cnt;
}

// Loop through the extents looking for the block.
// returns the block #, or -1 if not found.
int get_matching_block(int block)
{
  printf("In get_macthing_block, block: %i\n", block);
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
      if(ext_cnt == 0)
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
}

int main(int argc, char* argv[])
{
  if(argc < 2)
    return 0;
  int block = atoi(argv[1]);
  create_extent();
  int block_addr = get_matching_block(block);
  printf("get_matching_block returned %i\n", block_addr);
  int res = 0;
  if(block_addr == -1)
    {
      while(1)
	{
	  res = add_to_extents();
	  if(res == -1)
	    {
	      printf("Res was -1, abort\n");
	      exit(0);
	    }
	  if(res == block)
	    {
	      block_addr = get_matching_block(block);
	      printf("re_call g_m_b: %i\n", block_addr);
	      break;
	    }
	}
    }

  int i;
  for(i = 0; i < 10; i++)
    printf("E: %i, %i, %i\n", i_data_global[i], get_ext_pos(i_data_global[i]), get_ext_cnt(i_data_global[i]));

  printf("The final block is at: %i\n", block_addr);
  printf("Done\n");
}








