/*  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "queue.h"


static QueueSet* all_queue_sets[MAX_QUEUESETS];
static int queue_set_count = 0;
static pthread_mutex_t global_qs_mutex = PTHREAD_MUTEX_INITIALIZER;

static void get_future_time(struct timespec* ts, int ms) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    ts->tv_sec = now.tv_sec + ms / 1000;
    ts->tv_nsec = now.tv_nsec + (ms % 1000) * 1000000;
    if (ts->tv_nsec >= 1000000000) {
        ts->tv_sec += 1;
        ts->tv_nsec -= 1000000000;
    }
}
// ========== Queue Implementation ==========

void queue_init(Queue* q) {
    q->head = q->tail = NULL;
    q->size = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void queue_destroy(Queue* q) {
    while (q->head) {
        QueueNode* tmp = q->head;
        q->head = tmp->next;
        free(tmp);
    }
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

void queue_push(Queue* q, QueueData *data) {
    QueueNode* node = (QueueNode*)malloc(sizeof(QueueNode));
    memcpy(&node->data,data,sizeof(QueueData));
    //node->data = data;
    node->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if (q->tail) {
        q->tail->next = node;
    } else {
        q->head = node;
    }
    q->tail = node;
    q->size++;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);

    // Automatically notify all QueueSets that include this queue
    pthread_mutex_lock(&global_qs_mutex);
    for (int i = 0; i < queue_set_count; i++) {
        QueueSet* qs = all_queue_sets[i];
        for (int j = 0; j < qs->count; j++) {
            if (qs->queues[j] == q) {
                queueset_signal(qs);
                break;
            }
        }
    }
    pthread_mutex_unlock(&global_qs_mutex);
}

void queue_pop(Queue* q, QueueData *data) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == 0) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    QueueNode* node = q->head;
    memcpy(data,&node->data,sizeof(QueueData));
    q->head = node->next;
    if (!q->head) q->tail = NULL;
    free(node);
    q->size--;
    pthread_mutex_unlock(&q->mutex);
}

void queue_pop_timeout(Queue* q, QueueData **data, int milliseconds){
    pthread_mutex_lock(&q->mutex);

    if (q->size == 0) {
        struct timespec deadline;
        get_future_time(&deadline, milliseconds);

        while (q->size == 0) {
            int res = pthread_cond_timedwait(&q->cond, &q->mutex, &deadline);
            if (res == ETIMEDOUT) {
                pthread_mutex_unlock(&q->mutex);
                *data=NULL;
                return; // timeout
            }
        }
    }

    QueueNode* node = q->head;
    memcpy(*data,&node->data,sizeof(QueueData));
    q->head = node->next;
    if (!q->head) q->tail = NULL;
    free(node);
    q->size--;
    pthread_mutex_unlock(&q->mutex);
}

int queue_peek(Queue* q) {
    int success = 0;
    pthread_mutex_lock(&q->mutex);
    if (q->size > 0) {
        success = 1;
    }
    pthread_mutex_unlock(&q->mutex);
    return success;
}

// ========== QueueSet Implementation ==========


void queueset_init(QueueSet* qs, Queue** queues, int count) {
    qs->queues = queues;
    qs->count = count;
    pthread_mutex_init(&qs->mutex, NULL);
    pthread_cond_init(&qs->cond, NULL);

    pthread_mutex_lock(&global_qs_mutex);
    if (queue_set_count < MAX_QUEUESETS) {
        all_queue_sets[queue_set_count++] = qs;
    }
    pthread_mutex_unlock(&global_qs_mutex);
}

void queueset_destroy(QueueSet* qs) {
    pthread_mutex_destroy(&qs->mutex);
    pthread_cond_destroy(&qs->cond);
}

Queue* queueset_wait_timeout(QueueSet* qs, int milliseconds){
    struct timespec deadline;

    pthread_mutex_lock(&qs->mutex);
    get_future_time(&deadline, milliseconds);

    while (1) {
        for (int i = 0; i < qs->count; i++) {
            if (queue_peek(qs->queues[i])) {
                pthread_mutex_unlock(&qs->mutex);
                return qs->queues[i];
            }
        }

        int res = pthread_cond_timedwait(&qs->cond, &qs->mutex, &deadline);
        if (res == ETIMEDOUT) {
            pthread_mutex_unlock(&qs->mutex);
            return NULL;  // timeout reached
        }
    }
}

void queueset_signal(QueueSet* qs) {
    pthread_mutex_lock(&qs->mutex);
    pthread_cond_signal(&qs->cond);
    pthread_mutex_unlock(&qs->mutex);
}


