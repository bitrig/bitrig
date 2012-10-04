/*
 * USB TTL Module
 */
#define	OMAP_USBTLL_REVISION                        0x0000
#define	OMAP_USBTLL_SYSCONFIG                       0x0010
#define	OMAP_USBTLL_SYSSTATUS                       0x0014
#define	OMAP_USBTLL_IRQSTATUS                       0x0018
#define	OMAP_USBTLL_IRQENABLE                       0x001C
#define	OMAP_USBTLL_TLL_SHARED_CONF                 0x0030
#define	OMAP_USBTLL_TLL_CHANNEL_CONF(i)             (0x0040 + (0x04 * (i)))
#define	OMAP_USBTLL_SAR_CNTX(i)                     (0x0400 + (0x04 * (i)))
#define	OMAP_USBTLL_ULPI_VENDOR_ID_LO(i)            (0x0800 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_VENDOR_ID_HI(i)            (0x0801 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_PRODUCT_ID_LO(i)           (0x0802 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_PRODUCT_ID_HI(i)           (0x0803 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_FUNCTION_CTRL(i)           (0x0804 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_FUNCTION_CTRL_SET(i)       (0x0805 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_FUNCTION_CTRL_CLR(i)       (0x0806 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_INTERFACE_CTRL(i)          (0x0807 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_INTERFACE_CTRL_SET(i)      (0x0808 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_INTERFACE_CTRL_CLR(i)      (0x0809 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_OTG_CTRL(i)                (0x080A + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_OTG_CTRL_SET(i)            (0x080B + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_OTG_CTRL_CLR(i)            (0x080C + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_EN_RISE(i)         (0x080D + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_EN_RISE_SET(i)     (0x080E + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_EN_RISE_CLR(i)     (0x080F + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_EN_FALL(i)         (0x0810 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_EN_FALL_SET(i)     (0x0811 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_EN_FALL_CLR(i)     (0x0812 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_STATUS(i)          (0x0813 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_LATCH(i)           (0x0814 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_DEBUG(i)                   (0x0815 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_SCRATCH_REGISTER(i)        (0x0816 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_SCRATCH_REGISTER_SET(i)    (0x0817 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_SCRATCH_REGISTER_CLR(i)    (0x0818 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_EXTENDED_SET_ACCESS(i)     (0x082F + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_UTMI_VCONTROL_EN(i)        (0x0830 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_UTMI_VCONTROL_EN_SET(i)    (0x0831 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_UTMI_VCONTROL_EN_CLR(i)    (0x0832 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_UTMI_VCONTROL_STATUS(i)    (0x0833 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_UTMI_VCONTROL_LATCH(i)     (0x0834 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_UTMI_VSTATUS(i)            (0x0835 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_UTMI_VSTATUS_SET(i)        (0x0836 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_UTMI_VSTATUS_CLR(i)        (0x0837 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_USB_INT_LATCH_NOCLR(i)     (0x0838 + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_VENDOR_INT_EN(i)           (0x083B + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_VENDOR_INT_EN_SET(i)       (0x083C + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_VENDOR_INT_EN_CLR(i)       (0x083D + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_VENDOR_INT_STATUS(i)       (0x083E + (0x100 * (i)))
#define	OMAP_USBTLL_ULPI_VENDOR_INT_LATCH(i)        (0x083F + (0x100 * (i)))


