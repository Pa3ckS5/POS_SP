#ifndef SIMULATION_H
#define SIMULATION_H

#include "shared.h"
#include <stdbool.h>

typedef struct {
    SimulationConfig* config; // Smerník na konfiguráciu simulácie
    int** world; // 0 = empty, 1 = obstacle
    int walker_x, walker_y;
    int goal_x, goal_y; // Cieľové súradnice
    int current_step; // Track the current step count
    int current_replication;
    int** success_count; // Track successful simulations for each tile
    int** total_count;   // Track total simulations for each tile
} Simulation;

Simulation* create_simulation(SimulationConfig* config);
void destroy_simulation(Simulation* sim);
void random_step(Simulation* sim);
int random_walk(Simulation* sim, int start_x, int start_y);
void print_result(Simulation* sim, char* buffer);
void print_world_normal(Simulation* sim, char* buffer);
void print_world_success_rate(Simulation* sim, char* buffer);
void initialize_walker(Simulation* sim);
bool is_goal_reached(Simulation* sim);
bool is_max_steps_reached(Simulation* sim);
void reset_walker(Simulation* sim);
void final_result(Simulation* sim, char* buffer);

#endif // SIMULATION_H