#include "configuration.h"
#include <stddef.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

typedef enum {DATE_SIZE_ONLY, NO_PARALLEL} long_opt_values;

/*!
 * @brief function display_help displays a brief manual for the program usage
 * @param my_name is the name of the binary file
 * This function is provided with its code, you don't have to implement nor modify it.
 */
void display_help(char *my_name) {
    printf("%s [options] source_dir destination_dir\n", my_name);
    printf("Options: \t-n <processes count>\tnumber of processes for file calculations\n");
    printf("         \t-h display help (this text)\n");
    printf("         \t--date_size_only disables MD5 calculation for files\n");
    printf("         \t--no-parallel disables parallel computing (cancels values of option -n)\n");
}

/*!
 * @brief init_configuration initializes the configuration with default values
 * @param the_config is a pointer to the configuration to be initialized
 */
void init_configuration(configuration_t *the_config) {
    the_config->source[0] = '\0';
    the_config->destination[0] = '\0';
    the_config->processes_count = 1;
    the_config->is_parallel = true;
    the_config->uses_md5 = true;
    the_config->verbose = false;
    the_config->dry_run = false;

    // Vérification des paramètres
    if (the_config->processes_count < 1) {
        the_config->processes_count = 1; // Au moins 1 processus
    }

    // Par défaut, MD5 et parallélisme sont actifs
    the_config->uses_md5 = true;
    the_config->is_parallel = true;
}

/*!
 * @brief set_configuration updates a configuration based on options and parameters passed to the program CLI
 * @param the_config is a pointer to the configuration to update
 * @param argc is the number of arguments to be processed
 * @param argv is an array of strings with the program parameters
 * @return -1 if configuration cannot succeed, 0 when ok
 */
int set_configuration(configuration_t *the_config, int argc, char *argv[]) {

    //Assez d'arguement ?
    if (argc <= 2) {
        return -1;
    }


    int opt = 0;
    struct option my_opts[] = {
            {.name = "date-size-only", .has_arg = 0, .flag = 0, .val = 'a'},
            {.name = "n", .has_arg = 1, .flag = 0, .val = 'b'},
            {.name = "no-parallel", .has_arg = 0, .flag = 0, .val = 'c'},
            {.name = "v", .has_arg = 0, .flag = 0, .val = 'd'},
            {.name = "dry-run", .has_arg = 0, .flag = 0, .val = 'e'},
            {.name = 0, .has_arg = 0, .flag = 0, .val = 0},
    };

    while ((opt = getopt_long(argc, argv, "", my_opts, NULL)) != -1) {
        switch (opt) {
            case 'a':
                the_config->uses_md5 = false;
                break;

            case 'b':
                if (optarg) {
                    the_config->processes_count = atoi(optarg);
                }
                break;

            case 'c':
                the_config->is_parallel = false;
                break;

            case 'd':
                the_config->verbose = true;
                break;

            case 'e':
                the_config->dry_run = true;
                break;
        }
    }

    // Vérification des arguments restants
    if (optind + 1 >= argc) {
        fprintf(stderr, "Erreur: Il manque des arguments.\n");
        return -1;
    }

    // Vérification des tailles des chaînes de caractères
    if (strlen(argv[optind]) >= 1024 || strlen(argv[optind + 1]) >= 1024) {
        fprintf(stderr, "Erreur: Les chemins sources et de destination doivent avoir une taille inférieure à 1024 caractères.\n");
        return -1;
    }

    // Copie des chemins source et destination
    strcpy(the_config->source, argv[optind]);
    strcpy(the_config->destination, argv[optind + 1]);

    return 0;

}
