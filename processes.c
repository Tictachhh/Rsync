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
#include <signal.h>
#include <sys/wait.h> 

/*!
 * @brief prepare prepares (only when parallel is enabled) the processes used for the synchronization.
 * @param the_config is a pointer to the program configuration
 * @param p_context is a pointer to the program processes context
 * @return 0 if all went good, -1 else
 */
int prepare(configuration_t *the_config, process_context_t *p_context) {
    if (the_config->is_parallel) {
        p_context->processes_count = the_config->processes_count;
        p_context->main_process_pid = getpid();

        p_context->source_analyzers_pids = (pid_t *)malloc(sizeof(pid_t) * p_context->processes_count);
        p_context->destination_analyzers_pids = (pid_t *)malloc(sizeof(pid_t) * p_context->processes_count);

        key_t my_key = ftok("processes.c", 25);
        int msg_id = msgget(my_key, 0666 | IPC_CREAT);

        p_context->shared_key = my_key;
        p_context->message_queue_id = msg_id;

        if (p_context->source_analyzers_pids == NULL || p_context->destination_analyzers_pids == NULL) {
            perror("Memory allocation failed");
            return -1;
        }

        lister_configuration_t source_lister_config;
        source_lister_config.my_recipient_id = 1;//J'envoie à lui
        source_lister_config.my_receiver_id = 2;//Je reçois de lui
        source_lister_config.analyzers_count = the_config->processes_count;
        source_lister_config.mq_key = p_context->shared_key;

        lister_configuration_t destination_lister_config;
        destination_lister_config.my_recipient_id = 3;//J'envoie à lui
        destination_lister_config.my_receiver_id = 4;//Je reçois de lui
        destination_lister_config.analyzers_count = the_config->processes_count;
        destination_lister_config.mq_key = p_context->shared_key;


        p_context->source_lister_pid = make_process(p_context,lister_process_loop, (void *)&source_lister_config);
        p_context->destination_lister_pid = make_process(p_context,lister_process_loop, (void *)&destination_lister_config);


        // Creer un analyseur de source
        analyzer_configuration_t source_analyzer_config;
        source_analyzer_config.my_recipient_id = 1;
        source_analyzer_config.my_receiver_id = 2;
        source_analyzer_config.mq_key = p_context->shared_key;
        source_analyzer_config.use_md5 = the_config->uses_md5;

        for (int i = 0; i < p_context->processes_count; i++) {
            p_context->source_analyzers_pids[i] = make_process(p_context, analyzer_process_loop, (void *)&source_analyzer_config);
        }

        // Creer un analyseur de destination
        analyzer_configuration_t destination_analyzer_config;
        destination_analyzer_config.my_recipient_id = 3;
        destination_analyzer_config.my_receiver_id = 4;
        destination_analyzer_config.mq_key = p_context->shared_key;
        destination_analyzer_config.use_md5 = the_config->uses_md5;

        for (int i = 0; i < p_context->processes_count; i++) {
            p_context->destination_analyzers_pids[i] = make_process(p_context, analyzer_process_loop, (void *)&destination_analyzer_config);
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
        //_exit(0); // Fin du processus enfant
    } else {
        return pid; // Retourne le PID du processus enfant au parent
    }
}

/*!
 * @brief lister_process_loop is the lister process function (@see make_process)
 * @param parameters is a pointer to its parameters, to be cast to a lister_configuration_t
 */
void lister_process_loop(void *parameters) {
    lister_configuration_t *config = (lister_configuration_t *)parameters;

    int my_recipient_id = config->my_recipient_id;
    int my_receiver_id = config->my_receiver_id;
    int analyzers_count = config->analyzers_count;
    key_t mq_key = config->mq_key;

    // Example: Print information
    printf("Lister_process:\n");
    printf("my_recipient_id: %d\n", my_recipient_id);
    printf("my_receiver_id: %d\n", my_receiver_id);
    printf("analyzers_count: %d\n", analyzers_count);
    printf("mq_key: %d\n", mq_key);

}

/*!
 * @brief analyzer_process_loop is the analyzer process function
 * @param parameters is a pointer to its parameters, to be cast to an analyzer_configuration_t
 */
void analyzer_process_loop(void *parameters) {
    analyzer_configuration_t *config = (analyzer_configuration_t *)parameters;

    int my_recipient_id = config->my_recipient_id;
    int my_receiver_id = config->my_receiver_id;
    key_t mq_key = config->mq_key;
    bool use_md5 = config->use_md5;

    // Example: Print information
    printf("Analyzer_process: \n");
    printf("my_recipient_id: %d\n", my_recipient_id);
    printf("my_receiver_id: %d\n", my_receiver_id);
    printf("mq_key: %d\n", mq_key);
    printf("use_md5: %s\n", use_md5 ? "true" : "false");

}

/*!
 * @brief clean_processes cleans the processes by sending them a terminate command and waiting to the confirmation
 * @param the_config is a pointer to the program configuration
 * @param p_context is a pointer to the processes context
 */
void clean_processes(configuration_t *the_config, process_context_t *p_context) {
    // Do nothing if not parallel
    if (the_config->is_parallel) {
    // Send terminate
        for (int i = 0; i < p_context->processes_count; ++i) {
            kill(p_context->source_analyzers_pids[i], SIGTERM);
            kill(p_context->destination_analyzers_pids[i], SIGTERM);
        }

        // Wait for for responses
        for (int i = 0; i < p_context->processes_count; ++i) {
            int status;
            waitpid(p_context->source_analyzers_pids[i], &status, 0);
            if (WIFEXITED(status)) {
                printf("Process %d exited with status %d\n", p_context->source_analyzers_pids[i], WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Process %d terminated by signal %d\n", p_context->source_analyzers_pids[i], WTERMSIG(status));
            }

            waitpid(p_context->destination_analyzers_pids[i], &status, 0);
            if (WIFEXITED(status)) {
                printf("Process %d exited with status %d\n", p_context->destination_analyzers_pids[i], WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("Process %d terminated by signal %d\n", p_context->destination_analyzers_pids[i], WTERMSIG(status));
            }
        }

        // Free allocated memory
        free(p_context->source_analyzers_pids);
        free(p_context->destination_analyzers_pids);

        // Free the MQ
        if (msgctl(p_context->message_queue_id, IPC_RMID, NULL) == -1) {
            perror("Error deleting message queue");
        }
    }

    // Do nothing if not parallel
    // Send terminate
    // Wait for responses
    // Free allocated memory
    // Free the MQ
}


void request_element_details(int msg_queue, files_list_entry_t *entry, lister_configuration_t *cfg, int *current_analyzers){

}