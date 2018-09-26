/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"
#include "common.h"
#include "crs.h"

#define CRS_INI_STR(strct, fd, prop) do {\
	write_str((fd), "\n" #prop "="); \
	write_str((fd), strct->prop); \
} while(0);

#define CRS_INI_INT(strct, fd, prop) do {\
	write_str((fd), "\n" #prop "="); \
	write_str((fd), ultoa(strct->prop)); \
} while(0);

void crs_meta_write_ini(const struct crs_meta *meta, const char *path_meta)
{
	int fd_meta;
	size_t i;

	if ((fd_meta = creat(path_meta, 0644)) == -1) {
		die(FMT_ERR("creat(%s) : %s", path_meta, STR_ERR));
	}

	/* fd_meta = 1; */
	write_str(fd_meta, "[__meta__]");
	CRS_INI_STR(meta, fd_meta, vendor);
	CRS_INI_STR(meta, fd_meta, name);
	CRS_INI_INT(meta, fd_meta, n_col);
	CRS_INI_INT(meta, fd_meta, n_row);
	CRS_INI_INT(meta, fd_meta, sz_row);

	for (i = 0; i < meta->n_col; i++) {
		write_str(fd_meta, "\n\n[");
		write_str(fd_meta, meta->coldef[i]->name);
		write_str(fd_meta, "]");
		CRS_INI_STR(meta->coldef[i], fd_meta, type);
		CRS_INI_INT(meta->coldef[i], fd_meta, width);
	}

	close(fd_meta);
}

void crs_set_packed_row_attr(struct packed_row_attr *attr)
{
	memset(attr, 0, sizeof(struct packed_row_attr));
	/*
	 * These are only for human eyes. Reading from crs_content must be an
	 * informed read without reliance on delimeters.
	 * */
	attr->delim_sep = "|";
	attr->delim_end = "\n";
}

/* static int crs_meta_read_ini(struct crs_meta *meta, const char *path_meta) */
/* { */

/* } */

static size_t packed_get_n_row(const struct crs_meta *meta, size_t sz_packed)
{
	size_t n_row;

	n_row = sz_packed / meta->sz_row;

	if (!n_row) {
		die(FMT_ERR("BUG: erroneous sz_packed/sz_row as %zu/%zu\n", 
					sz_packed, meta->sz_row));
	}

	return n_row;
}

/* get upper-bound row length */
static size_t packed_row_length_ub(const struct crs_meta *meta, 
		const struct packed_row_attr *attr, size_t sz_packed)
{
	size_t ub;
	size_t n_row;

	ub = sz_packed;
	if (attr->n_esc || attr->esc_bksl) {
		/*
		 * upper bound size of escaped row entry of size n will be 2n; eg. For the worst
		 * case of row of (')^n where we have to esacpe all the n '
		 */
		ub = sz_packed * 2;
	}

	n_row = packed_get_n_row(meta, sz_packed);
	
	ub += n_row * (ustrlen(attr->delim_beg) + ustrlen(attr->delim_end));
	/* # of delim_sep = # of col -1 */
	ub += n_row * (ustrlen(attr->delim_sep) * (meta->n_col - 1));
	ub += n_row * ((ustrlen(attr->surr_beg) * meta->n_col) + 
			(ustrlen(attr->surr_end) * meta->n_col));
	return ub;
}

static void memstrcpy_upd(void **dest, const char *src)
{
	size_t sz;

	sz = strlen(src);
	memcpy(*dest, src, sz);
	(*dest) = (char *)(*dest) + sz;
}

static size_t packed_row_get_buf(const struct crs_meta *meta, 
		const struct packed_row_attr *attr,
		void *buf_row, const void *buf_packed, size_t sz_packed)
{
	size_t i, iesc, width_coldef, n_row, n_sqzd_esc;
	void *buf_row_start;

	buf_row_start = buf_row;
	n_row = packed_get_n_row(meta, sz_packed);

	while (n_row--) {
		if (attr->delim_beg)
			memstrcpy_upd(&buf_row, attr->delim_beg);

		for (i = 0; i < meta->n_col; i++) {
			if (attr->surr_beg)
				memstrcpy_upd(&buf_row, attr->surr_beg);

			width_coldef = meta->coldef[i]->width;
			memcpy(buf_row, buf_packed, width_coldef);

			if (attr->esc_bksl || attr->n_esc) {
				n_sqzd_esc = 0;

				if (attr->esc_bksl)
					n_sqzd_esc += memesc(buf_row, width_coldef + n_sqzd_esc, "\\");

				for (iesc = 0; iesc < attr->n_esc; iesc++)
					n_sqzd_esc += memesc(buf_row, width_coldef + n_sqzd_esc, 
							attr->esc[iesc]);

				buf_row = (char *)buf_row + width_coldef + n_sqzd_esc;

			} else {
				buf_row = (char *)buf_row + width_coldef;
			}
			
			if (attr->surr_end)
				memstrcpy_upd(&buf_row, attr->surr_end);

			if ((i + 1) < meta->n_col) {
				if (attr->delim_sep)
					memstrcpy_upd(&buf_row, attr->delim_sep);
			}
			buf_packed = (char *)buf_packed + width_coldef;
		}

		buf_packed = (char *)buf_packed + 1;
		if (attr->delim_end)
			memstrcpy_upd(&buf_row, attr->delim_end);
	}

	return (size_t)((char *)buf_row - (char *)buf_row_start);
}

void packed_row_write(const struct crs_meta *meta, 
		const struct packed_row_attr *attr,
		int fd_content, const void *buf_packed, size_t sz_packed)
{
	void *buf_row;
	size_t sz_row, ub;

	ub = packed_row_length_ub(meta, attr, sz_packed);
	buf_row = umalloc(ub);
	sz_row = packed_row_get_buf(meta, attr, buf_row, buf_packed, sz_packed);

	if (sz_row > ub) {
		die("BUG: used more than mallocd %zu > %zu", sz_row, ub);
	}

	write(fd_content, buf_row, sz_row);
	free(buf_row);
}
