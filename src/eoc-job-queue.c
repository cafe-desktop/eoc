/* Eye Of Cafe - Jobs Queue
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-job-queue.c) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "eoc-jobs.h"
#include "eoc-job-queue.h"

static GCond  render_cond;
static GMutex eoc_queue_mutex;

static GQueue *thumbnail_queue = NULL;
static GQueue *load_queue = NULL;
static GQueue *model_queue = NULL;
static GQueue *transform_queue = NULL;
static GQueue *save_queue = NULL;
static GQueue *copy_queue = NULL;

static gboolean
remove_job_from_queue (GQueue *queue, EocJob *job)
{
	GList *list;

	list = g_queue_find (queue, job);

	if (list) {
		g_object_unref (G_OBJECT (job));
		g_queue_delete_link (queue, list);

		return TRUE;
	}

	return FALSE;
}

static void
add_job_to_queue_locked (GQueue *queue, EocJob  *job)
{
	g_object_ref (job);
	g_queue_push_tail (queue, job);
	g_cond_broadcast (&render_cond);
}

static gboolean
notify_finished (GObject *job)
{
	eoc_job_finished (EOC_JOB (job));

	return FALSE;
}

static void
handle_job (EocJob *job)
{
	g_object_ref (G_OBJECT (job));

	// Do the EOC_JOB cast for safety
	eoc_job_run (EOC_JOB (job));

	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_finished,
			 job,
			 g_object_unref);
}

static gboolean
no_jobs_available_unlocked (void)
{
	return  g_queue_is_empty (load_queue)      &&
		g_queue_is_empty (transform_queue) &&
		g_queue_is_empty (thumbnail_queue) &&
		g_queue_is_empty (model_queue) &&
		g_queue_is_empty (save_queue) &&
		g_queue_is_empty (copy_queue);
}

static EocJob *
search_for_jobs_unlocked (void)
{
	EocJob *job;

	job = (EocJob *) g_queue_pop_head (load_queue);
	if (job)
		return job;

	job = (EocJob *) g_queue_pop_head (transform_queue);
	if (job)
		return job;

	job = (EocJob *) g_queue_pop_head (thumbnail_queue);
	if (job)
		return job;

	job = (EocJob *) g_queue_pop_head (model_queue);
	if (job)
		return job;

	job = (EocJob *) g_queue_pop_head (save_queue);
	if (job)
		return job;

	job = (EocJob *) g_queue_pop_head (copy_queue);
	if (job)
		return job;

	return NULL;
}

static gpointer
eoc_render_thread (gpointer data G_GNUC_UNUSED)
{
	while (TRUE) {
		EocJob *job;

		g_mutex_lock (&eoc_queue_mutex);

		if (no_jobs_available_unlocked ()) {
			g_cond_wait (&render_cond, &eoc_queue_mutex);
		}

		job = search_for_jobs_unlocked ();

		g_mutex_unlock (&eoc_queue_mutex);

		/* Now that we have our job, we handle it */
		if (job) {
			handle_job (job);
			g_object_unref (G_OBJECT (job));
		}
	}
	return NULL;

}

void
eoc_job_queue_init (void)
{
	g_cond_init (&render_cond);
	g_mutex_init (&eoc_queue_mutex);

	thumbnail_queue = g_queue_new ();
	load_queue = g_queue_new ();
	model_queue = g_queue_new ();
	transform_queue = g_queue_new ();
	save_queue = g_queue_new ();
	copy_queue = g_queue_new ();

	g_thread_new ("EocJobQueue", eoc_render_thread, NULL);
}

static GQueue *
find_queue (EocJob *job)
{
	if (EOC_IS_JOB_THUMBNAIL (job)) {
		return thumbnail_queue;
	} else if (EOC_IS_JOB_LOAD (job)) {
		return load_queue;
	} else if (EOC_IS_JOB_MODEL (job)) {
		return model_queue;
	} else if (EOC_IS_JOB_TRANSFORM (job)) {
		return transform_queue;
	} else if (EOC_IS_JOB_SAVE (job)) {
		return save_queue;
	} else if (EOC_IS_JOB_COPY (job)) {
		return copy_queue;
	}

	g_assert_not_reached ();

	return NULL;
}

void
eoc_job_queue_add_job (EocJob *job)
{
	GQueue *queue;

	g_return_if_fail (EOC_IS_JOB (job));

	queue = find_queue (job);

	g_mutex_lock (&eoc_queue_mutex);

	add_job_to_queue_locked (queue, job);

	g_mutex_unlock (&eoc_queue_mutex);
}

gboolean
eoc_job_queue_remove_job (EocJob *job)
{
	gboolean retval = FALSE;

	g_return_val_if_fail (EOC_IS_JOB (job), FALSE);

	g_mutex_lock (&eoc_queue_mutex);

	if (EOC_IS_JOB_THUMBNAIL (job)) {
		retval = remove_job_from_queue (thumbnail_queue, job);
	} else if (EOC_IS_JOB_LOAD (job)) {
		retval = remove_job_from_queue (load_queue, job);
	} else if (EOC_IS_JOB_MODEL (job)) {
		retval = remove_job_from_queue (model_queue, job);
	} else if (EOC_IS_JOB_TRANSFORM (job)) {
		retval = remove_job_from_queue (transform_queue, job);
	} else if (EOC_IS_JOB_SAVE (job)) {
		retval = remove_job_from_queue (save_queue, job);
	} else if (EOC_IS_JOB_COPY (job)) {
		retval = remove_job_from_queue (copy_queue, job);
	} else {
		g_assert_not_reached ();
	}

	g_mutex_unlock (&eoc_queue_mutex);

	return retval;
}
