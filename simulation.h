#ifndef SIMULATION_H
#define SIMULATION_H

#include "shared.h"
#include <stdbool.h>

typedef struct {
    SimulationConfig* config;
    SimulationMode mode;
    int** world; // 1 = prekazka
    int** success_count;
    int** total_count;
    int** sum;

    int walker_x, walker_y;
    int goal_x, goal_y;
    int current_step;
    int current_replication;
} Simulation;

Simulation* create_simulation(SimulationConfig* config);
void destroy_simulation(Simulation* sim);

void random_step(Simulation* sim);
int random_walk(Simulation* sim, int start_x, int start_y);

bool is_goal_reached(Simulation* sim);
bool is_max_steps_reached(Simulation* sim);
void change_mode(Simulation* sim);

void get_state_print(Simulation* sim, char* buffer);
void get_result_print(Simulation* sim, char* buffer);
void get_result_for_save(Simulation* sim, char* buffer);

void get_visual_print(Simulation* sim, char* buffer);
void get_success_print(Simulation* sim, char* buffer);
void get_average_print(Simulation* sim, char* buffer);
void get_statistic_print(Simulation* sim, char* buffer);

#endif // SIMULATION_H