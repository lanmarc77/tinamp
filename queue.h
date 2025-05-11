#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <stdint.h>
#include <pthread.h>

#define QUEUE_DATA_NONE 0
#define QUEUE_DATA_INPUT 1
#define QUEUE_DATA_SD_PLAY 2
#define QUEUE_DATA_FF 3
#define QUEUE_DATA_QUIT 0xFF

typedef struct {
    uint8_t type;
    uint8_t key;
} INPUT_message_t;

typedef struct {
    uint8_t type;
    int32_t message;
} FF_message_t;

typedef struct {
    uint8_t type;
    uint8_t msgType;
    char * msgData;
    uint64_t startPosition;
    uint64_t currentPosition;
    uint64_t trackLength;
} SD_PLAY_message_t;


typedef union {
    uint8_t type;
    SD_PLAY_message_t sd_play_message;
    INPUT_message_t input_message;
    FF_message_t ff_message;
} QueueData;

typedef struct QueueNode {
    QueueData data;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* head;
    QueueNode* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int size;
} Queue;

void queue_init(Queue* q);
void queue_destroy(Queue* q);
void queue_push(Queue* q, QueueData *data);
void queue_pop_timeout(Queue* q, QueueData **data, int milliseconds);
void queue_pop(Queue* q, QueueData *data);  // blocking pop
int queue_peek(Queue* q); // non-blocking

// ----- QueueSet -----
#define MAX_QUEUESETS 32

typedef struct {
    Queue** queues;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} QueueSet;

void queueset_init(QueueSet* qs, Queue** queues, int count);
void queueset_destroy(QueueSet* qs);
Queue* queueset_wait_timeout(QueueSet* qs, int milliseconds);  // returns NULL on timeout

// Helper to signal queue set from queue_push
void queueset_signal(QueueSet* qs);


#endif
