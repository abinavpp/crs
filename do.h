/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef DOH /* woohoo */
#define DOH /* d'oh! */

extern int do_crs_from_foxdbf(const char *path_dst, const char *path_foxdbf[], 
		size_t n_foxdbf);
extern int do_mysql_from_foxdbf(const char *path_dst, const char *path_foxdbf[], 
		size_t n_foxdbf);
#endif /* woohoo */
