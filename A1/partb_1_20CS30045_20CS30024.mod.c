#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x2e2b9a95, "module_layout" },
	{ 0x2d30baa1, "remove_proc_entry" },
	{ 0x30feba91, "proc_create" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x9166fada, "strncpy" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xea8aa9a4, "kmem_cache_alloc_trace" },
	{ 0xfea80430, "kmalloc_caches" },
	{ 0x409bcb62, "mutex_unlock" },
	{ 0xc5850110, "printk" },
	{ 0xd129098d, "current_task" },
	{ 0x2ab7989d, "mutex_lock" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x37a0cba, "kfree" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "5FE1B52223832EE7C257F2F");
