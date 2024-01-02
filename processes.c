#include "processes.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <stdio.h>
#include "messages.h"
#include "file-properties.h"
#include "sync.h"
#include <string.h>
#include <errno.h>

/*!
 * @brief prepare prepares (only when parallel is enabled) the processes used for the synchronization.
 * @param the_config is a pointer to the program configuration
 * @param p_context is a pointer to the program processes context
 * @return 0 if all went good, -1 else
 */
int prepare(configuration_t *the_config, process_context_t *p_context) {
    if (the_config->is_parallel) {
        p_context->processes_count = the_config->processes_count;

        p_context->source_analyzers_pids = (pid_t *)malloc(sizeof(pid_t) * p_context->processes_count);
        p_context->destination_analyzers_pids = (pid_t *)malloc(sizeof(pid_t) * p_context->processes_count);

        if (p_context->source_analyzers_pids == NULL || p_context->destination_analyzers_pids == NULL) {
            perror("Memory allocation failed");
            return -1;
        }

        // Creer un analyseur de source
        analyzer_configuration_t source_analyzer_config;
        source_analyzer_config.my_recipient_id = 1;
        source_analyzer_config.my_receiver_id = 2;
        source_analyzer_config.mq_key = 1234;
        source_analyzer_config.use_md5 = the_config->uses_md5;

        for (int i = 0; i < p_context->processes_count; ++i) {
            make_process(p_context, analyzer_process_loop, (void *)&source_analyzer_config);
        }

        // Creer un analyseur de destination
        analyzer_configuration_t destination_analyzer_config;
        destination_analyzer_config.my_recipient_id = 3;
        destination_analyzer_config.my_receiver_id = 4;
        destination_analyzer_config.mq_key = 5678;
        destination_analyzer_config.use_md5 = the_config->uses_md5;

        for (int i = 0; i < p_context->processes_count; ++i) {
            make_process(p_context, analyzer_process_loop, (void *)&destination_analyzer_config);
        }

        return 0; // Success
    }

    return 0;
}

/*!
 * @brief make_process creates a process and returns its PID to the parent
 * @param p_context is a pointer to the processes context
 * @param func is the function executed by the new process
 * @param parameters is a pointer to the parameters of func
 * @return the PID of the child process (it never returns in the child process)
 */
int make_process(process_context_t *p_context, process_loop_t func, void *parameters) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {
        // Code exécuté par le processus enfant
        func(parameters); // Appel de la fonction avec les paramètres spécifiés
        _exit(0); // Fin du processus enfant
    } else {
        return pid; // Retourne le PID du processus enfant au parent
    }
}

/*!
 * @brief lister_process_loop is the lister process function (@see make_process)
 * @param parameters is a pointer to its parameters, to be cast to a lister_configuration_t
 */
void lister_process_loop(void *parameters) {
}

/*!
 * @brief analyzer_process_loop is the analyzer process function
 * @param parameters is a pointer to its parameters, to be cast to an analyzer_configuration_t
 */
void analyzer_process_loop(void *parameters) {
}

/*!
 * @brief clean_processes cleans the processes by sending them a terminate command and waiting to the confirmation
 * @param the_config is a pointer to the program configuration
 * @param p_context is a pointer to the processes context
 */
void clean_processes(configuration_t *the_config, process_context_t *p_context) {
    // Do nothing if not parallel
    // Send terminate
    // Wait for responses
    // Free allocated memory
    // Free the MQ
}
