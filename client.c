#include "client.h"

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

void* kb_pressed_function(void* arg) {
    atomic_char* key_pressed = (atomic_char*)arg;

    while (1) {
        if (kbhit()) {
            char pushed_ch = getchar();
            atomic_store(key_pressed, pushed_ch);
            if (pushed_ch == 'q' || pushed_ch == 'Q') {
                break;
            }
        }

        char ch = atomic_load(key_pressed);
        if (ch == 'q' || ch == 'Q') {
            break;
        }

        usleep(100000);  // 100 ms
    }
    return NULL;
}

bool is_port_available(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Chyba pri vytváraní soketu");
        return false;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    // Pokus o bind na port
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return false;
    }

    close(sock);
    return true;
}

void start_new_simulation(SimulationConfig* config) {
    //PORT
    int port;
    while (1) {
        printf("Zadajte port pre server (2000-5000, alebo 0 pre ukončenie): ");
        scanf("%d", &port);

        // Ukončenie, ak používateľ zadá 0
        if (port == 0) {
            printf("Ukončujem...\n");
            return;
        }

        if (port < 2000 || port > 5000) {
            printf("Port musí byť v rozsahu 2000 až 5000.\n");
            continue;
        }

        // Kontrola dostupnosti portu
        if (is_port_available(port)) {
            break;  // Port je voľný, pokračujeme
        } else {
            printf("Port %d nie je voľný. Skúste iný port.\n", port);
        }
    }

    //SERVER
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
        atomic_char key_pressed = '\0';
        pthread_t quit_thread;
        pthread_create(&quit_thread, NULL, kb_pressed_function, &key_pressed);

        MessageToClient message;
        while (1) {
            //vystup pre server
            char ch = atomic_load(&key_pressed);
            if (ch == 'm' || ch == 'M') {
                if (send(sock, &ch, sizeof(ch), 0) < 0) {
                    perror("Chyba pri odosielaní znaku");
                }
                atomic_store(&key_pressed, '\0');
            } else if (ch == 'e' || ch == 'E') {
            if (send(sock, &ch, sizeof(ch), 0) < 0) {
                perror("Chyba pri odosielaní znaku");
            }
            atomic_store(&key_pressed, '\0');
        }

            //vstup od servera
            if (recv(sock, &message, sizeof(message), 0) <= 0) {
                perror("Chyba pri prijímaní dát zo servera");
                break;
            }

            system("clear");
            printf("%s\n", message.buffer);

            if (message.type == SIMULATION_OVER) {
                atomic_store(&key_pressed, 'q');
                printf("Simulácia ukončená.\n\n");
                break;
            }
            
            usleep(100000);
        }

        pthread_join(quit_thread, NULL);

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
    } else {
        perror("Chyba pri forkovaní");
    }
}

void connect_to_existing_simulation() {
    int port;
    int sock;

    while (1) {
        printf("Zadajte port servera, ku ktorému sa chcete pripojiť (alebo 0 pre ukončenie): ");
        scanf("%d", &port);

        // Ukončenie, ak používateľ zadal 0
        if (port == 0) {
            printf("Ukončujem...\n\n");
            return;
        }

        // Pokus o pripojenie na server
        sock = connect_to_server("127.0.0.1", port);
        if (sock < 0) {
            perror("Chyba pri pripájaní na server. Skúste znova.");
        } else {
            break; // Úspešné pripojenie, opustíme cyklus
        }
    }

    char buffer[BUFFER_SIZE] = {0};

    // Vytvorenie vlákna na kontrolu stlačenia kláves
    atomic_char key_pressed = '\0';
    
    pthread_t quit_thread;
    pthread_create(&quit_thread, NULL, kb_pressed_function, &key_pressed);

    // Prijatie a zobrazenie výsledkov simulácie
    MessageToClient message;
    while (1) {
        //vystup pre server
        char ch = atomic_load(&key_pressed);
        if (ch == 'q' || ch == 'Q') {
            printf("Klient ukončuje spojenie ...\n\n");
            break;
        } else if (ch == 'm' || ch == 'M') {
            if (send(sock, &ch, sizeof(ch), 0) < 0) {
                perror("Chyba pri odosielaní znaku");
            }
            atomic_store(&key_pressed, '\0');
        }

        //vstup od servera
        if (recv(sock, &message, sizeof(message), 0) <= 0) {
            perror("Chyba pri prijímaní dát zo servera");
            break;
        }

        system("clear");
        printf("%s\n", message.buffer);

        if (message.type == SIMULATION_OVER) {
            atomic_store(&key_pressed, 'q');
            printf("Simulácia ukončená.\n\n");
            break;
        }
        usleep(100000);
    }

    pthread_join(quit_thread, NULL);
    close(sock);
}

void display_menu() {
    printf("Vyberte možnosť:\n");
    printf("1. Spustiť novú simuláciu\n");
    printf("2. Pripojiť sa k existujúcej simulácii\n");
    printf("3. Načítať simuláciu zo súboru\n");
    printf("4. Ukončiť aplikáciu\n");
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
            start_new_simulation(&config);
        } else if (choice == 2) {
            connect_to_existing_simulation();
        } else if (choice == 3) {
            char filename[31];
            printf("Zadajte názov súboru pre načítanie simulácie: ");
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
        } else if (choice == 10) {
            config.world_width = 5;
            config.world_height = 5;
            config.probabilities[0] = 0.2;
            config.probabilities[1] = 0.2;
            config.probabilities[2] = 0.3;
            config.probabilities[3] = 0.3;
            config.total_replications = 2;
            config.max_steps = 5;
            config.fast_mode = false;
            config.has_obstacles = true;
            strcpy(config.filename, "test_10");
            start_new_simulation(&config);
        }  else if (choice == 11) {
            config.world_width = 30;
            config.world_height = 30;
            config.probabilities[0] = 0.2;
            config.probabilities[1] = 0.2;
            config.probabilities[2] = 0.3;
            config.probabilities[3] = 0.3;
            config.total_replications = 300;
            config.max_steps = 300;
            config.fast_mode = true;
            config.has_obstacles = true;
            strcpy(config.filename, "test_11");
            start_new_simulation(&config);
        } else {
            printf("Neplatná voľba. Skúste znova.\n");
        }
    }

    return 0;
}