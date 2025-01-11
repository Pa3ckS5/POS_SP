#include "socket.h"
#include "shared.h"
#include "client_io.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>

//#define BUFFER_SIZE 15000

atomic_bool quit_requested = false; // Atomická premenná na kontrolu stlačenia 'q'

// Funkcia na kontrolu stlačenia klávesy 'q'
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

// Vlákno na kontrolu stlačenia klávesy 'q'
void* check_quit(void* arg) {
    while (!quit_requested) {
        if (kbhit()) {
            char ch = getchar();
            if (ch == 'q' || ch == 'Q') {
                quit_requested = true;
                break;
            }
        }
        usleep(100000); // Čakanie 100 ms
    }
    return NULL;
}

void display_menu() {
    printf("Vyberte možnosť:\n");
    printf("1. Spustiť novú simuláciu\n");
    printf("2. Pripojiť sa k existujúcej simulácii\n");
    printf("3. Načítať simuláciu zo súboru\n");
    printf("4. Ukončiť aplikáciu\n");
}

void start_new_simulation(SimulationConfig* config) {
    int port;
    printf("Zadajte port pre server: ");
    scanf("%d", &port);

    pid_t pid = fork();
    if (pid == 0) {
        // Child process - start server
        char port_str[10];
        sprintf(port_str, "%d", port);
        execl("./server", "server", port_str, NULL);
        perror("Chyba pri spúšťaní servera");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process - connect to server
        sleep(1); // Wait for server to start
        int sock = connect_to_server("127.0.0.1", port);
        if (sock < 0) {
            perror("Chyba pri pripájaní na server");
            return;
        }

        char buffer[BUFFER_SIZE] = {0};

        // Odoslanie konfigurácie serveru
        if (send(sock, config, sizeof(SimulationConfig), 0) < 0) {
            perror("Chyba pri odosielaní konfigurácie serveru");
            close(sock);
            return;
        }

        printf("Server pripravuje simuláciu...");

        // Prijatie a zobrazenie výsledkov simulácie
        MessageToClient message;
        while (1) {
            if (recv(sock, &message, sizeof(message), 0) <= 0) {
                perror("Chyba pri prijímaní dát zo servera");
                break;
            }

            system("clear");
            printf("%s\n", message.buffer);

            if (message.type == SIMULATION_OVER) {
                printf("Simulácia ukončená.\n");
                break;
            }
        }

        //ulozenie
        if (recv(sock, &message, sizeof(message), 0) <= 0) {
            perror("Chyba pri prijímaní dát pre ukladanie zo servera");
        } else {
            if (message.type == SIMULATION_RESULT) {
                if (!save_simulation(config, message.buffer)) {
                    printf("Nastal problém s ukladaním simulácie.\n\n");
                }
            }
        }
        
        close(sock);
        quit_requested = false;
    } else {
        perror("Chyba pri forkovaní");
    }
}

void connect_to_existing_simulation() {
    int port;
    printf("Zadajte port servera, ku ktorému sa chcete pripojiť: ");
    scanf("%d", &port);

    int sock = connect_to_server("127.0.0.1", port);
    if (sock < 0) {
        perror("Chyba pri pripájaní na server");
        return;
    }

    char buffer[BUFFER_SIZE] = {0};

    // Vytvorenie vlákna na kontrolu stlačenia 'q'
    pthread_t quit_thread;
    pthread_create(&quit_thread, NULL, check_quit, NULL);

    // Prijatie a zobrazenie výsledkov simulácie
    MessageToClient message;
    while (1) {
        if (quit_requested) {
            printf("Klient ukončuje spojenie...\n");
            break;
        }

        if (recv(sock, &message, sizeof(message), 0) <= 0) {
            perror("Chyba pri prijímaní dát zo servera");
            break;
        }

        system("clear");
        printf("%s\n", message.buffer);

        if (message.type == SIMULATION_OVER) {
            printf("Simulácia ukončená.\n");
            break;
        }
    }

    close(sock);
    pthread_join(quit_thread, NULL);
    quit_requested = false;
}

int main() {
    system("clear");
    printf("- RANDOM WALK APP -\n\n");

    while (1) {
        display_menu();
        int choice;
        SimulationConfig config;
        scanf("%d", &choice);

        if (choice == 1) {
            input_simulation_config(&config);
            //printf("DBG: config success\n");
            start_new_simulation(&config);
        } else if (choice == 2) {
            connect_to_existing_simulation();
        } else if (choice == 3) {
            char filename[31];
            printf("Zadajte názov súboru pre načítanie similácie: ");
            scanf("%s", &filename);
            if (load_simulation(filename, &config)) {
                printf("Chcete simuláciu znovu spustiť?\n1. Áno\n2. Nie ");
                int repeat;
                scanf("%d", &repeat);
                if(repeat == 1) {
                    update_config(&config);
                    start_new_simulation(&config);
                }
            } else {
                printf("Simuláciu sa nepodarilo načítať.\n");
            }
        } else if (choice == 4) {
            printf("Ukončujem aplikáciu...\n");
            break;
        } else {
            printf("Neplatná voľba. Skúste znova.\n");
        }
    }

    return 0;
}