#ifndef CLIENT_H
#define CLIENT_H

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
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>


int kbhit(void);
void* check_quit(void* arg);
bool is_port_available(int port);
void start_new_simulation(SimulationConfig* config);
void connect_to_existing_simulation();
void display_menu();

#endif // CLIENT_H