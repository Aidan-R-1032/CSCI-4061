#include <stdio.h>
#include <string.h>
#include "connection_queue.h"
#include <unistd.h>

int connection_queue_init(connection_queue_t *queue) {
   //set fields of the queue struct
    queue->length = 0; //difference between read and write idx
    queue->capacity = 5;
    queue->shutdown = 0;
    queue->read_idx = 0;
    queue->write_idx = 0;
    queue->directory = "";
    memset(queue->client_fds, 0, queue->capacity); //set array
    int result = 0; //for error checks
    
    //call init functions for the mutex and conditionals
    if((result = pthread_mutex_init(&queue->lock, NULL)) != 0){
    fprintf(stderr, "mutex_init failed: %s\n", strerror(result));
    return -1;
    }
    if((result = pthread_cond_init(&queue->write_catchup, NULL)) != 0){
    fprintf(stderr, "Pthread_cond for write_catchup failed: %s\n", strerror(result));
    return -1;
    }
    if((result = pthread_cond_init(&queue->read_catchup, NULL)) != 0){
    fprintf(stderr, "Pthread_cond for read_catchup failed: %s\n", strerror(result));
    return -1;
    }

    return 0;
}

//add an fd to the circular queue
int connection_enqueue(connection_queue_t *queue, int connection_fd) {
    int result = 0; //for error checking
    if((result = pthread_mutex_lock(&queue->lock)) != 0){
    fprintf(stderr, "lock failed: %s\n", strerror(result));
        return -1;
    }
    

    while(queue->length >= queue->capacity && queue->shutdown != 1) { //queue is full, shutdown is not enabled
        //wait for the write conditional
        if((result = pthread_cond_wait(&queue->write_catchup, &queue->lock)) != 0) {
        fprintf(stderr, "wait failed: %s\n", strerror(result));
        return -1;
        }
    }
    //shutdown condition, make sure the mutex is given up when shutdown is entered
    if(queue->shutdown){
        if((result = pthread_mutex_unlock(&queue->lock)) != 0){
    	fprintf(stderr, "unlock failed: %s\n", strerror(result));
    }
        return -1;
    }
    
    //Do the enqueue stuff, add the fd to the client_fds[] field, increment the length and write idx
    queue->client_fds[queue->write_idx] = connection_fd;
    queue->length++;
    queue->write_idx++;
    //for wrap around, these lines make it a circular queue
    if(queue->write_idx == queue->capacity) { //full condition
        queue->write_idx = 0;
    }

    //signal the read conditional for the waiting threads
    if((result = pthread_cond_signal(&queue->read_catchup)) != 0){
    	fprintf(stderr, "signal read_catchup failed: %s\n", strerror(result));
    	return -1;
    }
    
    if((result = pthread_mutex_unlock(&queue->lock)) != 0){
    	fprintf(stderr, "unlock failed: %s\n", strerror(result));
    	return -1;
    }

    return 0;
}

int connection_dequeue(connection_queue_t *queue) {
   int result = 0; //for error checks
   if((result = pthread_mutex_lock(&queue->lock)) != 0){
    fprintf(stderr, "lock failed: %s\n", strerror(result));
        return -1;
    }

    while(queue->length == 0 && queue->shutdown != 1) { //queue is full or not in shutdown
        if((result = pthread_cond_wait(&queue->read_catchup, &queue->lock)) != 0) { //wait for catchup
        fprintf(stderr, "wait failed: %s\n", strerror(result));
        return -1;
        }
    }
    //unlock the mutex in the case of a shutdown condition
        if(queue->shutdown){
            if((result = pthread_mutex_unlock(&queue->lock)) != 0){
    		fprintf(stderr, "unlock failed: %s\n", strerror(result));
    		}
            return -1;
        }
        
        //decrement length and grab the proper file descriptor from the array
        queue->length--;
        int fd = queue->client_fds[queue->read_idx];
        queue->read_idx++;
        //for wrap around
        if(queue->read_idx == queue->capacity) {
            queue->read_idx = 0; //wraps the queue
        }
    
	//signal the write conditional so threads can continue after unlock
 	if((result = pthread_cond_signal(&queue->write_catchup)) != 0){
    		fprintf(stderr, "signal read_catchup failed: %s\n", strerror(result));
    		return -1;
    	}
    
    	if((result = pthread_mutex_unlock(&queue->lock)) != 0){
    		fprintf(stderr, "unlock failed: %s\n", strerror(result));
    		return -1;
    	}
    
    return fd;
}

//sets shutdown variable, and wakes up all threads so they can terminate
int connection_queue_shutdown(connection_queue_t *queue) {
int result = 0; //for error checking
	//lock to make sure the shutdown occurs correctly
    if((result = pthread_mutex_lock(&queue->lock)) != 0){
    	fprintf(stderr, "lock failed: %s\n", strerror(result));
        return -1;
    }
    //set shutdown condition
    queue->shutdown = 1;
    
    //broadcast to both of the conditionals in order to wake up the threads that could be waiting
     if((result = pthread_cond_broadcast(&queue->write_catchup)) != 0){
    	fprintf(stderr, "broadcast read_catchup failed: %s\n", strerror(result));
    	return -1;
    }
     if((result = pthread_cond_broadcast(&queue->read_catchup)) != 0){
    	fprintf(stderr, "broadcast read_catchup failed: %s\n", strerror(result));
    	return -1;
    }
    if((result = pthread_mutex_unlock(&queue->lock)) != 0){
    	fprintf(stderr, "unlock failed: %s\n", strerror(result));
    	return -1;
    }
    
    return 0;
}

//fress all of the conditionals and the lock
int connection_queue_free(connection_queue_t *queue) {
int result = 0; //for error checking
	//destroy the mutex and the conditionals with their respective syscalls
    if((result = pthread_mutex_destroy(&queue->lock)) != 0){
    fprintf(stderr, "mutex_destroy failed: %s\n", strerror(result));
    return -1;
    }
    if((result = pthread_cond_destroy(&queue->write_catchup)) != 0){
    fprintf(stderr, "cond_destroy for write_catchup failed: %s\n", strerror(result));
    return -1;
    }
    if((result = pthread_cond_destroy(&queue->read_catchup)) != 0){
    fprintf(stderr, "cond_destroy for read_catchup failed: %s\n", strerror(result));
    return -1;
    }
    return 0;
}
