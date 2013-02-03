/* SMC interface for hardware */

#ifdef _KERNEL

/* XXX these defines go here ? */
#define SMC_L2_DBG	0x100
#define SMC_L2_CTL	0x102

void platform_smc_write(bus_space_tag_t, bus_space_handle_t, bus_size_t,
    uint32_t, uint32_t);

#endif /* _KERNEL */
