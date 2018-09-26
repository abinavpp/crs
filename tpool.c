/*
 * SPDX-License-Identifier: GPL-2.0+
 * Copyright (C) 2018 Abinav Puthan Purayil
 * */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "queue.h"
#include "tpool.h"

static void *tpool_run_thread(void *arg)
{
	struct tpool *tp = arg;
	struct sll_node *task_queue_node;
	struct tpool_task *task;

	while (1) {
		pthread_mutex_lock(&tp->task_queue_lock);

		while (QUEUE_IS_EMPTY(&tp->task_queue) && !tp->terminate) {
			pthread_cond_wait(&tp->task_queue_not_empty, &tp->task_queue_lock);
		}

		if (tp->terminate) {
			pthread_mutex_unlock(&tp->task_queue_lock);
			pthread_exit(NULL);
		}

		task_queue_node = queue_pop(&tp->task_queue);
		
		/* for tpool_terminate */
		if (QUEUE_IS_EMPTY(&tp->task_queue)) {
			pthread_cond_signal(&tp->task_queue_empty);
		}

		pthread_mutex_unlock(&tp->task_queue_lock);

		task = task_queue_node->data;
		task->run(task->arg);

		free(task_queue_node);
		if (tp->free_task_arg)
			tp->free_task_arg(task->arg);
	}
}

void tpool_add_task(struct tpool *tp, void *(*run)(void *), void *arg)
{
	struct sll_node *task_queue_node;
	struct tpool_task *task;

	pthread_mutex_lock(&tp->task_queue_lock);

	if (tp->terminate) {
		pthread_mutex_unlock(&tp->task_queue_lock);
		return;
	}

	task = TPOOL_TASK_NEW;

	task->run = run;
	task->arg = arg;

	task_queue_node = queue_push(&tp->task_queue);
	task_queue_node->data = task;
	pthread_cond_signal(&tp->task_queue_not_empty);

	pthread_mutex_unlock(&tp->task_queue_lock);
}

void tpool_start(struct tpool *tp)
{
	size_t i = 0;

	for (i = 0; i < tp->n_thread; i++) {
		if (pthread_create(&tp->thread[i], NULL, 
					tpool_run_thread, tp) != 0) {
			fprintf(stderr, "thread creation failure\n");
			tp->n_thread--;
		}
	}
}

void tpool_terminate(struct tpool *tp)
{
	size_t i = 0;

	pthread_mutex_lock(&tp->task_queue_lock);

	if (tp->terminate)
		return;

	while (!QUEUE_IS_EMPTY(&tp->task_queue)) {
		pthread_cond_wait(&tp->task_queue_empty, &tp->task_queue_lock);
	}

	tp->terminate = 1;
	pthread_mutex_unlock(&tp->task_queue_lock);

	/* 
	 * signal remaining threads if they are waiting for non-empty task_queue,
	 * they will kill themself at checking tp-terminate after waking
	 * */
	pthread_cond_broadcast(&tp->task_queue_not_empty);

	for (i = 0; i < tp->n_thread; i++) {
		pthread_join(tp->thread[i], NULL);
	}

	
	free(tp->thread);
	free(tp);
}
