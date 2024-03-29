#include "files-list.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "file-properties.h"

/*!
 * @brief clear_files_list clears a files list
 * @param list is a pointer to the list to be cleared
 * This function is provided, you don't need to implement nor modify it
 */
void clear_files_list(files_list_t *list) {
    while (list->head) {
        files_list_entry_t *tmp = list->head;
        list->head = tmp->next;
        free(tmp);
    }
}

/*!
 *  @brief add_file_entry adds a new file to the files list.
 *  It adds the file in an ordered manner (strcmp) and fills its properties
 *  by calling stat on the file.
 *  Il the file already exists, it does nothing and returns 0
 *  @param list the list to add the file entry into
 *  @param file_path the full path (from the root of the considered tree) of the file
 *  @return 0 if success, -1 else (out of memory)
 */
files_list_entry_t *add_file_entry(files_list_t *list, char *file_path) {
    if (list == NULL || file_path == NULL) {
        return NULL;
    }

    files_list_entry_t *existing_entry = find_entry_by_name(list, file_path, 0, 0);
    if (existing_entry != NULL) {
        return existing_entry;
    }

    files_list_entry_t *new_entry = malloc(sizeof(files_list_entry_t));

    if (new_entry == NULL) {
        return NULL;
    }


    strncpy(new_entry->path_and_name, file_path, sizeof(new_entry->path_and_name) - 1);
    new_entry->path_and_name[sizeof(new_entry->path_and_name) - 1] = '\0';
    new_entry->prev = NULL;
    new_entry->next = NULL;

    if (list->head == NULL || strcmp(file_path, list->head->path_and_name) < 0) {
        // Insérer au début de la liste (liste vide ou nouveau premier élément)
        new_entry->next = list->head;
        if (list->head != NULL) {
            list->head->prev = new_entry;
        }
        list->head = new_entry;
    } else {
        files_list_entry_t *current = list->head;
        while (current->next != NULL && strcmp(file_path, current->next->path_and_name) > 0) {
            current = current->next;
        }

        new_entry->next = current->next;
        new_entry->prev = current;

        if (current->next != NULL) {
            current->next->prev = new_entry;
        }

        current->next = new_entry;
    }

    return new_entry;
}
/*!
 * @brief add_entry_to_tail adds an entry directly to the tail of the list
 * It supposes that the entries are provided already ordered, e.g. when a lister process sends its list's
 * elements to the main process.
 * @param list is a pointer to the list to which to add the element
 * @param entry is a pointer to the entry to add. The list becomes owner of the entry.
 * @return 0 in case of success, -1 else
 */
int add_entry_to_tail(files_list_t *list, files_list_entry_t *entry) {
    if (list == NULL || entry == NULL) {
        return -1;
    }

    files_list_entry_t *new_el = malloc(sizeof(*entry));
    *new_el = *entry;

    if (list->head == NULL) {
        list->head = new_el;
        list->tail = new_el;
        new_el->next = NULL;
        new_el->prev = NULL;
        return 0;
    }

    list->tail->next = new_el;
    new_el->prev = list->tail;
    new_el->next = NULL;
    list->tail = new_el;

    return 0;
}

/*!
 *  @brief find_entry_by_name looks up for a file in a list
 *  The function uses the ordering of the entries to interrupt its search
 *  @param list the list to look into
 *  @param file_path the full path of the file to look for
 *  @param start_of_src the position of the name of the file in the source directory (removing the source path)
 *  @param start_of_dest the position of the name of the file in the destination dir (removing the dest path)
 *  @return a pointer to the element found, NULL if none were found.
 */
files_list_entry_t *find_entry_by_name(files_list_t *list, char *file_path, size_t start_of_src, size_t start_of_dest) {
    if (list == NULL || file_path == NULL) {
        return NULL;
    }

    files_list_entry_t *actuel = list->head;

    while (actuel != NULL) {
        if (strcmp(actuel->path_and_name + start_of_src, file_path + start_of_dest) == 0) {
            return actuel;
        }
        actuel = actuel->next;
    }

    return NULL;
}

/*!
 * @brief display_files_list displays a files list
 * @param list is the pointer to the list to be displayed
 * This function is already provided complete.
 */
void display_files_list(files_list_t *list) {
    if (!list)
        return;
    printf("\n----\n");
    for (files_list_entry_t *cursor=list->head; cursor!=NULL; cursor=cursor->next) {
        printf("%s\n", cursor->path_and_name);
    }
    printf("----\n");
}

/*!
 * @brief display_files_list_reversed displays a files list from the end to the beginning
 * @param list is the pointer to the list to be displayed
 * This function is already provided complete.
 */
void display_files_list_reversed(files_list_t *list) {
    if (!list)
        return;

    for (files_list_entry_t *cursor=list->tail; cursor!=NULL; cursor=cursor->prev) {
        printf("%s\n", cursor->path_and_name);
    }
}
