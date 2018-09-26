/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#include "crs.h"
#include "mysql.h"
#include "util.h"
#include "common.h"
#include "foxdbf.h"
#include "mixed.h"
#include "log.h"

int mysql_from_foxdbf(const char *path_dst, const char *path_foxdbf)
{
	int fd_foxdbf, fd_quer;
	void *buf_fio;
	char path_quer[PATH_MAX];
	size_t buf_fio_sz, rwcnt;
	ssize_t rd;

	struct foxdbf_header fox_header;
	struct crs_meta meta;
	struct packed_row_attr attr;

	if (!S_ISDIR(stat_mode(path_dst))) {
		die(FMT_ERR("%s not a directory", path_dst));
	}

	crs_meta_from_foxdbf(&meta, path_foxdbf);
	mysql_set_packed_row_attr(&attr);

	sprintf(path_quer, "%s%s%s%s", path_dst, "/", meta.name, EXT_MYSQL);

	if ((fd_quer = creat(path_quer, 0644)) == -1) {
		die(FMT_ERR("creat(%s) : %s", path_quer, STR_ERR));
	}

	mysql_from_crs_meta(&meta, fd_quer);
	foxdbf_read_header(&fox_header, path_foxdbf);

	if ((fd_foxdbf = open(path_foxdbf, O_RDONLY)) == -1) {
		die(FMT_ERR("open(%s) : %s", path_foxdbf, STR_ERR));
	}

	lseek(fd_foxdbf, fox_header.off_row1 + 1, SEEK_SET);

	buf_fio_sz = decide_rowcnt(path_foxdbf, meta.sz_row) * meta.sz_row;
	buf_fio = (void *)umalloc(buf_fio_sz);

	/* to stop the import from being dead-slow */
	write_str(fd_quer, "SET autocommit=0;\n");

	write_str(fd_quer, "INSERT INTO ");
	WS_SURROUND(fd_quer, meta.name, "`");
	write_str(fd_quer, "VALUES ");

	for (rwcnt = 0; (rd = read(fd_foxdbf, buf_fio, buf_fio_sz)) > 0; rwcnt++) {
		packed_row_write(&meta, &attr, fd_quer, buf_fio, rd);
	}

	log_debug(FMT_DEBUG("%s r/w count = %zu\n", path_foxdbf, rwcnt));
	/* write over the trailing ",\n" of the last row */
	lseek(fd_quer, -2, SEEK_CUR);

	write_str(fd_quer, ";\nCOMMIT;\n");

	cleanup("aff", buf_fio, fd_foxdbf, fd_quer);
	return 0;
}
