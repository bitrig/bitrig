/*
 * Copyright (c) 2014 Pedro Martelletto
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/exec_elf.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *kern_path;
const char *fs_path;
bool forcing = false;
bool extracting = false;
bool inspecting = false;
int kern_fd;
int fs_fd;

#define error(...)	err(EXIT_FAILURE, __VA_ARGS__)
#define xerror(...)	errx(EXIT_FAILURE, __VA_ARGS__)

void
xread(int fd, void *buf, size_t nbytes, off_t offset)
{
	ssize_t n = pread(fd, buf, nbytes, offset);
	if (n < 0 || (size_t)n != nbytes)
		error("pread");
}

void
xwrite(int fd, void *buf, size_t nbytes, off_t offset)
{
	ssize_t n = pwrite(fd, buf, nbytes, offset);
	if (n < 0 || (size_t)n != nbytes)
		error("pwrite");
}

void
xseek(int fd, Elf_Off offset, int whence)
{
	off_t n = lseek(fd, offset, whence);
	if (n < 0 || (Elf_Off)n != offset)
		error("lseek");
}

void
read_ehdr(Elf_Ehdr **ehdr)
{
	*ehdr = calloc(1, sizeof(**ehdr));
	if (*ehdr == NULL)
		error("calloc ehdr");

	xread(kern_fd, *ehdr, sizeof(**ehdr), 0);

	if (IS_ELF(**ehdr) == 0)
		xerror("%s: not an elf binary", kern_path);

	if ((*ehdr)->e_ident[EI_CLASS] != ELFCLASS)
		xerror("%s: unsupported elf class", kern_path);

	if ((*ehdr)->e_ehsize != sizeof(**ehdr))
		xerror("%s: invalid elf header size", kern_path);
	if ((*ehdr)->e_phentsize != sizeof(Elf_Phdr))
		xerror("%s: invalid program header size", kern_path);
	if ((*ehdr)->e_shentsize != sizeof(Elf_Shdr))
		xerror("%s: invalid section header size", kern_path);
}

void
write_ehdr(Elf_Ehdr *ehdr)
{
	xwrite(kern_fd, ehdr, sizeof(*ehdr), 0);
	free(ehdr);
}

void
read_phdr(const Elf_Ehdr *ehdr, Elf_Phdr **phdr)
{
	size_t phdr_size;

	*phdr = calloc(ehdr->e_phnum, ehdr->e_phentsize);
	if (*phdr == NULL)
		error("calloc phdr");

	phdr_size = (size_t)ehdr->e_phnum * ehdr->e_phentsize;
	xread(kern_fd, *phdr, phdr_size, ehdr->e_phoff);
}

void
write_phdr(const Elf_Ehdr *ehdr, Elf_Phdr *phdr)
{
	size_t phdr_size;

	if (SIZE_MAX / ehdr->e_phentsize < ehdr->e_phnum)
		xerror("%s: too many program header entries", kern_path);
	phdr_size = (size_t)ehdr->e_phnum * ehdr->e_phentsize;

	xwrite(kern_fd, phdr, phdr_size, ehdr->e_phoff);
	free(phdr);
}

void
read_shdr(const Elf_Ehdr *ehdr, Elf_Shdr **shdr)
{
	size_t shdr_size;

	*shdr = calloc(ehdr->e_shnum, ehdr->e_shentsize);
	if (*shdr == NULL)
		error("calloc shdr");

	shdr_size = (size_t)ehdr->e_shnum * ehdr->e_shentsize;
	xread(kern_fd, *shdr, shdr_size, ehdr->e_shoff);
}

void
write_shdr(const Elf_Ehdr *ehdr, Elf_Shdr *shdr)
{
	size_t shdr_size;

	if (SIZE_MAX / ehdr->e_shentsize < ehdr->e_shnum)
		xerror("%s: too many section header entries", kern_path);
	shdr_size = (size_t)ehdr->e_shnum * ehdr->e_shentsize;

	xwrite(kern_fd, shdr, shdr_size, ehdr->e_shoff);
	free(shdr);
}

int
scan_phdr(const Elf_Ehdr *ehdr, const Elf_Phdr *phdr, Elf_Off *offset)
{
	int i, s = -1;

	for (i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type == PT_BITRIG_TMPFS_RAMDISK)
			s = i;
		else if (offset != NULL)
			if (phdr[i].p_offset + phdr[i].p_memsz > *offset)
				*offset = phdr[i].p_offset + phdr[i].p_memsz;
	}

	return (s);
}

void
scan_shdr(const Elf_Ehdr *ehdr, const Elf_Shdr *shdr, Elf_Off *offset)
{
	int i;

	for (i = 0; i < ehdr->e_shnum; i++) {
		if (shdr[i].sh_offset + shdr[i].sh_size > *offset)
			*offset = shdr[i].sh_offset + shdr[i].sh_size;
	}
}

void
open_kern(const char *path)
{
	int flags = (extracting || inspecting) ? O_RDONLY : O_RDWR;
	kern_path = path;
	kern_fd = open(kern_path, flags);
	if (kern_fd < 0)
		error("open %s", kern_path);
}

void
open_fs(const char *path)
{
	fs_path = path;

	if (extracting) {
		if (forcing)
			fs_fd = open(fs_path, O_WRONLY|O_CREAT|O_TRUNC, 0600);
		else
			fs_fd = open(fs_path, O_WRONLY|O_CREAT|O_EXCL, 0600);
	} else
		fs_fd = open(fs_path, O_RDONLY);

	if (fs_fd < 0)
		error("open %s", fs_path);
}

void
dup_fs(void)
{
	if (extracting) {
		fs_path = "stdout";
		fs_fd = dup(STDOUT_FILENO);
	} else {
		fs_path = "stdin";
		fs_fd = dup(STDIN_FILENO);
	}

	if (fs_fd < 0)
		error("dup %s", fs_path);
}

void
setup(int argc, char **argv)
{
	struct stat kern_st, fs_st;

	open_kern(argv[0]);

	if (argc > 1)
		open_fs(argv[1]);
	else
		dup_fs();

	if (fstat(kern_fd, &kern_st) < 0)
		error("fstat");
	if (fstat(fs_fd, &fs_st) < 0)
		error("fstat");

	if (kern_st.st_dev == fs_st.st_dev &&
	    kern_st.st_ino == fs_st.st_ino)
		xerror("kernel and fs are the same file");
}

#define CHUNKSIZ	(8 * 1024 * 1024)

size_t
drain_fs(void)
{
	char *buf;
	size_t count = 0;

	buf = calloc(1, CHUNKSIZ);
	if (buf == NULL)
		error("calloc buf");

	for (;;) {
		ssize_t r = read(fs_fd, buf, CHUNKSIZ);
		if (r < 0)
			error("read %s", fs_path);
		if (r == 0)
			break;
		if (write(kern_fd, buf, r) != r)
			error("write %s", kern_path);
		count += r;
	}

	free(buf);

	return (count);
}

/*
 * This function's unusual structure mimics the procedure taken in
 * loadfile_elf.c by the boot loader.
 */
__dead void
inspect(const Elf_Ehdr *ehdr, const Elf_Phdr *phdr)
{
	Elf_Shdr *shdr;
	uint64_t maxsize = 0;

	read_shdr(ehdr, &shdr);

	for (int i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type != PT_LOAD ||
		   (phdr[i].p_flags & (PF_R|PF_W|PF_X)) == 0)
			continue;
		if (phdr[i].p_paddr + phdr[i].p_filesz > maxsize)
			maxsize = phdr[i].p_paddr + phdr[i].p_filesz;
		if (phdr[i].p_filesz < phdr[i].p_memsz) /* BSS */
			maxsize += phdr[i].p_memsz - phdr[i].p_filesz;
	}

	maxsize += roundup(ehdr->e_shnum * sizeof(Elf_Shdr), sizeof(Elf_Addr));

	for (int i = 0; i < ehdr->e_shnum; i++) {
		if (shdr[i].sh_type == SHT_SYMTAB ||
		    shdr[i].sh_type == SHT_STRTAB)
			maxsize += roundup(shdr[i].sh_size, sizeof(Elf_Addr));
	}

	for (int i = 0; i < ehdr->e_phnum; i++)
		if (phdr[i].p_type == PT_BITRIG_TMPFS_RAMDISK)
			maxsize += phdr[i].p_filesz;

	maxsize &= 0xfffffff;
	printf("%s will need %lluMB to boot\n", kern_path,
	    maxsize / 1024 / 1024);
#ifdef __amd64__
	printf("max kernel size on amd64 is %dMB\n", 0xfffffff / 1024 / 1024);
#endif

	exit(0);
}

#define min(a, b)	(((a) < (b)) ? (a) : (b))

__dead void
extract(const Elf_Phdr *phdr, int i)
{
	char *buf;
	Elf_Word n = 0;

	buf = calloc(1, CHUNKSIZ);
	if (buf == NULL)
		error("calloc buf");

	xseek(kern_fd, phdr[i].p_offset, SEEK_SET);

	while (n < phdr[i].p_filesz) {
		ssize_t chunksiz = min(phdr[i].p_filesz - n, CHUNKSIZ);
		if (read(kern_fd, buf, chunksiz) != chunksiz)
			error("read %s", kern_path);
		if (write(fs_fd, buf, chunksiz) != chunksiz)
			error("write %s", fs_path);
		n += chunksiz;
	}

	exit(0);
}

__dead void
replace(Elf_Ehdr *ehdr, Elf_Phdr *phdr)
{
	Elf_Shdr *shdr;
	Elf_Off off = 0;
	int slot;

	slot = scan_phdr(ehdr, phdr, &off);
	if (slot == -1)
		error("missing slot");

	read_shdr(ehdr, &shdr);
	scan_shdr(ehdr, shdr, &off);

	if (ftruncate(kern_fd, off) < 0)
		error("truncate %s", kern_path);

	off = roundup(off, PAGE_SIZE);
	xseek(kern_fd, off, SEEK_SET);

	phdr[slot].p_offset = off;
	phdr[slot].p_filesz = drain_fs();
	off += phdr[slot].p_filesz;
	off = roundup(off, PAGE_SIZE);

	printf("replaced %llu bytes at offset 0x%llx\n",
	    (unsigned long long)phdr[slot].p_filesz,
	    (unsigned long long)phdr[slot].p_offset);
	printf("rewriting section header table at offset 0x%llx\n",
	    (unsigned long long)off);

	ehdr->e_shoff = off;
	write_shdr(ehdr, shdr);
	write_phdr(ehdr, phdr);
	write_ehdr(ehdr);

	exit(0);
}

__dead void
insert(Elf_Ehdr *ehdr, Elf_Phdr *phdr)
{
	Elf_Shdr *shdr;
	Elf_Off off = 0;
	int slot;

	scan_phdr(ehdr, phdr, &off);
	read_shdr(ehdr, &shdr);
	scan_shdr(ehdr, shdr, &off);

	phdr = realloc(phdr, (ehdr->e_phnum + 1) * sizeof(Elf_Phdr));
	if (phdr == NULL)
		error("realloc phdr");

	xseek(kern_fd, off, SEEK_SET);

	slot = ehdr->e_phnum++;
	bzero(&phdr[slot], sizeof(phdr[slot]));
	phdr[slot].p_type = PT_BITRIG_TMPFS_RAMDISK;
	phdr[slot].p_flags = PF_R;
	phdr[slot].p_offset = off;
	phdr[slot].p_filesz = drain_fs();
	phdr[slot].p_align = PAGE_SIZE;
	off += phdr[slot].p_filesz;
	off = roundup(off, PAGE_SIZE);

	printf("inserted %llu bytes at offset 0x%llx\n",
	    (unsigned long long)phdr[slot].p_filesz,
	    (unsigned long long)phdr[slot].p_offset);
	printf("rewriting section header table at offset 0x%llx\n",
	    (unsigned long long)off);

	ehdr->e_shoff = off;
	write_shdr(ehdr, shdr);
	write_phdr(ehdr, phdr);
	write_ehdr(ehdr);

	exit(0);
}

__dead void
usage(void)
{
	extern char *__progname;
	fprintf(stderr, "usage: %s [-fIX] <kernel image> [fs image]\n",
	    __progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	int ch, slot;
	Elf_Ehdr *ehdr;
	Elf_Phdr *phdr;

	while ((ch = getopt(argc, argv, "fhIX")) != -1) {
		switch (ch) {
		case 'f':
			forcing = true;
			break;
		case 'h':
			usage();
			/* NOTREACHED */
		case 'I':
			inspecting = true;
			break;
		case 'X':
			extracting = true;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1 || argc > 2 || (inspecting && extracting))
		usage();

	setup(argc, argv);

	read_ehdr(&ehdr);
	read_phdr(ehdr, &phdr);

	slot = scan_phdr(ehdr, phdr, NULL);

	if (inspecting)
		inspect(ehdr, phdr);

	if (extracting) {
		if (slot == -1)
			xerror("couldn't locate fs in %s", kern_path);
		extract(phdr, slot);
	}

	if (slot != -1) {
		if (forcing)
			replace(ehdr, phdr);
		else
			xerror("%s already contains an fs", kern_path);
	}

	insert(ehdr, phdr);
}
