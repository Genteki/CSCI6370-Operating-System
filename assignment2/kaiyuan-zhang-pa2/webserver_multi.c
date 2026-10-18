// local includes
#include <unistd.h>
#include <semaphore.h>
// template includes
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "webserver.h"

#define MAX_REQUEST 100
int port, numThread;
sem_t sem_empty, sem_full;
pthread_mutex_t mutex;
typedef struct RequestQueue {
	int buffer[MAX_REQUEST];
	int head;
	int tail;
} RequestQueue;

void emplace(RequestQueue *q, int fd) {
	q->buffer[q->tail] = fd;
	q->tail = (q->tail + 1) % MAX_REQUEST;
}
int pop(RequestQueue *q) {
	int r = q->buffer[q->head];
	q->head = (q->head + 1) % MAX_REQUEST;
	return r;
}

RequestQueue rq;

void *listener(void* arg) {
	// `void* arg` for meeting -std=C23
	int r;
	struct sockaddr_in sin;
	struct sockaddr_in peer;
	int peer_len = sizeof(peer);
	int sock;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	r = bind(sock, (struct sockaddr *) &sin, sizeof(sin));
	if(r < 0) {
		perror("Error binding socket:");
		return NULL;
	}

	r = listen(sock, 5);
	if(r < 0) {
		perror("Error listening socket:");
		return NULL;
	}

	printf("HTTP server listening on port %d\n", port);
	while (1)
	{
		int s;
		s = accept(sock, NULL, NULL);
		if (s < 0) break;

		sem_wait(&sem_empty);  // if sem_empty > 0 sem_empty--; else wait
		pthread_mutex_lock(&mutex);
		emplace(&rq, s);
		pthread_mutex_unlock(&mutex);
		sem_post(&sem_full);
		//process(s);
	}
	close(sock);
	return NULL;
}

void *worker(void* arg) { 
	// `void* arg` for meeting -std=C23
	int fd;
	while (1) {
		sem_wait(&sem_full);  // if sem_full > 0 sem_full--; else wait
		pthread_mutex_lock(&mutex);
		fd = pop(&rq);
		pthread_mutex_unlock(&mutex);
		sem_post(&sem_empty);
		process(fd);
	}
}

void thread_control() {	
	/* ----- */
	pthread_t tid_listener;
	pthread_t tid_pool[numThread];
	pthread_create(&tid_listener, NULL, listener, NULL);
	for (int i = 0; i < numThread; ++i) {
		pthread_create(&tid_pool[i], NULL, worker, NULL);
	}
	pthread_join(tid_listener, NULL);
	for (int i = 0; i < numThread; ++i)  pthread_join(tid_pool[i], NULL);
}

int main(int argc, char *argv[])
{
	if(argc != 3 || atoi(argv[1]) < 2000 || atoi(argv[1]) > 50000)
	{
		fprintf(stderr, "./webserver_multi PORT(2001 ~ 49999) #_of_threads\n");
		return 0;
	}

	port = atoi(argv[1]);
	numThread = atoi(argv[2]);
	rq.head = 0; rq.tail = 0;

	// numThread should be in [1, 100]
	if (numThread < 1) {fprintf(stderr, "# of threads must >= 1\n"); numThread = 1; return 0;}
	if (numThread > MAX_REQUEST) {fprintf(stderr, "# of threads must <= %d\n", MAX_REQUEST); return 0;}

	// sem & mutex init
	sem_init(&sem_empty, 0, MAX_REQUEST);
	sem_init(&sem_full, 0, 0);
	pthread_mutex_init(&mutex, NULL);
	
	// create listener + #numThread worker thread pool
	thread_control();

	// desctuction
	sem_destroy(&sem_empty);
	sem_destroy(&sem_full);
	pthread_mutex_destroy(&mutex);
	return 0;
}

