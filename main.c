#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MAX_SIZE 10
#define TASK_LIMIT 100


struct Task {
    void* (*fn)(void*) ;
}; 

struct Thread {
    pthread_t t;
    int id;
};

struct Queue {
    struct Task task_queue[MAX_SIZE];
    int head,tail,count;
    pthread_mutex_t mutex;
    pthread_cond_t notify;
};

int shutdown = 0;
int task_count = 0;

struct Queue Q;

void* 
worker(void* args) {

    int id = ((struct Thread*)args)->id;

    while(1) {
        if ((pthread_mutex_lock(&Q.mutex)) != 0 ) {
            perror("worker: failed to lock mutex\n");
            exit(0);
        }
        // Because the condition can change before an awakened thread returns 
        // from pthread_cond_wait(), the condition that caused the wait must 
        // be retested before the mutex lock is acquired. The recommended test 
        // method is to write the condition check as a while() loop that calls 
        // pthread_cond_wait().
        while (Q.count == 0 && shutdown != 1) {
            if ((pthread_cond_wait(&Q.notify,&Q.mutex)) != 0) {
                perror("worker: failed to wait\n");
                exit(0);
            }
        } 

        if (shutdown == 1) {
           break; 
        }

        struct Task t = Q.task_queue[Q.head];
        Q.head = (Q.head + 1) % MAX_SIZE;
        Q.count--;
        task_count++;
        int task_id = task_count;
        if ((pthread_mutex_unlock(&Q.mutex)) != 0 ) {
            perror("worker: failed to unlock mutex\n");
            exit(0);
        }
        t.fn((void*)&task_id);
    }
    if ((pthread_mutex_unlock(&Q.mutex)) != 0 ) {
        perror("worker: failed to unlock mutex\n");
        exit(0);
    }
    pthread_exit(NULL);
    return NULL;
}

void*
task(void* args) {
    int id = *(int*)args; 
    printf("task number %d was done\n",id);
    return NULL;
}

void
insert_task() {
    if ((pthread_mutex_lock(&Q.mutex)) != 0 ) {
        perror("insert_task: failed to lock mutex\n");
        exit(0);
    }
    if (Q.count == MAX_SIZE) {
        if ((pthread_mutex_unlock(&Q.mutex)) != 0) {
            perror("insert_task: failed to lock mutex\n");
            exit(0);
        }
        return;
    }
    struct Task t = {task};
    Q.task_queue[Q.tail] = t;
    Q.tail = ( Q.tail + 1 ) % MAX_SIZE;
    Q.count++;
    if ((pthread_cond_signal(&Q.notify)) != 0) {
        perror("insert_task: failed to notify from\n");
        exit(0);
    }
    if ((pthread_mutex_unlock(&Q.mutex)) != 0) {
        perror("insert_task: failed to lock mutex\n");
        exit(0);
    }
}

int 
main() {
    srand(time(NULL));
    struct Thread thread_pool[MAX_SIZE];

    if (pthread_mutex_init(&Q.mutex, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

     if (pthread_cond_init(&Q.notify,NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }


    for (int i = 0 ; i < MAX_SIZE; i++) {
        thread_pool[i].id = i;
        pthread_create(&thread_pool[i].t,NULL,worker,(void*)&thread_pool[i]);
    }



    while (1) {
        insert_task();
        pthread_mutex_lock(&Q.mutex);
        if (task_count >= TASK_LIMIT) {
            shutdown = 1;
            pthread_cond_broadcast(&Q.notify);
            pthread_mutex_unlock(&Q.mutex);
            break;
        }
        pthread_mutex_unlock(&Q.mutex);
    }

    for (int i = 0 ; i < MAX_SIZE; i++) {
        pthread_join(thread_pool[i].t,NULL);
    }

}
