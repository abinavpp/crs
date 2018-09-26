/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>

#include "util.h"

void NORETURN err_exit(int status, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	_exit(status);
}

void NORETURN die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	_exit(EXIT_FAILURE);
}

/* all allocation functions are fatal on failure */

void *umalloc(size_t size)
{
	void *alloc;

	alloc = malloc(size);

	if (!alloc) {
		die("malloc failed!");
	}
	return alloc;
}

void *urealloc(void *ptr, size_t size)
{
	void *alloc;

	alloc = realloc(ptr, size);

	if (!alloc) {
		die("malloc failed!");
	}
	return alloc;
}

/* returns zeroed allocation of size bytes */
void *zalloc(size_t size)
{
	void *alloc;

	alloc = calloc(size, 1);

	if (!alloc) {
		die("malloc failed!");
	}
	return alloc;
}

void *stralloc(const char *str)
{
	void *alloc;
	size_t len = strlen(str);

	alloc = umalloc(len + 1); /* +1 for '\0' */

	return alloc;
}

size_t ustrlen(const char *str)
{
	if (str)
		return strlen(str);
	else
		return 0;
}

char *ultoa(unsigned long inp)
{
	unsigned long n = snprintf(NULL, 0, "%lu", inp);
	char *str = (char *)umalloc(n + 1);
	snprintf(str, n + 1, "%lu", inp);
	return str;
}

/* returns hash value of a string of n bytes modulo mod */
long hash_simfn(const void *bytes, size_t n, size_t mod)
{
	int base = UCHAR_MAX + 1;
	unsigned long hash = 0;
	size_t i;
	char *chars = (char *)bytes;

	for (i = 0; i < n; i++) {
		hash = (base * hash + chars[i]) % mod;
	}

	return hash;
}

/* squeeze @me of length @sz_me at @mem of length sz_mem */
void memsqz(void *mem, size_t sz_mem, const void *me, size_t sz_me)
{
	memmove((char *)mem + sz_me, mem, sz_mem);
	memcpy(mem, me, sz_me);
}

mode_t stat_mode(const char *path)
{
	struct stat st;
	if (stat(path, &st) == -1) {
		return -1; /* all bits set = crazy file */
	}

	return st.st_mode;
}

void cleanup(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	while (*fmt) {
		switch (*fmt++) {
		case 'f':
			close(va_arg(ap, int));
			break;
		case 'a':
			free(va_arg(ap, void *));
			break;
		default :
			break;
		}
	}
	va_end(ap);
}

/*
 * pretty prints @n columns of text where each column corresponds
 * to each string args @... after @n. Column sizes(fixed) correspond to @width[].
 * Make sure @width[] has 1-1 correspondence with @... and that their
 * cardinality = n.
 *  */
void printc(size_t width[], size_t n, ...)
{
	char arg[1024], fmt[1024];
	const char **argv;
	uint8_t *argdone;
	va_list ap;
	size_t done = 0, i;

	va_start(ap, n);
	/* tracking "where we left off" for each arg when printing lines */
	argv = (const char **)umalloc(n * sizeof(void *));

	/* argdone[i] == 0 ? pending chars to be printed in argv[i] : argv[i] is
	 * fully printed. Num of argdone[] set = done */
	argdone = (uint8_t *)zalloc(n * sizeof(uint8_t));

	for (i = 0; i < n; i++) {
		argv[i] = va_arg(ap, char *);
	}
	va_end(ap);

	while (done != n) {
		for (i = 0; i < n; i++) {
			strncpy(arg, argv[i], width[i]);
			arg[width[i]] = 0;

			sprintf(fmt, "%%-%zus ", width[i]);
			printf(fmt, arg);

			/* the messy update argv section */
			if (strlen(argv[i]) < width[i]) {
				/* don't skip by width[i] if argv[i] is not long enough,
				 * else we'll start printing garbage */
				argv[i] = "";
			} else {
				argv[i] = argv[i] + width[i];
			}
			if (!strlen(argv[i]) && !argdone[i]) {
				argdone[i] = 1;
				done++;
			}
		}
		printf("\n");
	}

	free(argv);
	free(argdone);
}

void optusage(struct options opt[])
{
	char opt_short[8];
	char opt_long[1024];

	size_t width[] = {4, 20, 75};
	size_t i;

	for (i = 0; (opt[i].name_short || opt[i].name_long)
			&& opt[i].desc; i++) {
		/* init opt_x strings as empty */
		opt_short[0] = 0;
		opt_long[0] = 0;
		if (opt[i].name_short)
			sprintf(opt_short, "  -%c", opt[i].name_short[0]);
		if (opt[i].name_long)
			sprintf(opt_long, "--%s", opt[i].name_long);
		if (opt[i].has_arg)
			strcat(opt_long, " <arg>");

		printc(width, 3, opt_short, opt_long, opt[i].desc);
	}
}

int parseopt(char **argvp[], char **optarg, const struct options opt[])
{
	char **argv = *argvp;
	const char *arg = *argv;
	int ret = PARSEOPT_UNREC, has_optarg = 0;
	size_t i;

	if (!arg || arg[0] != '-' || !arg[1])
		return PARSEOPT_DONE;

	if ((*optarg = strchr(arg, '='))) {
		/* 
		 * nullify '=', so that strcmping on arg works. This is why we have
		 * to do this before deciding the option.
		 * */
		**optarg = 0;
		(*optarg)++;
		if (**optarg == 0) {
			/* in case of -x= ... or -xx= ...*/
			return PARSEOPT_NOOPTARG;
		}
	} /* else *optarg = NULL (strchr fail) */

	/* long option or -- terminator */
	if (arg[1] == '-') {
		if (!arg[2]) { /* -- terminator */
			return PARSEOPT_DONE;
		}
		for (i = 0; opt[i].name_short || opt[i].name_long; i++) {
			if (opt[i].name_long && strcmp(arg + 2, opt[i].name_long) == 0) {
				has_optarg = (opt[i].has_arg) ? 1 : 0;
				ret = opt[i].ret;
			}
		}

	/* short option */	
	} else {
		for (i = 0; opt[i].name_short || opt[i].name_long; i++) {
			if (opt[i].name_short && strncmp(arg + 1, opt[i].name_short, 1) == 0) {
				has_optarg = (opt[i].has_arg) ? 1 : 0;
				ret = opt[i].ret;
			}
		}
	}

	if (ret == PARSEOPT_UNREC)
		return PARSEOPT_UNREC;

	/* since has_optarg is by default 0, if opt is unrecognized, it
	 * would remain 0 */
	if (has_optarg && *optarg == NULL) {
		if (argv[1] == NULL)
			return PARSEOPT_NOOPTARG;
		*optarg = argv[1];
		*argvp = argv + 2;
		return ret;
	}

	/* for opts without optarg or --opt=x or -o=x kinds */
	*argvp = argv + 1;
	return ret;
}
