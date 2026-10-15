/*
 * a simple elf core file backtrace viewer
 *
 * Copyright (C) 2010-2026 Ali Gholami Rudi
 *
 * This program is released under the Modified BSD license.
 */
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include "elfcore.h"

/* simplified elf struct and macro names */
#ifdef __x86_64__
#  define Elf_Phdr	Elf64_Phdr
#  define Elf_Ehdr	Elf64_Ehdr
#  define Elf_Nhdr	Elf64_Nhdr
#else
#  define Elf_Phdr	Elf32_Phdr
#  define Elf_Ehdr	Elf32_Ehdr
#  define Elf_Nhdr	Elf32_Nhdr
#endif

#define ALIGN(x, a)		(((x) + (a) - 1) & ~((a) - 1))

static long readlong(void *mem, unsigned long addr)
{
	Elf_Ehdr *ehdr = mem;
	Elf_Phdr *phdr = mem + ehdr->e_phoff;
	int i;
	for (i = 0; i < ehdr->e_phnum; i++) {
		unsigned long beg = phdr[i].p_vaddr;
		if (addr >= beg && addr < beg + phdr[i].p_filesz) {
			unsigned long diff = addr - beg;
			return *(long *) (mem + phdr[i].p_offset + diff);
		}
	}
	return 0;
}

static void dump_prstatus(void *mem, struct elf_prstatus *prs)
{
	struct user_regs_struct *regs = (void *) &prs->pr_reg;
	unsigned long bp = regs->bp;
	printf("ip=%lx, sp=%lx, bp=%lx\n\n", regs->ip, regs->sp, regs->bp);
	printf("\t%lx: %lx\n", regs->ip, regs->bp);
	while (bp) {
		printf("\t%lx: %lx\n",
			readlong(mem, bp + sizeof(long)), readlong(mem, bp));
		bp = readlong(mem, bp);
	}
	printf("\n");
}

static void dumpnote(void *mem, void *note, int len)
{
	void *end = note + len;
	while (note < end) {
		Elf_Nhdr *enote = note;
		note += sizeof(*enote);
		note += ALIGN(enote->n_namesz, 4);
		if (enote->n_type == NT_PRSTATUS)
			dump_prstatus(mem, note);
		note += ALIGN(enote->n_descsz, 4);
	}
}

static void dumpelf(void *mem)
{
	Elf_Ehdr *ehdr = (void *) mem;
	Elf_Phdr *phdr = (void *) (mem + ehdr->e_phoff);
	if (ehdr->e_type == ET_CORE && phdr[0].p_type == PT_NOTE)
		dumpnote(mem, mem + phdr[0].p_offset, phdr[0].p_filesz);
}

static void die(char *msg)
{
	perror(msg);
	exit(1);
}

static long filesize(int fd)
{
	struct stat stat;
	fstat(fd, &stat);
	return stat.st_size;
}

int main(int argc, char **argv)
{
	int fd;
	long size;
	void *mem;
	if (argc < 2) {
		printf("Usage: %s path\n", argv[0]);
		return 0;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		die("open");
	size = filesize(fd);
	mem = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (!mem)
		die("mmap");
	dumpelf(mem);
	munmap(mem, size);
	close(fd);
	return 0;
}
