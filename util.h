/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef	UTILH
#define UTILH

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#define NORETURN __attribute__((noreturn))
#define PACKED __attribute__((packed))

#define FMT_ERR(fmt, ...) "(%s at %d) : " fmt, __FILE__,  __LINE__,  ## __VA_ARGS__
#define FMT_DEBUG(fmt, ...) FMT_ERR(fmt, ## __VA_ARGS__)

#define STR_ERR strerror(errno)

#define ZALLOC_2D(var, i, m, n) do { \
	var = zalloc(sizeof(var) * (m)); \
	for (; (i) < (m); (i)++) { \
		var[(i)] = zalloc(sizeof(typeof(**(var))) * (n)); \
	} \
} while (0);

#define FREE_2D(var, i, n) do {\
	for (; (i) < (n); (i)++) { \
		free(var[(i)]); \
	} \
	free(var); \
} while (0);

#define PARSEOPT_DONE -1
#define PARSEOPT_UNREC -2
#define PARSEOPT_NOOPTARG -3

struct options {
	const char *name_short;
	const char *name_long;
	uint8_t has_arg;
	int ret; /* must be >= 0 */
	const char *desc; /* does not affect parseopt */
};

static inline void write_str(int fd, const char *str)
{
	write(fd, str, strlen(str));
}

extern void *umalloc(size_t size);
extern void *urealloc(void *ptr, size_t size);
extern void *zalloc(size_t size);
extern void *stralloc(const char *str);
extern size_t ustrlen(const char *str);
extern void cleanup(const char *fmt, ...);

extern void err_exit(int status, const char *fmt, ...);
extern void die(const char *fmt, ...);

extern char *ultoa(unsigned long inp);
extern void memsqz(void *mem, size_t sz_mem, const void *me, size_t sz_me);
extern long hash_simfn(const void *bytes, size_t n, size_t mod);

extern mode_t stat_mode(const char *path);
extern int parseopt(char **argvp[], char **optarg, const struct options opt[]);
extern void printc(size_t width[], size_t n, ...);
extern void optusage(struct options opt[]);
#endif
