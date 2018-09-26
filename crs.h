/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef CRSH
#define CRSH

#include <stdint.h>

#define CRS_LEN_VENDOR 16
#define CRS_LEN_COL_TYPE 16

/* CRS_COL/VENDOR_TYPE_*** allows 15 chars at max */
#define CRS_COL_TYPE_CHAR "char"
#define CRS_COL_TYPE_VARCHAR "varchar"
#define CRS_COL_TYPE_INT "int"
#define CRS_COL_TYPE_FLOAT "float"
#define CRS_COL_TYPE_DATE "date"
#define CRS_COL_TYPE_UNK "unk"

#define CRS_VENDOR_FOXDBF "foxdbf"
#define CRS_VENDOR_MYSQL "mysql"
#define CRS_VENDOR_CRS "crs"

#define EXT_MYSQL ".mysql"
#define EXT_CRS ".crs"
#define EXT_META ".meta"
#define EXT_CONTENT ".cont"


struct crs_meta {
	char vendor[CRS_LEN_VENDOR];

	/*
	 * name, each crs_coldef members, must be zalloc'd since we rely on the
	 * fact that a "null entry" means "no entry". The pointers for
	 * crs_coldef members need not be zalloc'd since we rely on n_col to
	 * determine the no. of valid crs_coldef entries.c
	 */
	char *name;
	size_t n_col;
	size_t n_row;
	/* sz_row = 0 if row are not fixed in size */
	size_t sz_row;

	struct crs_coldef {
		char *name;
		size_t width;
		char type[CRS_LEN_COL_TYPE];
		char **attr;
	} **coldef;
};

struct packed_row_attr {
	/* WARNING: changes here must reflect the functions that decide row ub */

	const char *delim_beg;
	const char *delim_end;
	const char *delim_sep;

	const char *surr_beg;
	const char *surr_end;

	/*
	 * esc holds n_esc chars that will be escaped, do not include '\'
	 * itself in esc[], instead set esc_bksl. */
	size_t n_esc;
	const char *esc[16]; /* 16 escape chars ought to be enough for all */
	uint8_t esc_bksl;
};

extern void crs_meta_write_ini(const struct crs_meta *meta, const char *path_meta);

extern void crs_set_packed_row_attr(struct packed_row_attr *attr);

extern void packed_row_write(const struct crs_meta *meta, 
		const struct packed_row_attr *attr,
		int fd_content, const void *buf_packed, size_t sz_packed);
#endif
