void _dl_allocate_tls_offset(elf_object_t *object);
void *_dl_allocate_tls(char *oldtls, elf_object_t *objhead, size_t tcbsize, size_t tcbalign);
void _dl_allocate_first_tls(void);
void _dl_free_tls(void *tls, size_t tcbsize, size_t tcbalign);

#define RTLD_STATIC_TLS_EXTRA 64
/*
 * Globals to control TLS allocation.
 */
extern size_t _dl_tls_static_space; /* Static TLS space allocated */
extern int _dl_tls_dtv_generation;  /* Used to detect when dtv size changes */
extern int _dl_tls_max_index;	    /* Largest module index allocated */
extern int _dl_tls_free_idx;
extern int _dl_tls_first_done;

void * _dl_tls_get_addr_common(Elf_Addr** dtvp, int index, size_t offset);

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

