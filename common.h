/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef COMMONH
#define COMMONH

#include "util.h"
#include "crs.h"
#include <limits.h>

struct run_attr {
	char path_dst[PATH_MAX + 1];

	char from[CRS_LEN_VENDOR + 1];
	char to[CRS_LEN_VENDOR + 1];

	uint8_t verbose;

	size_t n_thread;

	int conv;

	/* the arg here refers to the "non-cmdname, non-option, non-optarg"
	 * arguments in the cmdline (unlike the arg of main()) */
	int argc;
	const char **argv;
};

extern size_t decide_rowcnt(const char *path_file, size_t sz_row);
extern void stresc(char *str, const char *esc);
extern size_t memesc(void *mem, size_t n, const char *esc);
extern char *ustrcat(char *dst, size_t n, ...);

#define WS_SURROUND(fd, str, c) do {\
	write_str((fd), c); \
	write_str((fd), (str)); \
	write_str((fd), c " "); \
} while(0);

#endif
