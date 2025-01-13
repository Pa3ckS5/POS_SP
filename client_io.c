#include "client_io.h"

int int_input_check(int min, int max) {
    int input;
    int valid;

    do {
        valid = scanf("%d", &input);

        while (getchar() != '\n'); // vyčistenie buffera

        if (!valid) {
            printf("Neplatný vstup! Prosím, zadajte celé číslo.\n");
        } else if (input < min || input > max) {
            printf("Číslo mimo rozsahu! Skúste znovu:\n");
        }
    } while (!valid || input < min || input > max);

    return input;
}

double double_input_check(double min, double max) {
    double input;
    int valid;

    do {
        valid = scanf("%lf", &input);

        while (getchar() != '\n'); // vyčistenie buffera

        if (!valid) {
            printf("Neplatný vstup! Prosím, zadajte desatinné číslo:");
        } else if (input < min || input > max) {
            printf("Číslo mimo povoleného rozsahu! Skúste znova:");
        }
    } while (!valid || input < min || input > max);

    return input;
}

char* text_input_check() {
    static char input[MAX_FILENAME_LENGTH];

    do {
        fgets(input, sizeof(input), stdin);

        // Odstránenie nového riadku na konci (ak existuje)
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        if (strlen(input) == 0) {
            printf("Text nesmie byť prázdny!\n");
        } else if (strlen(input) > 30) {
            printf("Text je príliš dlhý! Prosím, skúste znova:\n");
        }
    } while (strlen(input) == 0 || strlen(input) > 30);

    return input;
}

bool save_simulation(SimulationConfig* config, const char* result) {
    // pridanie prípony .txt k názvu súboru
    char full_filename[MAX_FILENAME_LENGTH];
    snprintf(full_filename, sizeof(full_filename), "%s.txt", config->filename);

    FILE* file = fopen(full_filename, "w");
    if (!file) {
        perror("Chyba pri otváraní súboru na zápis");
        return false;
    }

    // Zápis konfigurácie
    fprintf(file, "%d\n", config->fast_mode ? 2 : 1);
    fprintf(file, "%d\n", config->has_obstacles ? 2 : 1);
    fprintf(file, "%d %d\n", config->world_width, config->world_height);
    fprintf(file, "%d\n", config->max_steps);     
    fprintf(file, "%d\n", config->total_replications);
    fprintf(file, "%.2f %.2f %.2f %.2f\n",
            config->probabilities[0], config->probabilities[1],
            config->probabilities[2], config->probabilities[3]);

    // Zápis výsledkov
    fprintf(file, "%s", result);

    fclose(file);
    printf("Simulácia bola úspešne uložená do súboru: %s\n\n", config->filename);
    
    return true;
}

bool load_simulation(const char* filename, SimulationConfig* config) {
    // pridanie prípony .txt k názvu súboru
    char full_filename[MAX_FILENAME_LENGTH];
    snprintf(full_filename, sizeof(full_filename), "%s.txt", filename);

    FILE* file = fopen(full_filename, "r");
    if (file == NULL) {
        perror("Chyba pri otváraní súboru na čítanie");
        return false;
    }

    // načítanie konfigurácie
    int mode, obstacles;
    fscanf(file, "%d", &mode);
    fscanf(file, "%d", &obstacles);
    fscanf(file, "%d", &config->world_width);
    fscanf(file, "%d", &config->world_height);
    fscanf(file, "%d", &config->max_steps);
    fscanf(file, "%d", &config->total_replications);
    fscanf(file, "%.2lf %.2lf %.2lf %.2lf",
        &config->probabilities[0], &config->probabilities[1],
        &config->probabilities[2], &config->probabilities[3]);

    if (mode == 2) {
        config->fast_mode = true;
    } else {
        config->fast_mode = false;
    }
    if (obstacles == 2) {
        config->has_obstacles = true;
    } else {
        config->has_obstacles = false;
    }

    // vypis konfiguracie
    printf("\nNačítaná konfigurácia simulácie:\n");
    printf("Mód simulácie: %s\n", config->fast_mode ? "zrýchlený" : "pomalý");
    printf("Typ sveta: %s\n", config->has_obstacles ? "áno" : "nie");
    printf("Šírka mriežky: %d\n", config->world_width);
    printf("Výška mriežky: %d\n", config->world_height);
    printf("Maximálny počet krokov: %d\n", config->max_steps);
    printf("Počet replikácií: %d\n", config->total_replications);
    printf("Pravdepodobnosti: %lf %lf %lf %lf\n", 
        config->probabilities[0], config->probabilities[1],
        config->probabilities[2], config->probabilities[3]);

    // výpis výsledkov
    char buffer[BUFFER_SIZE];
    printf("Výsledok simulácie:\n");
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }
    printf("Výsledok simulácie:\n");

    fclose(file);
    return true;
}

void input_simulation_config(SimulationConfig* config) {
    printf("Vyberte mód simulácie:\n");
    printf("1. Normálny mód\n");
    printf("2. Mód úspešnosti\n");
    int mode = int_input_check(1, 2);
    config->fast_mode = (mode == 2);

    printf("Vyberte typ sveta:\n");
    printf("1. Bez prekážok\n");
    printf("2. S prekážkami\n");
    int obstacles = int_input_check(1, 2);
    config->has_obstacles = (obstacles == 2);

    printf("Zadajte šírku mriežky (min 3, max 50): ");
    config->world_width = int_input_check(3, 50);

    printf("Zadajte výšku mriežky (min 3, max 50): ");
    config->world_height = int_input_check(3, 50);

    printf("Zadajte maximálny počet krokov (min 1, max 1000): ");
    config->max_steps = int_input_check(1, 1000);

    printf("Zadajte počet replikácií pre každé políčko (min 1, max 1000): ");
    config->total_replications = int_input_check(1, 1000);

    printf("Zadajte pravdepodobnosti pre pohyb smerom 1-hore, 2-dole, 3-vľavo, 4-vpravo (súčet musí byť 1.00):\n");
    double sum = 0.0;
    do {
        sum = 0.0;
        for (int i = 0; i < 4; i++) {
            printf("Pravdepodobnosť pre smer %d (min 0.0, max 1.0): ", i + 1);
            config->probabilities[i] = double_input_check(0.0, 1.0);
            sum += config->probabilities[i];
        }
        if (sum != 1.0) {
            printf("Súčet pravdepodobností musí byť 1.00! Skúste znova.\n");
        }
    } while (sum != 1.0);

    printf("Zadajte názov súboru pre uloženie výsledku simulácie: ");
    strncpy(config->filename, text_input_check(), sizeof(config->filename) - 1);
    config->filename[sizeof(config->filename) - 1] = '\0'; // ukončenia reťazca
}

void update_config(SimulationConfig* config) {
    printf("Aktuálny počet replikácií: %d\n", config->total_replications);
    printf("Zadajte nový počet replikácií (min 1, max 1000): ");
    config->total_replications = int_input_check(1, 1000);

    printf("Aktuálny názov súboru: %s\n", config->filename);
    printf("Zadajte nový názov súboru pre uloženie výsledku simulácie: ");
    strncpy(config->filename, text_input_check(), sizeof(config->filename) - 1);
    config->filename[sizeof(config->filename) - 1] = '\0'; //ukončenia reťazca

    printf("Konfigurácia bola úspešne aktualizovaná.\n");
}