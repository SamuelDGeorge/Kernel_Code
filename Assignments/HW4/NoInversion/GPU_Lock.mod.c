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
	{ 0x6ececaf2, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x53b151c6, __VMLINUX_SYMBOL_STR(debugfs_create_file) },
	{ 0xccd2be1e, __VMLINUX_SYMBOL_STR(debugfs_create_dir) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xb4721c2b, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x9005af8f, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0x50eedeb8, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x2f67d258, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "AE08EF92CC93C42184A37F6");
