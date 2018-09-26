/* 
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>

#include "util.h"

#define CONFIG_DEBUG
/* #undef CONFIG_DEBUG */

#define LOG_LINE_MAX 1024

static int fd_log;
static const char *path_log = "crs-log.txt";

void log_init()
{
#ifdef CONFIG_DEBUG
	if ((fd_log = creat(path_log, 0644)) == -1) {
		die(FMT_ERR("creat(%s) : %s", path_log, STR_ERR));
	}
#endif
}

void log_debug(const char *fmt, ...)
{
#ifdef CONFIG_DEBUG
	char log_line[LOG_LINE_MAX];
	va_list ap;

	va_start(ap, fmt);
	vsprintf(log_line, fmt, ap);
	va_end(ap);

	write_str(fd_log, log_line);
#endif
}
