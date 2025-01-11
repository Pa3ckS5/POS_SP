#include "simulation.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h> // For random initialization

Simulation* create_simulation(SimulationConfig* config) {
    Simulation* sim = malloc(sizeof(Simulation));
    if (!sim) {
        perror("Chyba pri alokácii pamäte pre simuláciu");
        return NULL;
    }
    
    sim->config = config; // Priradenie konfigurácie

    // Alokácia pamäte pre mriežku a štatistiky
    sim->world = malloc(config->world_height * sizeof(int*));
    sim->success_count = malloc(config->world_height * sizeof(int*));
    sim->total_count = malloc(config->world_height * sizeof(int*));
    for (int i = 0; i < config->world_height; i++) {
        sim->world[i] = malloc(config->world_width * sizeof(int));
        sim->success_count[i] = malloc(config->world_width * sizeof(int));
        sim->total_count[i] = malloc(config->world_width * sizeof(int));
        memset(sim->world[i], 0, config->world_width * sizeof(int));
        memset(sim->success_count[i], 0, config->world_width * sizeof(int));
        memset(sim->total_count[i], 0, config->world_width * sizeof(int));
    }

    sim->current_step = 0; // Initialize step count
    sim->current_replication = 0;

    // Initialize goal coordinates (middle of the grid)
    sim->goal_x = config->world_width / 2;
    sim->goal_y = config->world_height / 2;

    // Initialize the walker at a random position
    initialize_walker(sim);

    // Generate obstacles if has_obstacles is true
    if (config->has_obstacles) {
        int total_cells = config->world_width * config->world_height;
        int obstacle_count = total_cells / 10; // 1/10 of total cells
        srand(time(NULL)); // Seed the random number generator

        for (int i = 0; i < obstacle_count; i++) {
            int x, y;
            do {
                x = rand() % config->world_width;
                y = rand() % config->world_height;
            } while ((x == sim->goal_x && y == sim->goal_y) || sim->world[y][x] == 1); // Ensure not on goal or already an obstacle
            sim->world[y][x] = 1; // Set as obstacle
        }
    }

    return sim;
}

void destroy_simulation(Simulation* sim) {
    for (int i = 0; i < sim->config->world_height; i++) {
        free(sim->world[i]);
        free(sim->success_count[i]);
        free(sim->total_count[i]);
    }
    free(sim->world);
    free(sim->success_count);
    free(sim->total_count);
    free(sim);
}

void initialize_walker(Simulation* sim) {
    srand(time(NULL)); // Seed the random number generator
    do {
        sim->walker_x = rand() % sim->config->world_width;
        sim->walker_y = rand() % sim->config->world_height;
    } while (sim->world[sim->walker_y][sim->walker_x] == 1); // Ensure walker is not on an obstacle
}

void reset_walker(Simulation* sim) {
    do {
        sim->walker_x = rand() % sim->config->world_width;
        sim->walker_y = rand() % sim->config->world_height;
    } while (sim->world[sim->walker_y][sim->walker_x] == 1); // Ensure walker is not on an obstacle
    sim->current_step = 0;
}

bool is_goal_reached(Simulation* sim) {
    return (sim->walker_x == sim->goal_x && sim->walker_y == sim->goal_y);
}

bool is_max_steps_reached(Simulation* sim) {
    return (sim->current_step >= sim->config->max_steps);
}

void random_step(Simulation* sim) {
    double random = (double)rand() / RAND_MAX;
    double cumulative_probability = 0.0;
    int new_x = sim->walker_x;
    int new_y = sim->walker_y;

    for (int i = 0; i < 4; i++) {
        cumulative_probability += sim->config->probabilities[i];
        if (random < cumulative_probability) {
            switch (i) {
                case 0: new_y--; break; // up
                case 1: new_y++; break; // down
                case 2: new_x--; break; // left
                case 3: new_x++; break; // right
            }
            break;
        }
    }

    // Wrap around the world and check for obstacles
    if (new_x < 0) new_x = sim->config->world_width - 1;
    if (new_x >= sim->config->world_width) new_x = 0;
    if (new_y < 0) new_y = sim->config->world_height - 1;
    if (new_y >= sim->config->world_height) new_y = 0;

    // Check if the new position is an obstacle
    if (sim->world[new_y][new_x] == 1) {
        // If it's an obstacle, don't move but still count the step
        sim->current_step++;
    } else {
        // Move the walker
        sim->walker_x = new_x;
        sim->walker_y = new_y;
        sim->current_step++;
    }
}

int random_walk(Simulation* sim, int start_x, int start_y) {
    int current_x = start_x;
    int current_y = start_y;
    int steps = 0;

    while (steps < sim->config->max_steps) {
        // Move the walker
        double random = (double)rand() / RAND_MAX;
        double cumulative_probability = 0.0;
        int new_x = current_x;
        int new_y = current_y;

        for (int i = 0; i < 4; i++) {
            cumulative_probability += sim->config->probabilities[i];
            if (random < cumulative_probability) {
                switch (i) {
                    case 0: new_y--; break; // up
                    case 1: new_y++; break; // down
                    case 2: new_x--; break; // left
                    case 3: new_x++; break; // right
                }
                break;
            }
        }

        // Wrap around the world and check for obstacles
        if (new_x < 0) new_x = sim->config->world_width - 1;
        if (new_x >= sim->config->world_width) new_x = 0;
        if (new_y < 0) new_y = sim->config->world_height - 1;
        if (new_y >= sim->config->world_height) new_y = 0;

        // Check if the new position is an obstacle
        if (sim->world[new_y][new_x] == 1) {
            // If it's an obstacle, don't move but still count the step
            steps++;
        } else {
            // Move the walker
            current_x = new_x;
            current_y = new_y;
            steps++;
        }

        // Check if the goal is reached
        if (current_x == sim->goal_x && current_y == sim->goal_y) {
            return steps; // Return the number of steps taken to reach the goal
        }
    }

    return -1; // Goal not reached within max_steps
}

void print_world_normal(Simulation* sim, char* buffer) {
    buffer[0] = '\0'; // Clear the buffer
    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            if (x == sim->goal_x && y == sim->goal_y) {
                sprintf(buffer + strlen(buffer), ", ");
            } else if (x == sim->walker_x && y == sim->walker_y) {
                sprintf(buffer + strlen(buffer), "O ");
            } else if (sim->world[y][x] == 1) {
                sprintf(buffer + strlen(buffer), "X ");
            } else {
                sprintf(buffer + strlen(buffer), ". ");
            }
        }
        sprintf(buffer + strlen(buffer), "\n");
    }

    // Pridanie informácií pod mriežku
    sprintf(buffer + strlen(buffer), "%dx%d - Steps: %d - Reps: %d\n",
            sim->config->world_width, sim->config->world_height, sim->config->max_steps, sim->config->total_replications);
    sprintf(buffer + strlen(buffer), "[%.2f, %.2f, %.2f, %.2f]\n",
            sim->config->probabilities[0], sim->config->probabilities[1], sim->config->probabilities[2], sim->config->probabilities[3]);

    // Celkový počet vykonaných replikácií
    int total_reps_done = 0;
    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            total_reps_done += sim->total_count[y][x];
        }
    }
    sprintf(buffer + strlen(buffer), "Total reps: %d / %d\n",
            total_reps_done, sim->config->world_width * sim->config->world_height * sim->config->total_replications);
}

void print_world_success_rate(Simulation* sim, char* buffer) {
    buffer[0] = '\0'; // Clear the buffer
    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            if (x == sim->goal_x && y == sim->goal_y) {
                sprintf(buffer + strlen(buffer), "-.-- "); // Cieľové políčko
            } else if (sim->world[y][x] == 1) {
                sprintf(buffer + strlen(buffer), "X.XX "); // Prekážka
            } else {
                double success_rate = (sim->total_count[y][x] == 0) ? 0.0 :
                                     (double)sim->success_count[y][x] / sim->total_count[y][x];
                sprintf(buffer + strlen(buffer), "%.2f ", success_rate); // Úspešnosť
            }
        }
        sprintf(buffer + strlen(buffer), "\n");
    }

    // Pridanie informácií pod mriežku
    sprintf(buffer + strlen(buffer), "%dx%d - Steps: %d - Reps: %d\n",
            sim->config->world_width, sim->config->world_height, sim->config->max_steps, sim->config->total_replications);
    sprintf(buffer + strlen(buffer), "[%.2f, %.2f, %.2f, %.2f]\n",
            sim->config->probabilities[0], sim->config->probabilities[1], sim->config->probabilities[2], sim->config->probabilities[3]);

    // Celkový počet vykonaných replikácií
    int total_reps_done = 0;
    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            total_reps_done += sim->total_count[y][x];
        }
    }
    sprintf(buffer + strlen(buffer), "Total reps: %d / %d\n",
            total_reps_done, sim->config->world_width * sim->config->world_height * sim->config->total_replications);
}

void print_result(Simulation* sim, char* buffer) {
    buffer[0] = '\0'; // Clear the buffer

    // Uloženie základných parametrov simulácie
    sprintf(buffer + strlen(buffer), "%d\n", sim->config->success_rate_mode ? 2 : 1); // Mód simulácie
    sprintf(buffer + strlen(buffer), "%d\n", sim->config->has_obstacles ? 2 : 1); // Typ sveta
    sprintf(buffer + strlen(buffer), "%d %d\n", sim->config->world_width, sim->config->world_height); // Veľkosť mriežky
    sprintf(buffer + strlen(buffer), "%d\n", sim->config->max_steps); // Maximálny počet krokov
    sprintf(buffer + strlen(buffer), "%d\n", sim->config->total_replications); // Počet replikácií
    sprintf(buffer + strlen(buffer), "%.2f %.2f %.2f %.2f\n", // Pravdepodobnosti
            sim->config->probabilities[0], sim->config->probabilities[1], sim->config->probabilities[2], sim->config->probabilities[3]);

    // Uloženie mriežky so štatistikami
    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            if (x == sim->goal_x && y == sim->goal_y) {
                sprintf(buffer + strlen(buffer), "9.99 "); // Cieľové políčko
            } else if (sim->world[y][x] == 1) {
                sprintf(buffer + strlen(buffer), "8.88 "); // Prekážka
            } else {
                double success_rate = (sim->total_count[y][x] == 0) ? 0.0 :
                                     (double)sim->success_count[y][x] / sim->total_count[y][x];
                sprintf(buffer + strlen(buffer), "%.2f ", success_rate); // Úspešnosť
            }
        }
        sprintf(buffer + strlen(buffer), "\n");
    }
}