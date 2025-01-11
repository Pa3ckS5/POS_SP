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

#define MAX_THREADS 8
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
} ClientInfo;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
ClientInfo clients[MAX_CLIENTS];
int client_count = 0;

void broadcast_to_clients(MessageToClient* message) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        if (send(clients[i].client_socket, message, sizeof(MessageToClient), 0) < 0) {
            perror("Chyba pri odosielaní správy klientovi");
            // Odstráňte klienta zo zoznamu, ak sa nepodarilo poslať správu
            close(clients[i].client_socket);
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            i--; // Upravte index, pretože sme odstránili klienta
        }
    }
    pthread_mutex_unlock(&mutex);
}

void* success_rate_thread_function(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    Simulation* sim = data->sim;

    for (int y = data->start_row; y <= data->end_row; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            int success_count = 0;

            for (int rep = 0; rep < sim->config->total_replications; rep++) {
                int steps = random_walk(sim, x, y);
                if (steps != -1) {
                    success_count++;
                }
            }

            pthread_mutex_lock(data->mutex);
            sim->success_count[y][x] = success_count;
            sim->total_count[y][x] = sim->config->total_replications;
            pthread_mutex_unlock(data->mutex);
        }
    }

    pthread_mutex_lock(data->mutex);
    for (int y = data->start_row; y <= data->end_row; y++) {
        data->finished[y] = 1;
    }
    pthread_mutex_unlock(data->mutex);

    pthread_exit(NULL);
}

void normal_mode_simulation(Simulation* sim) {
    MessageToClient message;
    message.type = SIMULATION_STATE;
    char buffer[BUFFER_SIZE] = {0};

    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            sim->success_count[y][x] = 0;
            sim->total_count[y][x] = 0;
        }
    }

    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            for (int rep = 0; rep < sim->config->total_replications; rep++) {
                sim->walker_x = x;
                sim->walker_y = y;
                sim->current_step = 0;

                while (!is_goal_reached(sim) && !is_max_steps_reached(sim)) {
                    random_step(sim);
                    print_world_normal(sim, message.buffer);
                    broadcast_to_clients(&message);
                    usleep(100000);
                }

                if (is_goal_reached(sim)) {
                    sim->success_count[y][x]++;
                }
                sim->total_count[y][x]++;
            }
        }
    }

    message.type = SIMULATION_OVER;
    print_world_success_rate(sim, message.buffer);
    broadcast_to_clients(&message);

    message.type = SIMULATION_RESULT;
    print_result(sim, message.buffer);
    broadcast_to_clients(&message);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};

    // Vytvorenie konfigurácie simulácie
    SimulationConfig config;

    if (recv(client_socket, &config, sizeof(config), 0) <= 0) {
        perror("Chyba pri prijímaní konfigurácie simulácie");
        close(client_socket);
        return;
    }

    // Vytvorenie simulácie
    Simulation* sim = create_simulation(&config);
    printf(sim->config->success_rate_mode ? "srm: yes" : "srm: no");
    printf(sim->config->success_rate_mode ? "ho: yes" : "ho: no");
    printf(config.success_rate_mode ? "srm: yes" : "srm: no");
    printf(config.success_rate_mode ? "ho: yes" : "ho: no");
    sleep(3);

    if (config.success_rate_mode) {
        pthread_t threads[MAX_THREADS];
        ThreadData thread_data[MAX_THREADS];
        int finished[config.world_height];

        for (int i = 0; i < MAX_THREADS; i++) {
            thread_data[i].sim = sim;
            thread_data[i].start_row = i * (config.world_height / MAX_THREADS);
            thread_data[i].end_row = (i + 1) * (config.world_height / MAX_THREADS) - 1;
            if (i == MAX_THREADS - 1) {
                thread_data[i].end_row = config.world_height - 1;
            }
            thread_data[i].mutex = &mutex;
            thread_data[i].finished = finished;

            for (int y = thread_data[i].start_row; y <= thread_data[i].end_row; y++) {
                finished[y] = 0;
            }

            if (pthread_create(&threads[i], NULL, success_rate_thread_function, &thread_data[i]) != 0) {
                perror("Chyba pri vytváraní vlákna");
                exit(EXIT_FAILURE);
            }
        }

        MessageToClient message;
        message.type = SIMULATION_STATE;

        while (1) {
            usleep(500000);
            pthread_mutex_lock(&mutex);
            print_world_success_rate(sim, message.buffer);
            pthread_mutex_unlock(&mutex);
            broadcast_to_clients(&message);

            int all_done = 1;
            pthread_mutex_lock(&mutex);
            for (int y = 0; y < config.world_height; y++) {
                if (!finished[y]) {
                    all_done = 0;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            if (all_done) {
                break;
            }
        }

        for (int i = 0; i < MAX_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }

        message.type = SIMULATION_OVER;
        print_world_success_rate(sim, message.buffer);
        broadcast_to_clients(&message);

        message.type = SIMULATION_RESULT;
        print_result(sim, message.buffer);
        broadcast_to_clients(&message);
    } else {
        normal_mode_simulation(sim);
    }

    destroy_simulation(sim);
    close(client_socket);
}

void* client_handler(void* arg) {
    int client_socket = *(int*)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_socket, (struct sockaddr*)&client_addr, &addr_len);

    pthread_mutex_lock(&mutex);
    clients[client_count].client_socket = client_socket;
    clients[client_count].client_addr = client_addr;
    client_count++;
    pthread_mutex_unlock(&mutex);

    handle_client(client_socket);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].client_socket == client_socket) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    close(client_socket);
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Použitie: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int server_fd = passive_socket_init(port);
    if (server_fd < 0) {
        perror("Chyba pri vytváraní pasívneho soketu");
        exit(EXIT_FAILURE);
    }

    printf("Server počúva na porte %d...\n", port);

    while (1) {
        int client_socket = passive_socket_wait_for_client(server_fd);
        if (client_socket < 0) {
            perror("Chyba pri prijímaní klienta");
            continue;
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, &client_socket) != 0) {
            perror("Chyba pri vytváraní vlákna pre klienta");
            close(client_socket);
        } else {
            pthread_detach(thread); // Odpojte vlákno, aby sa automaticky ukončilo po skončení
        }
    }

    passive_socket_destroy(server_fd);
    return 0;
}