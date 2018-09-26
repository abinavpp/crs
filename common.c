/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <sys/stat.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <stdarg.h>

#include "util.h"

#if defined SANE_MEM_PERMYRIAD /* or 1/10,000; yeah I wiki'd */
#	if SANE_MEM_PERMYRIAD == 0
#		undef SANE_MEM_PERMYRIAD
#		define SANE_MEM_PERMYRIAD 10
#	elif SANE_MEM_PERMYRIAD >= 5000 /* Don't allow 50%+ request */
#		undef SANE_MEM_PERMYRIAD
#		define SANE_MEM_PERMYRIAD 4999
#endif
#else
#	define SANE_MEM_PERMYRIAD 10 /* 0.001% ought to be enough for everyone */
#endif

static size_t decide_bufsz()
{
	struct sysinfo si;

	sysinfo(&si);
	unsigned long sz = si.freeram * ((float)SANE_MEM_PERMYRIAD / 10000);

	return sz;
}

/* TODO: test the hell outta this */
size_t decide_rowcnt(const char *path_file, size_t sz_row)
{
	size_t sane_stat_bufsz, sane_sys_bufsz;
	struct stat st;

	sane_sys_bufsz = decide_bufsz();
	stat(path_file, &st);
	sane_stat_bufsz = (st.st_size * ((float)10 / 100));

	if (sane_sys_bufsz < sane_stat_bufsz)
		sane_stat_bufsz = sane_sys_bufsz; /* limited by system */

	if (sane_stat_bufsz < sz_row)
		sane_stat_bufsz = sz_row; /* never return 0, even if we cause OOM in malloc */

	return (sane_stat_bufsz / sz_row);
}

/* 
 * esc should _never_ be = "\0" (strchr would succeed, trailing \0 of str
 * will be escaped!)
 * */
void stresc(char *str, const char *esc)
{
	char *at = str;

	for (; (at = strchr(at, esc[0])); at += 2) {
		/* 
		 * Since memsqz here relies on strlen + 1 of at, the trailing \0 is
		 * guaranteed to be there in the final modified str.
		 */
		memsqz(at, strlen(at) + 1, "\\", 1);
	}
}

/* XXX */
size_t memesc(void *mem, size_t sz, const char *esc)
{
	void *search_from = mem;
	void *search_end = (char *)mem + sz;
	size_t sz_search_from;

	size_t n_sqzd;

/* 
 * We don't check if the char is already escaped since we could have esc =
 * ['\', 'a'] which should make "\a" as "\\\a". Checking if the char is
 * escaped prevents us to do so.
 * */

	/* this should be n */
	sz_search_from = (size_t)((char *)search_end - (char *)search_from);
	for (n_sqzd = 0;
			(search_from = memchr(search_from, esc[0], sz_search_from)); ) {
		memsqz(search_from, sz_search_from, "\\", 1);
		n_sqzd++;

		search_end = (char *)search_end + 1; /* '\' got squeezed in */
		search_from = (char *)search_from + 2; /* skip '\' and esc[0] char */
		sz_search_from = (size_t)((char *)search_end - (char *)search_from);
	}

	return n_sqzd;
}

char *ustrcat(char *dst, size_t n, ...)
{
	va_list va;

	va_start(va, n);

	while (n--) {
		strcat(dst, va_arg(va, char *));
	}

	va_end(va);

	return dst;
}
