/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>

#include "util.h"
#include "crs.h"
#include "do.h"
#include "common.h"
#include "tpool.h"
#include "log.h"

enum {
	OPT_DESTDIR,
	OPT_FROM,
	OPT_TO,
	OPT_HELP,
	OPT_VERSION,
	OPT_LISTCONV,
	OPT_VERBOSE,
	OPT_N_THREAD
};

static struct options opt[] = {
	{"d", "dest-dir", 1, OPT_DESTDIR, "sets destination dir for the output"},
	{"f", "from", 1, OPT_FROM, "sets the vendor of the source database"},
	{"t", "to", 1, OPT_TO, "makes the output conform to the vendor specified"},
	{"h", "help", 0, OPT_HELP, "to view this"},
	{NULL, "version", 0, OPT_VERSION, "prints version"},
	{"v", NULL, 0, OPT_VERBOSE, "be verbose"},
	{"l", "list-conv", 0, OPT_LISTCONV, "lists supported conversions"},
	{"j", NULL, 1, OPT_N_THREAD, "parallel job count"},
	{NULL, NULL, 0, 0, NULL}
};

enum {
	FOX_2_CRS,
	CRS_2_FOX,

	MY_2_FOX,
	FOX_2_MY
};

/* TODO-forever: complete the list that never ends */
/* supported conversions */
static struct sup_conv {
	const char *from;
	const char *to;
	int ret;

} sup_conv[] = {
	{CRS_VENDOR_FOXDBF, CRS_VENDOR_CRS, FOX_2_CRS},
	/* {CRS_VENDOR_CRS, CRS_VENDOR_FOXDBF, CRS_2_FOX}, */

	/* {CRS_VENDOR_MYSQL, CRS_VENDOR_FOXDBF, MY_2_FOX}, */
	{CRS_VENDOR_FOXDBF, CRS_VENDOR_MYSQL, FOX_2_MY},
};

struct run_attr run_attr;

static void usage()
{
	printf("Usage : crs [OPTIONS] -f <arg> -t <arg> <path_1> <path_2> ...\n");
	printf("The Common Relation Structure utility\n\n");
	optusage(opt);
}

static void NORETURN version()
{
	printf("crs v0.1\n");
	printf("(C) 2018 Abinav Puthan Purayil\n");
	exit(EXIT_SUCCESS);
}

static void NORETURN list_sup_conv()
{
	size_t n_conv, i;

	n_conv = sizeof(sup_conv) / sizeof(sup_conv[0]);
	for (i = 0; i < n_conv; i++) {
		printf("%s -> %s\n", sup_conv[i].from, sup_conv[i].to);
	}

	exit(EXIT_SUCCESS);
}

static int decide_conv()
{
	size_t n_conv, i;

	n_conv = sizeof(sup_conv) / sizeof(sup_conv[0]);
	for (i = 0; i < n_conv; i++) {
		if (strcmp(run_attr.from, sup_conv[i].from) == 0 &&
				strcmp(run_attr.to, sup_conv[i].to) == 0) {
			return sup_conv[i].ret;
		}
	}

	return -1;
}

static void run_attr_verify_hard()
{
	int n_cpu;

	if (!run_attr.to[0] || !run_attr.from[0]) {
		fprintf(stderr, "from and to vendor must be specified\n\n");
		usage();
		exit(EXIT_FAILURE);
	}

	if ((run_attr.conv = decide_conv()) == -1) {
		fprintf(stderr, "unsupported / unrecognized conversion specification\n"
				"run crs -l to list supported conversions\n\n");
		usage();
		exit(EXIT_FAILURE);
	}

	if (!run_attr.argc) {
		fprintf(stderr, "main arguments missing\n\n");
		usage();
		exit(EXIT_FAILURE);
	}

	if (run_attr.path_dst[0]) {
		if (!S_ISDIR(stat_mode(run_attr.path_dst))) {
			fprintf(stderr, "unrecognized destination directory\n\n");
			usage();
			exit(EXIT_FAILURE);
		}
	} else {
		strcpy(run_attr.path_dst, ".");
	}

	n_cpu = get_nprocs();
	if (run_attr.n_thread > n_cpu) {
		run_attr.n_thread = n_cpu;
	} else if (!run_attr.n_thread) {
		run_attr.n_thread = 1;
	}
}

struct argv_chunk {
	size_t index;
	size_t n;
};

static void *run_argv_chunk(void *_avc)
{
	struct argv_chunk *avc = _avc;

	switch (run_attr.conv) {
	case FOX_2_CRS:
		if (do_crs_from_foxdbf(run_attr.path_dst, 
					run_attr.argv + avc->index, avc->n) == -1) {
			usage();
			exit(EXIT_FAILURE);
		}
		break;

	case FOX_2_MY:
		if (do_mysql_from_foxdbf(run_attr.path_dst, 
					run_attr.argv + avc->index, avc->n) == -1) {
			usage();
			exit(EXIT_FAILURE);
		}
		break;
	}
	return NULL;
}

#ifdef CONFIG_NO_MULTITHREAD
static void start_without_tpool()
{
	struct argv_chunk avc = {0, run_attr.argc};
	run_argv_chunk(&avc);
}
#else
static void start_tpool()
{
	size_t i;
	struct tpool *tp = TPOOL_NEW;
	struct argv_chunk *avc;

	TPOOL_INIT(tp, run_attr.n_thread, free);
	tpool_start(tp);
	if (run_attr.verbose) {
		printf("Running %zu thread(s)\n", run_attr.n_thread);	
	}

	for (i = 0; i < run_attr.argc; i++) {
		avc = umalloc(sizeof(struct argv_chunk));
		avc->index = i; avc->n = 1;
		tpool_add_task(tp, run_argv_chunk, avc);
	}

	tpool_terminate(tp);
}
#endif

int main(int argc, char *argv[])
{
	char *optarg;
	int optret;

	if (argc == 1) {
		usage();
		exit(EXIT_FAILURE);
	}

	log_init();
	argv++;
	run_attr.n_thread = 1;

	while ((optret = parseopt(&argv, &optarg, opt)) != PARSEOPT_DONE) {
		switch (optret) {
		case OPT_DESTDIR:
			strncpy(run_attr.path_dst, optarg, PATH_MAX);
			break;

		case OPT_FROM:
			strncpy(run_attr.from, optarg, CRS_LEN_VENDOR);
			break;

		case OPT_TO:
			strncpy(run_attr.to, optarg, CRS_LEN_VENDOR);
			break;

		case OPT_VERBOSE:
			run_attr.verbose = 1;
			break;

		case OPT_N_THREAD:
			run_attr.n_thread = atol(optarg);
			break;

		case OPT_LISTCONV:
			list_sup_conv();

		case OPT_HELP:
			usage();
			exit(EXIT_SUCCESS);

		case OPT_VERSION:
			version();

		case PARSEOPT_NOOPTARG:
			fprintf(stderr, "%s requires argument\n", *argv);
			usage();
			exit(EXIT_FAILURE);

		case PARSEOPT_UNREC:
			fprintf(stderr, "unrecognized option : %s\n", *argv);
			usage();
			exit(EXIT_FAILURE);
		}
	}

	/* set run_attr's argv and argc */
	run_attr.argv = (const char **)argv;
	for (; *argv; run_attr.argc++, argv++)
		;

	run_attr_verify_hard();

#ifdef CONFIG_NO_MULTITHREAD
	start_without_tpool();
#else
	start_tpool();
#endif
	exit(EXIT_SUCCESS);
}
