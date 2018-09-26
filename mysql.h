/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef MYSQLH
#define MYSQLH

#include "crs.h"

extern int mysql_from_crs_meta(const struct crs_meta *meta, int fd_quer);

extern void mysql_set_packed_row_attr(struct packed_row_attr *attr);

#endif
