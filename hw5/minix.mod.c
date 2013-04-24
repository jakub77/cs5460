#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x68d372d2, "module_layout" },
	{ 0x7ca39e7, "__kmap_atomic" },
	{ 0xc0530fa2, "kmem_cache_destroy" },
	{ 0x3781e345, "iget_failed" },
	{ 0x794a2c29, "kmalloc_caches" },
	{ 0x12da5bb2, "__kmalloc" },
	{ 0x405c1144, "get_seconds" },
	{ 0x82819580, "mark_buffer_dirty_inode" },
	{ 0x65799580, "__bread" },
	{ 0xeb9d3d9d, "generic_file_llseek" },
	{ 0x51df5968, "__mark_inode_dirty" },
	{ 0x76ebea8, "pv_lock_ops" },
	{ 0xd0d8621b, "strlen" },
	{ 0x847f182b, "page_address" },
	{ 0x3fcac3d7, "block_write_begin" },
	{ 0x25820c64, "fs_overflowuid" },
	{ 0xc01cf848, "_raw_read_lock" },
	{ 0xdb79b576, "__lock_page" },
	{ 0xae739097, "generic_file_aio_read" },
	{ 0x94ccfb5a, "block_read_full_page" },
	{ 0xdf0f8fb1, "end_writeback" },
	{ 0xc93334e, "mount_bdev" },
	{ 0x713f9d2c, "generic_read_dir" },
	{ 0x707a7235, "generic_file_aio_write" },
	{ 0x2a9abbab, "__insert_inode_hash" },
	{ 0x2bc95bd4, "memset" },
	{ 0x50eedeb8, "printk" },
	{ 0x3443f968, "d_rehash" },
	{ 0x5152e605, "memcmp" },
	{ 0x7b59a3cc, "find_or_create_page" },
	{ 0x42064a75, "d_alloc_root" },
	{ 0x314a240b, "kunmap" },
	{ 0xa1c76e0a, "_cond_resched" },
	{ 0xb4390f9a, "mcount" },
	{ 0x1c1fd9ed, "kmem_cache_free" },
	{ 0x7f658e80, "_raw_write_lock" },
	{ 0xed93f29e, "__kunmap_atomic" },
	{ 0xe0bd2848, "setattr_copy" },
	{ 0xef716f12, "page_symlink" },
	{ 0x63f28403, "sync_dirty_buffer" },
	{ 0xff5fc144, "unlock_page" },
	{ 0xb9f7fc9a, "__brelse" },
	{ 0xf11543ff, "find_first_zero_bit" },
	{ 0x8fa01a71, "inode_init_once" },
	{ 0x604dfe96, "page_follow_link_light" },
	{ 0xcba7085a, "invalidate_inode_buffers" },
	{ 0x9a1ec4c8, "kmem_cache_alloc" },
	{ 0x738803e6, "strnlen" },
	{ 0x74117b38, "generic_file_mmap" },
	{ 0xe2fc4044, "kmap" },
	{ 0x3b056a37, "block_write_full_page" },
	{ 0x63580ed, "bdevname" },
	{ 0x8336c26c, "block_write_end" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x6babd437, "generic_write_end" },
	{ 0x23451304, "do_sync_read" },
	{ 0x33771543, "unlock_new_inode" },
	{ 0x5161968e, "kill_block_super" },
	{ 0xc5996ba3, "inode_change_ok" },
	{ 0x3f9b9190, "kmem_cache_alloc_trace" },
	{ 0x67f7403e, "_raw_spin_lock" },
	{ 0xa2e1adb, "kmem_cache_create" },
	{ 0xcdcae876, "register_filesystem" },
	{ 0x32a5af13, "iput" },
	{ 0x6af634b1, "read_cache_page" },
	{ 0x5429f762, "generic_file_fsync" },
	{ 0x37a0cba, "kfree" },
	{ 0x5386ecb5, "do_sync_write" },
	{ 0xeac8457c, "ihold" },
	{ 0x2e60bace, "memcpy" },
	{ 0x50f5e532, "call_rcu_sched" },
	{ 0x6128b5fc, "__printk_ratelimit" },
	{ 0xf7df110, "vmtruncate" },
	{ 0x48e60ad1, "block_truncate_page" },
	{ 0xed5cd2f9, "sb_set_blocksize" },
	{ 0xb62f4280, "generic_readlink" },
	{ 0x33842f27, "put_page" },
	{ 0x11abc1dc, "__bforget" },
	{ 0x74c134b9, "__sw_hweight32" },
	{ 0x419a8915, "__block_write_begin" },
	{ 0x79c4fd9b, "mark_buffer_dirty" },
	{ 0x197c45a1, "unregister_filesystem" },
	{ 0x657eb04f, "write_one_page" },
	{ 0xf6d8d826, "init_special_inode" },
	{ 0xac593e2a, "new_inode" },
	{ 0xf37c3022, "generic_file_splice_read" },
	{ 0x6da997bb, "page_put_link" },
	{ 0xc2cf201, "d_instantiate" },
	{ 0x9f8af360, "generic_block_bmap" },
	{ 0x1b66d2de, "iget_locked" },
	{ 0xf897db59, "generic_fillattr" },
	{ 0x396a9cff, "inode_init_owner" },
	{ 0xe914e41e, "strcpy" },
	{ 0xb4ba3de3, "truncate_inode_pages" },
	{ 0xdf929370, "fs_overflowgid" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "6D5F6D55DA8958416288119");
