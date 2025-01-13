#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>

#define BUFFER_SIZE 30000
#define MAX_FILENAME_LENGTH 31 //30 znakov + ukoncenie

typedef enum {
    VISUAL, // normalny mod
    SUCCESS, // statisticky mod
    AVERAGE // statisticky mod
} SimulationMode;

typedef struct {
    int world_width;
    int world_height;
    double probabilities[4];
    int total_replications;
    int max_steps;
    bool fast_mode;
    bool has_obstacles;
    char filename[MAX_FILENAME_LENGTH];
} SimulationConfig;

typedef enum {
    SIMULATION_CONFIG,
    SIMULATION_STATE,
    SIMULATION_OVER,
    SIMULATION_RESULT,
    SIMULATION_END, //stop simulation
    SIMULATION_MODE, //change mode
    SERVER_CREATED,
    SERVER_FULL,
    SERVER_SHUTDOWN
} MessageType;

typedef struct {
    MessageType type;
    char buffer[BUFFER_SIZE];  // Optional: Additional message data
} MessageToClient;

#endif // SHARED_H