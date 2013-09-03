#include <stdio.h>
#include <err.h>
#include <unistd.h>

#include <sys/exec_elf.h>

struct image_header;

#define        roundup(x, y)   ((((x)+((y)-1))/(y))*(y))

extern u_long copy_data(int, const char *, int, const char *, u_long,
	    struct image_header *, Elf_Word);
extern u_long fill_zeroes(int, const char *, u_long, struct image_header *, Elf_Word);

u_long
ELFNAME(copy_elf)(int ifd, const char *iname, int ofd, const char *oname, u_long crc,
    struct image_header *ih)
{
	ssize_t nbytes;
	Elf_Ehdr ehdr;
	Elf_Phdr phdr;
	Elf_Addr vaddr;
	int i;

	nbytes = read(ifd, &ehdr, sizeof ehdr);
	if (nbytes == -1)
		err(1, "%s", iname);
	if (nbytes != sizeof ehdr)
		return 0;

	for (i = 0; i < ehdr.e_phnum; i++) {
#ifdef DEBUG
		fprintf(stderr, "phdr %d/%d\n", i, ehdr.e_phnum);
#endif
		if (lseek(ifd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET) ==
		    (off_t)-1)
			err(1, "%s", iname);
		if (read(ifd, &phdr, sizeof phdr) != sizeof(phdr))
			err(1, "%s", iname);

#ifdef DEBUG
		fprintf(stderr, "vaddr %p offset %p filesz %p memsz %p\n",
		    phdr.p_vaddr, phdr.p_offset, phdr.p_filesz, phdr.p_memsz);
#endif
		if (i == 0)
			vaddr = phdr.p_vaddr;
		else if (vaddr != phdr.p_vaddr) {
#ifdef DEBUG
			fprintf(stderr, "gap %p->%p\n", vaddr, phdr.p_vaddr);
#endif
			/* fill the gap between the previous phdr if any */
			crc = fill_zeroes(ofd, oname, crc, ih,
			    phdr.p_vaddr - vaddr);
			vaddr = phdr.p_vaddr;
		}

		if (phdr.p_filesz != 0) {
#ifdef DEBUG
			fprintf(stderr, "copying %p from infile %p\n",
			    phdr.p_filesz, phdr.p_offset);
#endif
			if (lseek(ifd, phdr.p_offset, SEEK_SET) == (off_t)-1)
				err(1, "%s", iname);
			crc = copy_data(ifd, iname, ofd, oname, crc, ih,
			    phdr.p_filesz);
			if (phdr.p_memsz - phdr.p_filesz != 0) {
#ifdef DEBUG
				fprintf(stderr, "zeroing %p\n",
				    phdr.p_memsz - phdr.p_filesz);
#endif
				crc = fill_zeroes(ofd, oname, crc, ih,
				    phdr.p_memsz - phdr.p_filesz);
			}
		}
		/*
		 * If p_filesz == 0, this is likely .bss, which we do not
		 * need to provide. If it's not the last phdr, the gap
		 * filling code will output the necessary zeroes anyway.
		 */
		vaddr += phdr.p_memsz;
	}

	return crc;
}
