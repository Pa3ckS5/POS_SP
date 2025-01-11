#include "client_io.h"

// Funkcia na kontrolu celočíselného vstupu
int int_input_check(int min, int max) {
    int input;
    int valid;

    do {
        valid = scanf("%d", &input);

        while (getchar() != '\n'); // Vyčistenie buffera

        if (!valid) {
            printf("Neplatný vstup! Prosím, zadajte celé číslo.\n");
        } else if (input < min || input > max) {
            printf("Číslo mimo rozsahu! Prosím, skúste znova.\n");
        }
    } while (!valid || input < min || input > max);

    return input;
}

// Funkcia na kontrolu desatinného vstupu
double double_input_check(double min, double max) {
    double input;
    int valid;

    do {
        valid = scanf("%lf", &input);

        while (getchar() != '\n'); // Vyčistenie buffera

        if (!valid) {
            printf("Neplatný vstup! Prosím, zadajte desatinné číslo.\n");
        } else if (input < min || input > max) {
            printf("Číslo mimo povoleného rozsahu! Prosím, skúste znova.\n");
        }
    } while (!valid || input < min || input > max);

    return input;
}

// Funkcia na kontrolu textového vstupu
char* text_input_check() {
    static char input[31]; // Statický buffer pre reťazec s maximálnou dĺžkou 30 znakov

    do {
        printf("Zadajte text (max 30 znakov, nesmie byť prázdny): ");
        fgets(input, sizeof(input), stdin);

        // Odstránenie nového riadku na konci (ak existuje)
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        if (strlen(input) == 0) {
            printf("Text nesmie byť prázdny!\n");
        } else if (strlen(input) > 30) {
            printf("Text je príliš dlhý! Prosím, skúste znova.\n");
        }
    } while (strlen(input) == 0 || strlen(input) > 30);

    return input;
}

// Funkcia na uloženie simulácie do súboru
bool save_simulation(SimulationConfig* config, const char* result) {
    FILE* file = fopen(config->filename, "w");
    if (!file) {
        perror("Chyba pri otváraní súboru na zápis");
        return false;
    }

    // Zápis konfigurácie
    fprintf(file, "%d\n", config->success_rate_mode ? 2 : 1); // Mód simulácie
    fprintf(file, "%d\n", config->has_obstacles ? 2 : 1);     // Typ sveta
    fprintf(file, "%d %d\n", config->world_width, config->world_height); // Veľkosť mriežky
    fprintf(file, "%d\n", config->max_steps);                 // Maximálny počet krokov
    fprintf(file, "%d\n", config->total_replications);        // Počet replikácií
    fprintf(file, "%.2f %.2f %.2f %.2f\n",                   // Pravdepodobnosti
            config->probabilities[0], config->probabilities[1],
            config->probabilities[2], config->probabilities[3]);

    // Zápis výsledkov
    fprintf(file, "%s", result);

    fclose(file);
    printf("Simulácia bola úspešne uložená do súboru: %s\n", config->filename);
    
    return true;
}

bool load_simulation(const char* filename, SimulationConfig* config) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Chyba pri otváraní súboru na čítanie");
        return false;
    }

    // Načítanie konfigurácie
    int mode, world_type;
    fscanf(file, "Mód simulácie: %d\n", &mode);
    fscanf(file, "Typ sveta: %d\n", &world_type);
    fscanf(file, "Šírka mriežky: %d\n", &config->world_width);
    fscanf(file, "Výška mriežky: %d\n", &config->world_height);
    fscanf(file, "Maximálny počet krokov: %d\n", &config->max_steps);
    fscanf(file, "Počet replikácií: %d\n", &config->total_replications);
    fscanf(file, "Pravdepodobnosti: %lf %lf %lf %lf\n",
           &config->probabilities[0], &config->probabilities[1],
           &config->probabilities[2], &config->probabilities[3]);

    config->success_rate_mode = (mode == 2);
    config->has_obstacles = (world_type == 2);

    // Načítanie výsledkov
    char buffer[BUFFER_SIZE];
    printf("\nNačítané výsledky simulácie:\n");
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }

    fclose(file);
    return true;
}

void input_simulation_config(SimulationConfig* config) {
    // Výber módu simulácie
    printf("Vyberte mód simulácie:\n");
    printf("1. Normálny mód\n");
    printf("2. Mód úspešnosti\n");
    int mode = int_input_check(1, 2);
    config->success_rate_mode = (mode == 2);

    // Výber typu sveta
    printf("Vyberte typ sveta:\n");
    printf("1. Bez prekážok\n");
    printf("2. S prekážkami\n");
    int obstacles = int_input_check(1, 2);
    config->has_obstacles = (obstacles == 2);

    // Vstupné parametre
    printf("Zadajte šírku mriežky (min 5, max 100): ");
    config->world_width = int_input_check(5, 100);

    printf("Zadajte výšku mriežky (min 5, max 100): ");
    config->world_height = int_input_check(5, 100);

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

    // Názov súboru pre uloženie výsledku
    printf("Zadajte názov súboru pre uloženie výsledku simulácie: ");
    strncpy(config->filename, text_input_check(), sizeof(config->filename) - 1);
    config->filename[sizeof(config->filename) - 1] = '\0'; // Zabezpečenie ukončenia reťazca
}

void update_config(SimulationConfig* config) {
    printf("Aktuálny počet replikácií: %d\n", config->total_replications);
    printf("Zadajte nový počet replikácií (min 1, max 1000): ");
    config->total_replications = int_input_check(1, 1000);

    printf("Aktuálny názov súboru: %s\n", config->filename);
    printf("Zadajte nový názov súboru pre uloženie výsledku simulácie: ");
    strncpy(config->filename, text_input_check(), sizeof(config->filename) - 1);
    config->filename[sizeof(config->filename) - 1] = '\0'; // Zabezpečenie ukončenia reťazca

    printf("Konfigurácia bola úspešne aktualizovaná.\n");
}