#ifndef CLIENT_IO_H
#define CLIENT_IO_H

#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int int_input_check(int min, int max);
double double_input_check(double min, double max);
char* text_input_check();

bool save_simulation(SimulationConfig* config, const char* result);
bool load_simulation(const char* filename, SimulationConfig* config);

void input_simulation_config(SimulationConfig* config);
void update_config(SimulationConfig* config);

#endif // CLIENT_IO_H