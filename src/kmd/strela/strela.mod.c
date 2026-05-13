#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

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



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x98d564ff, "__platform_driver_register" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0xbcdd90dd, "misc_deregister" },
	{ 0xd9978d7d, "_dev_info" },
	{ 0xae353d77, "arm_copy_from_user" },
	{ 0x5f754e5a, "memset" },
	{ 0xbaed7496, "dma_buf_get" },
	{ 0x232b4f0e, "_dev_err" },
	{ 0xf6bb31bb, "dma_buf_attach" },
	{ 0x52a96d45, "dma_buf_put" },
	{ 0x43193ae5, "dma_buf_map_attachment" },
	{ 0x57a7e224, "dma_buf_detach" },
	{ 0xea74d525, "dma_buf_unmap_attachment" },
	{ 0x828ce6bb, "mutex_lock" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x647af474, "prepare_to_wait_event" },
	{ 0x1000e51, "schedule" },
	{ 0x49970de8, "finish_wait" },
	{ 0x9618ede0, "mutex_unlock" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xff59abd9, "devm_kmalloc" },
	{ 0xc358aaf8, "snprintf" },
	{ 0xde4bf88b, "__mutex_init" },
	{ 0x5bbe49f4, "__init_waitqueue_head" },
	{ 0x28fa933e, "platform_get_resource" },
	{ 0xb0a9ac51, "devm_platform_get_and_ioremap_resource" },
	{ 0x33822dd8, "dev_err_probe" },
	{ 0x9ca651a1, "misc_register" },
	{ 0xf2294940, "platform_get_irq" },
	{ 0x7d9fc826, "devm_request_threaded_irq" },
	{ 0x8cd2cb58, "platform_driver_unregister" },
	{ 0x637493f3, "__wake_up" },
	{ 0x2203cf01, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*CCEI,strela");
MODULE_ALIAS("of:N*T*CCEI,strelaC*");

MODULE_INFO(srcversion, "F248FA5E7006E7FB608A8CA");
