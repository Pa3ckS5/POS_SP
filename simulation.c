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
    
    sim->config = config; // priradenie konfiguracie
    if (config->fast_mode) {
        sim->mode = SUCCESS;
    } else {
        sim->mode = VISUAL;
    }
     
    // alokácia pamate pre mriežku a statistiky
    sim->world = malloc(config->world_height * sizeof(int*));
    sim->success_count = malloc(config->world_height * sizeof(int*));
    sim->total_count = malloc(config->world_height * sizeof(int*));
    sim->sum = malloc(config->world_height * sizeof(int*));
    
    for (int i = 0; i < config->world_height; i++) {
        sim->world[i] = malloc(config->world_width * sizeof(int));
        sim->success_count[i] = malloc(config->world_width * sizeof(int));
        sim->total_count[i] = malloc(config->world_width * sizeof(int));
        sim->sum[i] = malloc(config->world_width * sizeof(int));

        memset(sim->world[i], 0, config->world_width * sizeof(int));
        memset(sim->success_count[i], 0, config->world_width * sizeof(int));
        memset(sim->total_count[i], 0, config->world_width * sizeof(int));
        memset(sim->sum[i], 0, config->world_width * sizeof(int));
    }

    sim->current_step = 0;
    sim->current_replication = 0;

    sim->goal_x = config->world_width / 2;
    sim->goal_y = config->world_height / 2;

    sim->walker_x = 0;
    sim->walker_y = 0;

    // generovanie prekazok
    if (config->has_obstacles) {
        int total_cells = config->world_width * config->world_height;
        int obstacle_count = total_cells / 10; // 1/10 z policok
        srand(time(NULL)); // seed

        for (int i = 0; i < obstacle_count; i++) {
            int x, y;
            do {
                x = rand() % config->world_width;
                y = rand() % config->world_height;
            } while ((x == sim->goal_x && y == sim->goal_y) || sim->world[y][x] == 1); // overenie s cielom
            sim->world[y][x] = 1; // prekazka
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

    // overenie prechadzania mimo
    if (new_x < 0) new_x = sim->config->world_width - 1;
    if (new_x >= sim->config->world_width) new_x = 0;
    if (new_y < 0) new_y = sim->config->world_height - 1;
    if (new_y >= sim->config->world_height) new_y = 0;

    // overenie prekazky
    if (sim->world[new_y][new_x] == 1) {
        sim->current_step++;
    } else {
        // pohyb
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
        // pohyb
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

        /// overenie prechadzania mimo
        if (new_x < 0) new_x = sim->config->world_width - 1;
        if (new_x >= sim->config->world_width) new_x = 0;
        if (new_y < 0) new_y = sim->config->world_height - 1;
        if (new_y >= sim->config->world_height) new_y = 0;

        // overenie prekazky
        if (sim->world[new_y][new_x] == 1) {
            steps++;
        } else {
            // pohyb
            current_x = new_x;
            current_y = new_y;
            steps++;
        }

        // ciel
        if (current_x == sim->goal_x && current_y == sim->goal_y) {
            return steps; // pocet krokov
        }
    }

    return -1; // ciel nedosiahnuty
}

void change_mode(Simulation* sim){
    if (sim->config->fast_mode) {
        switch (sim->mode) {
            case SUCCESS: 
                sim->mode = AVERAGE;
                break;
            case AVERAGE: 
                sim->mode = SUCCESS;
                break;
            default:
                break;
        }

    } else {
        switch (sim->mode) {
            case VISUAL: 
                sim->mode = SUCCESS;
                break;
            case SUCCESS: 
                sim->mode = AVERAGE;
                break;
            case AVERAGE: 
                sim->mode = VISUAL;
                break;
            default:
                break;
        }
    }
}

void get_state_print(Simulation* sim, char* buffer){
    buffer[0] = '\0';

    if (sim->mode == VISUAL) {
        get_visual_print(sim, buffer);
    } else if (sim->mode == AVERAGE) {
        get_average_print(sim, buffer);
    } else {
        get_success_print(sim, buffer);
    }
    sprintf(buffer + strlen(buffer), "\n");

    get_statistic_print(sim, buffer);
    
}

void get_result_print(Simulation* sim, char* buffer) {
    buffer[0] = '\0';

    get_success_print(sim, buffer);
    sprintf(buffer + strlen(buffer), "\n");
    get_average_print(sim, buffer);
    sprintf(buffer + strlen(buffer), "\n");
    get_statistic_print(sim, buffer);

}

void get_result_for_save(Simulation* sim, char* buffer) {
    buffer[0] = '\0';

    get_success_print(sim, buffer);
    sprintf(buffer + strlen(buffer), "\n");
    get_average_print(sim, buffer);

}

void get_visual_print (Simulation* sim, char* buffer) {

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
}

void get_success_print(Simulation* sim, char* buffer) {
    
    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            if (x == sim->goal_x && y == sim->goal_y) {
                sprintf(buffer + strlen(buffer), "-.-- "); // Cieľové políčko
            } else if (sim->world[y][x] == 1) {
                sprintf(buffer + strlen(buffer), "X.XX "); // prekážka
            } else {
                double success_rate = (sim->total_count[y][x] == 0) ? 0.0 :
                                     (double)sim->success_count[y][x] / sim->total_count[y][x];
                sprintf(buffer + strlen(buffer), "%.2f ", success_rate); // uspešnosť
            }
        }
        sprintf(buffer + strlen(buffer), "\n");
    }
}

void get_average_print(Simulation* sim, char* buffer) {
    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            if (x == sim->goal_x && y == sim->goal_y) {
                sprintf(buffer + strlen(buffer), "--- "); // Cieľové políčko
            } else if (sim->world[y][x] == 1) {
                sprintf(buffer + strlen(buffer), "XXX "); // prekazka
            } else {
                double success_rate = (sim->success_count[y][x] == 0) ? 0.0 :
                                     (double)sim->sum[y][x] / sim->success_count[y][x];
                int rounded_rate = (int)(success_rate + 0.5); // Zaokrúhlenie na najbližšie celé číslo
                sprintf(buffer + strlen(buffer), "%3d ", rounded_rate); // priemer z uspesnych
            }
        }
        sprintf(buffer + strlen(buffer), "\n");
    }
}

void get_statistic_print(Simulation* sim, char* buffer) {
    
    sprintf(buffer + strlen(buffer), "%dx%d - Steps: %d - Reps: %d\n",
            sim->config->world_width, sim->config->world_height, sim->config->max_steps, sim->config->total_replications);
    sprintf(buffer + strlen(buffer), "[%.2f, %.2f, %.2f, %.2f]\n",
            sim->config->probabilities[0], sim->config->probabilities[1], sim->config->probabilities[2], sim->config->probabilities[3]);

    // replikacie
    int total_reps_done = 0;
    for (int y = 0; y < sim->config->world_height; y++) {
        for (int x = 0; x < sim->config->world_width; x++) {
            total_reps_done += sim->total_count[y][x];
        }
    }
    sprintf(buffer + strlen(buffer), "Total reps: %d / %d\n",
            total_reps_done, sim->config->world_width * sim->config->world_height * sim->config->total_replications);
}