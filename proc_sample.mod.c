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
	{ 0xab6e4c24, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xcf823f49, __VMLINUX_SYMBOL_STR(single_release) },
	{ 0x36047c81, __VMLINUX_SYMBOL_STR(seq_read) },
	{ 0xf4fe4a0b, __VMLINUX_SYMBOL_STR(seq_lseek) },
	{ 0x6d5313f3, __VMLINUX_SYMBOL_STR(remove_proc_entry) },
	{ 0xc3dbeec2, __VMLINUX_SYMBOL_STR(proc_create_data) },
	{ 0xcc5e541f, __VMLINUX_SYMBOL_STR(init_task) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x5a921311, __VMLINUX_SYMBOL_STR(strncmp) },
	{ 0x4f6b400b, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x91831d70, __VMLINUX_SYMBOL_STR(seq_printf) },
	{ 0xb7e20ac9, __VMLINUX_SYMBOL_STR(single_open) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "838B88103895950A70C07B7");
