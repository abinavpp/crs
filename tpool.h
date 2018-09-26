/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#ifndef TPOOLH
#define TPOOLH

#include "queue.h"
#include <pthread.h>

struct tpool_task {
	void *(*run)(void *arg);
	void *arg;
};

struct tpool {
	/* worker threads */
	pthread_t *thread;
	size_t n_thread;

	/* task-queue */
	struct queue_struct task_queue;
	pthread_mutex_t task_queue_lock;
	pthread_cond_t task_queue_empty;
	pthread_cond_t task_queue_not_empty;

	void (*free_task_arg)(void *);
	int8_t terminate;
};

#define TPOOL_TASK_NEW zalloc(sizeof(struct tpool_task));

#define TPOOL_NEW zalloc(sizeof(struct tpool));

#define TPOOL_INIT(tp, n, fta) \
	do { \
		(tp)->thread = umalloc(sizeof(pthread_t) * n); \
		(tp)->n_thread = n; \
		pthread_mutex_init(&(tp)->task_queue_lock, NULL); \
		pthread_cond_init(&(tp)->task_queue_empty, NULL); \
		pthread_cond_init(&(tp)->task_queue_not_empty, NULL); \
		(tp)->free_task_arg = fta; \
	} while (0);

extern void tpool_add_task(struct tpool *tp, void *(*run)(void *), void *arg);
extern void tpool_start(struct tpool *tp);
extern void tpool_terminate(struct tpool *tp);

#endif
