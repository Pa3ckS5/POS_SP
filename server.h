#ifndef SERVER_H
#define SERVER_H

#include "socket.h"
#include "simulation.h"
#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define MAX_THREADS 3
#define MAX_CLIENTS 10

typedef struct {
    Simulation* sim;
    int start_row;
    int end_row;
    pthread_mutex_t* mutex;
    int* finished;
} ThreadData;

typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
    bool active; 
} ClientInfo;

typedef struct {
    ClientInfo clients[MAX_CLIENTS];
    pthread_t client_threads[MAX_CLIENTS];
    int client_count;
    pthread_mutex_t mutex;
    bool shutdown;
} Server;

void broadcast_to_clients(Server* server, MessageToClient* message);
void* fast_sim_function(void* arg);
void simulate_slow(Server* server, Simulation* sim, int client_socket);
void simulate_fast(Server* server, Simulation* sim, int client_socket);
void handle_client(Server* server, int client_socket);
void* client_handler(void* arg);

#endif // SERVER_H