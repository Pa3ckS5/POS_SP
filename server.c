#include "server.h"

void broadcast_to_clients(Server* server, MessageToClient* message) {
    pthread_mutex_lock(&server->mutex);
    for (int i = 0; i < server->client_count; i++) {
        if (server->clients[i].active && send(server->clients[i].client_socket, message, sizeof(MessageToClient), 0) < 0) {
            perror("Chyba pri odosielaní správy klientovi");
            // Odstráňte klienta zo zoznamu, ak sa nepodarilo poslať správu
            close(server->clients[i].client_socket);
            server->clients[i].active = false; // Označte klienta ako neaktívneho
        }
    }
    pthread_mutex_unlock(&server->mutex);
}

void* fast_sim_function(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    Simulation* sim = data->sim;

    for (int y = data->start_row; y <= data->end_row; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            int success_count = 0;
            int sum = 0;

            for (int rep = 0; rep < sim->config->total_replications; rep++) {
                int steps = random_walk(sim, x, y);
                if (steps != -1) {
                    success_count++;
                    sum += steps;
                }
            }

            pthread_mutex_lock(data->mutex);
            sim->success_count[y][x] = success_count;
            sim->total_count[y][x] = sim->config->total_replications;
            sim->sum[y][x] = sum;
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

void simulate_slow(Server* server, Simulation* sim, int client_socket) {
    MessageToClient message;
    message.type = SIMULATION_STATE;
    char buffer[BUFFER_SIZE] = {0};

    // Nastavenie časového limitu pre soket
    struct timeval timeout;
    timeout.tv_sec = 0; // 0 sekúnd
    timeout.tv_usec = 100000; // 100 ms
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            for (int rep = 0; rep < sim->config->total_replications; rep++) {
                sim->walker_x = x;
                sim->walker_y = y;
                int steps = 0;

                while (!is_goal_reached(sim) && !is_max_steps_reached(sim)) {

                    // Vstup od klienta
                    char input;
                    int bytes_received = recv(client_socket, &input, sizeof(input), 0);
                    if (bytes_received > 0) {
                        switch (input) {
                            case 'q': 
                                //printf("Klient požiadal o ukončenie simulácie.\n");
                                break;
                            case 'm': 
                                //printf("Klient požiadal o zmenu režimu.\n");
                                change_mode(sim);
                                break;
                            case 'e': 
                                //printf("Klient požiadal o ukončenie spojenia.\n");
                                pthread_mutex_lock(&server->mutex);
                                server->shutdown = true;
                                pthread_mutex_unlock(&server->mutex);
                                goto end_simulation;
                            default:
                                //printf("Neznámy príkaz od klienta: %c\n", input);
                                break;
                        }
                    } else if (bytes_received == 0) {
                        // Klient sa odpojil
                        printf("Klient sa odpojil.\n");
                        goto end_simulation;
                    }

                    // Vystup pre klienta
                    random_step(sim);
                    get_state_print(sim, message.buffer);
                    broadcast_to_clients(server, &message);

                    steps++;
                    usleep(100000);
                }

                if (is_goal_reached(sim)) {
                    sim->success_count[y][x]++;
                    sim->sum[y][x] += steps;
                }
                sim->total_count[y][x]++;
            }
        }
    }

end_simulation:
    message.type = SIMULATION_OVER;
    get_result_print(sim, message.buffer);
    broadcast_to_clients(server, &message);

    message.type = SIMULATION_RESULT;
    get_result_for_save(sim, message.buffer);
    broadcast_to_clients(server, &message);
}

void simulate_fast(Server* server, Simulation* sim, int client_socket) {
    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];
    int finished[sim->config->world_height];

    for (int i = 0; i < MAX_THREADS; i++) {
        thread_data[i].sim = sim;
        thread_data[i].start_row = i * (sim->config->world_height / MAX_THREADS);
        thread_data[i].end_row = (i + 1) * (sim->config->world_height / MAX_THREADS) - 1;
        if (i == MAX_THREADS - 1) {
            thread_data[i].end_row = sim->config->world_height - 1;
        }
        thread_data[i].mutex = &server->mutex;
        thread_data[i].finished = finished;

        for (int y = thread_data[i].start_row; y <= thread_data[i].end_row; y++) {
            finished[y] = 0;
        }

        if (pthread_create(&threads[i], NULL, fast_sim_function, &thread_data[i]) != 0) {
            perror("Chyba pri vytváraní vlákna");
            exit(EXIT_FAILURE);
        }
    }

    // Nastavenie časového limitu pre soket
    struct timeval timeout;
    timeout.tv_sec = 0; // 0 sekúnd
    timeout.tv_usec = 100000; // 100 ms
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    MessageToClient message;
    message.type = SIMULATION_STATE;

    while (1) {
        // vstup od klienta
        char input;
        int bytes_received = recv(client_socket, &input, sizeof(input), 0);
        if (bytes_received > 0) {
            switch (input) {
                case 'q': 
                    //printf("Klient požiadal o ukončenie simulácie.\n");
                    break;
                case 'm': 
                    //printf("Klient požiadal o zmenu režimu.\n");
                    change_mode(sim);
                    break;
                case 'e': 
                    //printf("Klient požiadal o ukončenie spojenia.\n");
                    pthread_mutex_lock(&server->mutex);
                    server->shutdown = true;
                    pthread_mutex_unlock(&server->mutex);
                    goto end_simulation;
                default:
                    //printf("Neznámy príkaz od klienta: %c\n", input);
                    break;
            }
        } else if (bytes_received == 0) {
            // Klient sa odpojil
            printf("Klient sa odpojil.\n");
            goto end_simulation;
        }

        //vystup pre klienta
        pthread_mutex_lock(&server->mutex);
        get_state_print(sim, message.buffer);
        pthread_mutex_unlock(&server->mutex);
        broadcast_to_clients(server, &message);

        int all_done = 1;
        pthread_mutex_lock(&server->mutex);
        for (int y = 0; y < sim->config->world_height; y++) {
            if (!finished[y]) {
                all_done = 0;
                break;
            }
        }
        pthread_mutex_unlock(&server->mutex);

        if (all_done) {
            break;
        }

        usleep(100000);
    }

end_simulation:
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    message.type = SIMULATION_OVER;
    get_result_print(sim, message.buffer);
    broadcast_to_clients(server, &message);

    message.type = SIMULATION_RESULT;
    get_result_for_save(sim, message.buffer);
    broadcast_to_clients(server, &message);
}

void handle_client(Server* server, int client_socket) {
    char buffer[BUFFER_SIZE] = {0};

    // Vytvorenie konfigurácie simulácie
    SimulationConfig config;

    if (recv(client_socket, &config, sizeof(config), 0) <= 0) {
        //perror("Chyba pri prijímaní konfigurácie simulácie");
        close(client_socket);
        return;
    }

    // Vytvorenie simulácie
    Simulation* sim = create_simulation(&config);

    if (config.fast_mode) {
        simulate_fast(server, sim, client_socket);
    } else {
        simulate_slow(server, sim, client_socket);
    }

    destroy_simulation(sim);
    close(client_socket);
}

void* client_handler(void* arg) {
    Server* server = (Server*)arg;
    int client_socket = server->clients[server->client_count - 1].client_socket;
    struct sockaddr_in client_addr = server->clients[server->client_count - 1].client_addr;

    handle_client(server, client_socket);

    pthread_mutex_lock(&server->mutex);
    for (int i = 0; i < server->client_count; i++) {
        if (server->clients[i].client_socket == client_socket) {
            server->clients[i].active = false; // Označte klienta ako neaktívneho
            break;
        }
    }
    pthread_mutex_unlock(&server->mutex);

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

    Server server;
    server.client_count = 0;
    server.shutdown = false;
    pthread_mutex_init(&server.mutex, NULL);

    while (!server.shutdown) {
        int client_socket = passive_socket_wait_for_client(server_fd);
        if (client_socket < 0) {
            perror("Chyba pri prijímaní klienta");
            continue;
        }

        pthread_mutex_lock(&server.mutex);
        if (server.client_count < MAX_CLIENTS) {
            server.clients[server.client_count].client_socket = client_socket;
            getpeername(client_socket, (struct sockaddr*)&server.clients[server.client_count].client_addr, &(socklen_t){sizeof(struct sockaddr_in)});
            server.clients[server.client_count].active = true; // Označte klienta ako aktívneho
            server.client_count++;
            pthread_mutex_unlock(&server.mutex);

            pthread_t thread;
            if (pthread_create(&thread, NULL, client_handler, &server) != 0) {
                perror("Chyba pri vytváraní vlákna pre klienta");
                close(client_socket);
            } else {
                // Uloženie vlákna do poľa
                server.client_threads[server.client_count - 1] = thread;
            }
        } else {
            pthread_mutex_unlock(&server.mutex);
            close(client_socket);
            printf("Maximálny počet klientov dosiahnutý. Pripojenie zamietnuté.\n");
        }
    }

    // Počkanie na všetky klientovské vlákna
    pthread_mutex_lock(&server.mutex);
    for (int i = 0; i < server.client_count; i++) {
        if (server.clients[i].active) {
            pthread_join(server.client_threads[i], NULL);
        }
    }
    pthread_mutex_unlock(&server.mutex);

    passive_socket_destroy(server_fd);
    pthread_mutex_destroy(&server.mutex);
    return 0;
}