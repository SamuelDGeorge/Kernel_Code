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
	{ 0x50eedeb8, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x4b6c0e4e, __VMLINUX_SYMBOL_STR(find_get_pid) },
	{ 0x72df2f2a, __VMLINUX_SYMBOL_STR(up_read) },
	{ 0xb1e25684, __VMLINUX_SYMBOL_STR(__trace_bputs) },
	{ 0xd0f0d945, __VMLINUX_SYMBOL_STR(down_read) },
	{ 0x67654027, __VMLINUX_SYMBOL_STR(get_task_mm) },
	{ 0x2f67d258, __VMLINUX_SYMBOL_STR(current_task) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0xf0fdf6cb, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xc72e1233, __VMLINUX_SYMBOL_STR(__trace_bprintk) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x3923e938, __VMLINUX_SYMBOL_STR(mem_map) },
	{ 0x4cdb3178, __VMLINUX_SYMBOL_STR(ns_to_timeval) },
	{ 0xc87c1f84, __VMLINUX_SYMBOL_STR(ktime_get) },
	{ 0xb4390f9a, __VMLINUX_SYMBOL_STR(mcount) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "6C28BEE3D459FD0F44F727C");
