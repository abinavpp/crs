/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/stat.h>

#include "foxdbf.h"
#include "mixed.h"
#include "common.h"

extern struct run_attr run_attr;

static int input_check_reg(const char *path[], size_t n)
{
	size_t i;

	for (i = 0; i < n; i++) {
		if (!S_ISREG(stat_mode(path[i]))) {
			fprintf(stderr, "unexpected/unrecognized path : %s\n\n",
					path[i]);
			return -1;
		}
	}
	return 0;
}

int do_crs_from_foxdbf(const char *path_dst, const char *path_foxdbf[], 
		size_t n_foxdbf)
{
	size_t i;

	if (input_check_reg(path_foxdbf, n_foxdbf) == -1)
		return -1;

	for (i = 0; i < n_foxdbf; i++) {
		if (run_attr.verbose)
			printf("processing %s\n", path_foxdbf[i]);

		crs_from_foxdbf(path_dst, path_foxdbf[i]);

		if (run_attr.verbose)
			printf("finished %s\n", path_foxdbf[i]);
	}
	return 0;
}

int do_mysql_from_foxdbf(const char *path_dst, const char *path_foxdbf[], 
		size_t n_foxdbf)
{
	size_t i;

	if (input_check_reg(path_foxdbf, n_foxdbf) == -1)
		return -1;

	for (i = 0; i < n_foxdbf; i++) {
		if (run_attr.verbose)
			printf("processing %s\n", path_foxdbf[i]);

		mysql_from_foxdbf(path_dst, path_foxdbf[i]);

		if (run_attr.verbose)
			printf("finished %s\n", path_foxdbf[i]);
	}

	return 0;
}
