/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef FOXDBH
#define FOXDBH

#include <stdint.h>
#include "util.h"
#include "crs.h"

#define FOXDBF_LEN_COLNAME 11

struct foxdbf_header {
	uint8_t magic;
	struct {
		uint8_t year;
		uint8_t month;
		uint8_t day;
	} date;
	unsigned int n_row;
	unsigned short off_row1;
	unsigned short sz_row;

	/* 
	 * Bunch of unknown sections here. (maybe padding)
	 * Both, unk_1 and unk_2 are usually nulled
	 * */
	uint8_t unk_1[16];
	uint8_t has_index;
	uint8_t unk_2[3];
} PACKED;

struct foxdbf_coldef {
	char name[FOXDBF_LEN_COLNAME];
	char type;
	unsigned int sz_sofar;
	/* unsigned short width; */
	uint8_t width;
	uint8_t precision; /* ? */

	uint8_t unk[14];
} PACKED;

extern void foxdbf_read_header(struct foxdbf_header *header, const char
		*path_foxdbf);

extern void crs_meta_from_foxdbf(struct crs_meta *meta, const char *path_foxdbf);
extern void crs_content_from_foxdbf(const char *path_content, const char *path_foxdbf);
extern void crs_from_foxdbf(const char *path_dst, const char *path_foxdbf);
#endif
