#include "defines.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*!
 * @brief concat_path concatenates suffix to prefix into result
 * It checks if prefix ends by / and adds this token if necessary
 * It also checks that result will fit into PATH_SIZE length
 * @param result the result of the concatenation
 * @param prefix the first part of the resulting path
 * @param suffix the second part of the resulting path
 * @return a pointer to the resulting path, NULL when concatenation failed
 */
char *concat_path(char *result, char *prefix, char *suffix) {

    if (strlen(prefix) + strlen(suffix) + 2 > PATH_SIZE) {
        return NULL;
    }
    result = malloc((strlen(prefix) + strlen(suffix) + 2) * sizeof (char));

    strcpy(result, prefix);
    if (prefix[strlen(prefix) - 1] != '/') {
        strcat(result, "/");
    }
    strcat(result, suffix);

    return result;


}
