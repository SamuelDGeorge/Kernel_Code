#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
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
	{ 0x7dbe36ee, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x55db6fda, __VMLINUX_SYMBOL_STR(debugfs_remove) },
	{ 0x53b151c6, __VMLINUX_SYMBOL_STR(debugfs_create_file) },
	{ 0xccd2be1e, __VMLINUX_SYMBOL_STR(debugfs_create_dir) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0xd0d8621b, __VMLINUX_SYMBOL_STR(strlen) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x50eedeb8, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xcf86fd2e, __VMLINUX_SYMBOL_STR(get_task_comm) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x67654027, __VMLINUX_SYMBOL_STR(get_task_mm) },
	{ 0x2f67d258, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0xe914e41e, __VMLINUX_SYMBOL_STR(strcpy) },
	{ 0x6f5a7368, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xc7f0025b, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x61651be, __VMLINUX_SYMBOL_STR(strcat) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "006FF344D6CB4FF202414DD");
