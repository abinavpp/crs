/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "foxdbf.h"
#include "crs.h"
#include "util.h"
#include "common.h"
#include "log.h"

/* 
 * Unfortunately, this needs to be global since reading the header helps
 * navigating foxdbf binary for APIs attempting conversion. We are not going
 * to add foxdbf or any other format specific details in crs_meta.
 * */
void foxdbf_read_header(struct foxdbf_header *header, const char *path_foxdbf)
{
	int fd_foxdbf;

	if ((fd_foxdbf = open(path_foxdbf, O_RDONLY)) == -1) {
		die(FMT_ERR("open(%s) : %s", path_foxdbf, STR_ERR));
	}

	read(fd_foxdbf, header, sizeof(struct foxdbf_header));
	close(fd_foxdbf);
}

static size_t foxdbf_colcnt(const struct foxdbf_header *header)
{
	/* -2 trims the trailing 0x0d20 coldefs-end-marker */
	size_t sz_coldefs = (header->off_row1 - 2) - 
		sizeof(struct foxdbf_header);
	return sz_coldefs / sizeof(struct foxdbf_coldef) + 1;
}

static void foxdbf_read_cols(struct foxdbf_coldef **cols, 
		size_t n_col, const char *path_foxdbf)
{
	int fd_foxdbf;
	size_t i;

	if ((fd_foxdbf = open(path_foxdbf, O_RDONLY)) == -1) {
		die(FMT_ERR("open(%s) : %s", path_foxdbf, STR_ERR));
	}
	lseek(fd_foxdbf, sizeof(struct foxdbf_header), SEEK_SET);

	for (i = 0; i < n_col; i++) {
		read(fd_foxdbf, cols[i], sizeof(struct foxdbf_coldef));
	}

	close(fd_foxdbf);
}

/* returns name of foxdbf (allocated) referred by file @path_foxdbf */
static char *foxdbf_get_name(const char *path_foxdbf)
{
	char *base = basename((char *)path_foxdbf);
	char *foxdbf_name = (char *)stralloc(base);
	strcpy(foxdbf_name, base);

	/* nullify the '.' of .DBF */
	foxdbf_name[strlen(foxdbf_name) - 1 - 3] = '\0';

	return foxdbf_name;
}

static const char *crs_col_type_from_foxdbf(const struct foxdbf_coldef *fox_coldef)
{
	switch (fox_coldef->type) {
		case 'C':
			return CRS_COL_TYPE_CHAR;
			break;
		case 'L': /* logical/boolean */
			return CRS_COL_TYPE_CHAR;
			break;
		case 'N':
			if (fox_coldef->precision)
				return CRS_COL_TYPE_FLOAT;
			return CRS_COL_TYPE_INT;

			break;
		case 'D':
			return CRS_COL_TYPE_DATE;
			break;
		case 'M': /* memo */
			return CRS_COL_TYPE_CHAR;
			break;
		default:
			return CRS_COL_TYPE_UNK;
	}
}

void crs_meta_from_foxdbf(struct crs_meta *meta, const char *path_foxdbf)
{
	size_t fox_n_col, i;
	struct foxdbf_header fox_header;
	struct foxdbf_coldef **fox_cols;

	foxdbf_read_header(&fox_header, path_foxdbf);

	fox_n_col = foxdbf_colcnt(&fox_header);

	i = 0;
	ZALLOC_2D(fox_cols, i, fox_n_col, 1);
	foxdbf_read_cols(fox_cols, fox_n_col, path_foxdbf);

	/* populating-meta section */
	strcpy(meta->vendor, CRS_VENDOR_FOXDBF);
	meta->name = foxdbf_get_name(path_foxdbf);
	meta->sz_row = fox_header.sz_row;
	meta->n_row = fox_header.n_row;
	meta->n_col = fox_n_col;

	meta->coldef = (struct crs_coldef **)umalloc(sizeof(void *) * fox_n_col);
	for (i = 0; i < fox_n_col; i++) {
		meta->coldef[i] = (struct crs_coldef *)zalloc(sizeof(struct crs_coldef));
		meta->coldef[i]->name = (char *)umalloc(FOXDBF_LEN_COLNAME);
		strcpy(meta->coldef[i]->name, fox_cols[i]->name);
		strcpy(meta->coldef[i]->type, crs_col_type_from_foxdbf(fox_cols[i]));
		meta->coldef[i]->width =  fox_cols[i]->width;
	}

	i = 0;
	FREE_2D(fox_cols, i, fox_n_col);
}

void crs_content_from_foxdbf(const char *path_content, const char *path_foxdbf)
{
	int fd_foxdbf, fd_content;
	void *buf_fio;
	size_t buf_fio_sz, rowcnt, rwcnt;
	ssize_t rd;

	struct packed_row_attr attr;
	struct crs_meta meta;
	struct foxdbf_header fox_header;

	crs_set_packed_row_attr(&attr);

	crs_meta_from_foxdbf(&meta, path_foxdbf);
	foxdbf_read_header(&fox_header, path_foxdbf);

	if ((fd_foxdbf = open(path_foxdbf, O_RDONLY)) == -1) {
		die(FMT_ERR("open(%s) : %s", path_foxdbf, STR_ERR));
	}

	if ((fd_content = creat(path_content, 0644)) == -1) {
		die(FMT_ERR("creat(%s) : %s", path_content, STR_ERR));
	}

	lseek(fd_foxdbf, fox_header.off_row1 + 1, SEEK_SET);

	rowcnt = decide_rowcnt(path_foxdbf, meta.sz_row);
	buf_fio_sz = rowcnt * meta.sz_row;
	buf_fio = (void *)umalloc(buf_fio_sz);

	log_debug(FMT_DEBUG("%s buf_fio_sz = %zu, rowcnt = %zu out of %zu\n", 
				path_foxdbf, buf_fio_sz, rowcnt, meta.n_row));

	for (rwcnt = 0; (rd = read(fd_foxdbf, buf_fio, buf_fio_sz)) > 0; rwcnt++) {
		packed_row_write(&meta, &attr, fd_content, buf_fio, rd);
	}

	log_debug(FMT_DEBUG("%s r/w count = %zu\n", path_foxdbf, rwcnt));

	cleanup("aff", buf_fio, fd_foxdbf, fd_content);
}

void crs_from_foxdbf(const char *path_dst, const char *path_foxdbf)
{
	char path_meta[PATH_MAX], path_content[PATH_MAX], path_crs[PATH_MAX];

	struct crs_meta meta;


	if (!S_ISDIR(stat_mode(path_dst))) {
		die(FMT_ERR("%s not a directory", path_dst));
	}

	crs_meta_from_foxdbf(&meta, path_foxdbf);

	snprintf(path_crs, PATH_MAX, "%s%s%s%s", path_dst, "/", meta.name, EXT_CRS);

	/* don't reuse path_crs since its PATH_MAX long hence format-overflow */

	snprintf(path_meta, PATH_MAX, "%s%s%s%s%s%s%s", path_dst, "/", meta.name, EXT_CRS,
			"/", meta.name, EXT_META);
	snprintf(path_content, PATH_MAX, "%s%s%s%s%s%s%s", path_dst, "/", meta.name, EXT_CRS,
			"/", meta.name, EXT_CONTENT);

	if (mkdir(path_crs, 0755) == -1) {
		die(FMT_ERR("mkdir(%s) : %s", path_crs, STR_ERR));
	}

	crs_meta_write_ini(&meta, path_meta);
	crs_content_from_foxdbf(path_content, path_foxdbf);
}

