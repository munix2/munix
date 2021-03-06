/* Generate assembler source containing symbol information.
 * Symplified by Abderrahim Rahali 2016, suppress of the string "compression"
 * 
 * Copyright 2002       by Kai Germaschewski
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 * Usage: nm -n vmlinux | scripts/kallsyms > symbols.S
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sym_entry {
	unsigned long long addr;
	char type;
	char *sym;
};


static struct sym_entry *table;
static int size, cnt;
static unsigned long long _stext, _etext, _sinittext, _einittext;

static void
usage(void)
{
	fprintf(stderr, "Usage: nm -n kernel.elf | scripts/kallsyms > symbols.S\n");
	exit(1);
}

static int
read_symbol(FILE *in, struct sym_entry *s)
{
	char str[500];
	int rc;

	rc = fscanf(in, "%llx %c %499s\n", &s->addr, &s->type, str);
	if (rc != 3) {
		if (rc != EOF) {
			/* skip line */
			fgets(str, 500, in);
		}
		return -1;
	}
	s->sym = strdup(str);
	return 0;
}

static int
symbol_valid(struct sym_entry *s)
{
	if ((s->addr < _stext || s->addr > _etext)
	    && (s->addr < _sinittext || s->addr > _einittext))
		return 0;

	if (strstr(s->sym, "_compiled."))
		return 0;

	return 1;
}

static void
read_map(FILE *in)
{
	int i;

	while (!feof(in)) {
		if (cnt >= size) {
			size += 10000;
			table = realloc(table, sizeof(*table) * size);
			if (!table) {
				fprintf(stderr, "out of memory\n");
				exit (1);
			}
		}
		if (read_symbol(in, &table[cnt]) == 0)
			cnt++;
	}
	for (i = 0; i < cnt; i++) {
		if (strcmp(table[i].sym, "_stext") == 0)
			_stext = table[i].addr;
		if (strcmp(table[i].sym, "_etext") == 0)
			_etext = table[i].addr;
		if (strcmp(table[i].sym, "_sinittext") == 0)
			_sinittext = table[i].addr;
		if (strcmp(table[i].sym, "_einittext") == 0)
			_einittext = table[i].addr;
	}
}

static void
write_src(void)
{
	unsigned long long last_addr;
	int i, valid = 0;

	//printf("#include <asm/types.h>\n"); ??TODO cause bad instruction
	//printf("#define __ksymtab __attribute((section(__ksymtab)))"
	printf("#if BITS_PER_LONG == 64\n");
	printf("#define PTR .quad\n");
	printf("#define ALGN .align 8\n");
	printf("#else\n");
	printf("#define PTR .long\n");
	printf("#define ALGN .align 4\n");
	printf("#endif\n");

	//printf(".data\n");
	printf(".section __ksymtab\n");

	printf(".globl kallsyms_addresses\n");
	printf("\tALGN\n");
	printf("kallsyms_addresses:\n");
	for (i = 0, last_addr = 0; i < cnt; i++) {
		if (!symbol_valid(&table[i]))
			continue;
		
		if (table[i].addr == last_addr)
			continue;

		printf("\tPTR\t%#llx\n", table[i].addr);
		valid++;
		last_addr = table[i].addr;
	}
	printf("\n");

	printf(".globl kallsyms_num_syms\n");
	printf("\tALGN\n");
	printf("kallsyms_num_syms:\n");
	printf("\tPTR\t%d\n", valid);
	printf("\n");

	printf(".globl kallsyms_names\n");
	printf("\tALGN\n");
	printf("kallsyms_names:\n");
	for (i = 0, last_addr = 0; i < cnt; i++) {

		if (!symbol_valid(&table[i]))
			continue;
		
		if (table[i].addr == last_addr)
			continue;
        
        printf("\t.asciz\t\"%s\"\n", table[i].sym);
        
		last_addr = table[i].addr;
	}
	printf("\n");
}

int
main(int argc, char **argv)
{
	if (argc != 1)
		usage();

	read_map(stdin);
	write_src();

	return 0;
}

