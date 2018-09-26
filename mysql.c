/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <string.h>

#include "mysql.h"
#include "crs.h"
#include "common.h"

int mysql_from_crs_meta(const struct crs_meta *meta, int fd_quer)
{
	size_t i;

	write_str(fd_quer, "CREATE TABLE ");
	WS_SURROUND(fd_quer, meta->name, "`");
	write_str(fd_quer, "(");

	for (i = 0; i < meta->n_col; i++) {
		/* backticks escape reserved words like GROUP */
		WS_SURROUND(fd_quer, meta->coldef[i]->name, "`");
		write_str(fd_quer, meta->coldef[i]->type);
		if (strcmp(meta->coldef[i]->type, CRS_COL_TYPE_DATE)) {
			write_str(fd_quer, "(");
			/* TODO: handle precision */
			write_str(fd_quer, ultoa(meta->coldef[i]->width));
			write_str(fd_quer, ")");
		}

		if (i + 1 < meta->n_col)
			write_str(fd_quer, ", ");
	}
	write_str(fd_quer, ");\n\n");

	return 0;
}

void mysql_set_packed_row_attr(struct packed_row_attr *attr)
{
	memset(attr, 0 ,sizeof(struct packed_row_attr));

	attr->delim_beg = "(";
	attr->delim_sep = ", ";
	attr->delim_end = "),\n";

	attr->surr_beg = "'";
	attr->surr_end = "'";

	attr->n_esc = 1;
	attr->esc[0] = "'";
	attr->esc_bksl = 1;
}

/* int mysql_from_crs(const struct crs_meta *meta, const char *path_content) */
/* { */
	/* TODO */
/* } */


