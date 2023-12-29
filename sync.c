#include "sync.h"
#include <dirent.h>
#include <string.h>
#include "processes.h"
#include "utility.h"
#include "messages.h"
#include "file-properties.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <utime.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

/*!
 * @brief synchronize is the main function for synchronization
 * It will build the lists (source and destination), then make a third list with differences, and apply differences to the destination
 * It must adapt to the parallel or not operation of the program.
 * @param the_config is a pointer to the configuration
 * @param p_context is a pointer to the processes context
 */
//A FAIRE
void synchronize(configuration_t *the_config, process_context_t *p_context) {

    //Création des trois listes
    files_list_t *source_list = malloc(sizeof(files_list_t));
    source_list->head = NULL;
    source_list->tail = NULL;
    files_list_t *destination_list = malloc(sizeof(files_list_t));
    destination_list->head = NULL;
    destination_list->tail = NULL;
    files_list_t *differences_list = malloc(sizeof(files_list_t));
    differences_list->head = NULL;
    differences_list->tail = NULL;

    //Remplissage des listes
    make_files_list(source_list, the_config->source);
    make_files_list(destination_list, the_config->destination);

    //Création des variables de parcours
    files_list_entry_t *current_source = source_list->head;
    files_list_entry_t *current_destination = destination_list->head;
	
	//Si la taille de la liste source est plus grande que la taille de la liste destination
    if (current_destination == NULL) {
        //ajout des elements dans la liste des differences
        while (current_source != NULL) {
            add_entry_to_tail(differences_list,current_source);

            current_source = current_source->next;
        }
    }
    else{
	//Parcours des deux listes
	    while (current_source != NULL){

		current_destination = destination_list->head;

		while(current_destination != NULL) {
			printf("lalal");
			//S'il y a une difference, on l'ajoute à la liste des differences
			if (mismatch(current_source,current_destination, the_config->uses_md5)) {
				add_entry_to_tail(differences_list,current_source);
			}
			current_destination = current_destination->next;
		}
		
		
		current_source = current_source->next;
	    }

    }

    

    


    //Variable de parcours
    files_list_entry_t *current_difference = differences_list->head;

    //Parcours de la liste des differences
    while (current_difference != NULL) {
        //Copie des differences
	
        copy_entry_to_destination(current_difference,the_config);
        current_difference = current_difference->next;
    }


    //Vide de la memoire
    /*clear_files_list(source_list);
    clear_files_list(destination_list);
    clear_files_list(differences_list);*/

}

/*!
 * @brief mismatch tests if two files with the same name (one in source, one in destination) are equal
 * @param lhd a files list entry from the source
 * @param rhd a files list entry from the destination
 * @has_md5 a value to enable or disable MD5 sum check
 * @return true if both files are not equal, false else
 */

bool mismatch(files_list_entry_t *lhd, files_list_entry_t *rhd, bool has_md5) {
    //vérification de l'existence des noms
    if (strcmp(lhd->path_and_name, "") == 1 || strcmp(rhd->path_and_name, "") == 1) {
        return true;
    }

    char name1[100], name2[100];
    int place_letter = 0;
    //récupération du nom depuis le chemin du fichier
    for (int i = strlen(lhd->path_and_name) - 1; i != '/'; i--) {
        name1[place_letter] = lhd->path_and_name[i];
        place_letter++;
    }
    place_letter = 0;
    //récupération du nom depuis le chemin du fichier
    for (int j = strlen(rhd->path_and_name) - 1; j != '/'; j--) {
        name2[place_letter] = rhd->path_and_name[j];
        place_letter++;
    }
    //si has_md5 est utilisé et sinon
    if (!has_md5) {
        //comparaison md5
        if (strcmp(name1, name2) == 0) {
            return false;
        } else {
            return true;
        }
    } else {
        char md5_1[100], md5_2[100];
        //conversion md5 en chaîne de caractères
        for (int k = 0; lhd->md5sum[k] != '\0'; k++) {
            md5_1[k] = lhd->md5sum[k];
        }
        for (int l = 0; rhd->md5sum[l] != '\0'; l++) {
            md5_2[l] = rhd->md5sum[l];
        }
        //comparaison nom + md5
        if (strcmp(name1, name2) == 0 && strcmp(md5_1, md5_2) == 0) {
            return false;
        } else {
            return true;
        }
    }
}

/*!
 * @brief make_files_list buils a files list in no parallel mode
 * @param list is a pointer to the list that will be built
 * @param target_path is the path whose files to list
 */
void make_files_list(files_list_t *list, char *target_path) {

    //Créer la liste des path des fichiers
    make_list(list,target_path);

    //Variable de parcours
    files_list_entry_t *current = list->head;

    //Parcours de la liste
    while (current != NULL) {
        //Récupération si possible de toutes les informations du fichier

	printf("\nPath : %s\n", current->path_and_name);
        if (get_file_stats(current) == -1) {
            perror("Impossible de récupérer les informations du fichier a");
        }
        current = current->next;
    }
}

/*!
 * @brief make_files_lists_parallel makes both (src and dest) files list with parallel processing
 * @param src_list is a pointer to the source list to build
 * @param dst_list is a pointer to the destination list to build
 * @param the_config is a pointer to the program configuration
 * @param msg_queue is the id of the MQ used for communication
 */
void make_files_lists_parallel(files_list_t *src_list, files_list_t *dst_list, configuration_t *the_config, int msg_queue) {
}

/*!
 * @brief copy_entry_to_destination copies a file from the source to the destination
 * It keeps access modes and mtime (@see utimensat)
 * Pay attention to the path so that the prefixes are not repeated from the source to the destination
 * Use sendfile to copy the file, mkdir to create the directory
 */
void copy_entry_to_destination(files_list_entry_t *source_entry, configuration_t *the_config) {
     char *source_path = source_entry->path_and_name;
     char *destination_path = malloc(sizeof(the_config->destination)+
				sizeof(source_entry->path_and_name) - 
				sizeof(the_config->source));
char *destination_path_without_name = malloc(sizeof(the_config->destination)+
				sizeof(source_entry->path_and_name) - 
				sizeof(the_config->source));

	//Creation de la nouvelle chaine de caractere du nouveau path

	int i,j,k,l = -1;
	for( i = 0; i < strlen(the_config->destination); i++){
		destination_path[i] = the_config->destination[i];
	}

	for(j=strlen(the_config->source); j < strlen(source_entry->path_and_name); j++){
		destination_path[i] = source_entry->path_and_name[j];
		i++;
	}

	//Suppression du nom
	
	strcpy(destination_path_without_name,destination_path);

	for(k = 0; k < strlen(destination_path_without_name); k++){
		if(destination_path_without_name[k] == '/'){
			l = k;
		}
	}

	if(l!=-1){
		destination_path_without_name[l] = '\0';		
	}

	printf("%s",destination_path);
	printf("%s",destination_path_without_name);


	//Creation des dossiers nécessaires

    char * tempPath = malloc(sizeof(char) * (strlen(destination_path_without_name)+1));
    
    for(int i = 0; i < strlen(tempPath); i++){
        tempPath[i] = '\0';
        }
    int cpt = 0;
    for(int i = 0; i < strlen(destination_path_without_name); i++){
        
        tempPath[cpt] = destination_path_without_name[i];
        tempPath[cpt+1] = '\0';
        cpt++;
        
        if(destination_path_without_name[i] == '/'){
            
            if ( chdir( tempPath ) != 0 ) {
                printf("Fonctionne pas pour %s\n", tempPath);
                printf("Creation du dossier %s\n", tempPath);
		
		
		
                if ( mkdir( tempPath, 0755 ) != 0 ) {
                    printf("Impossible de créer le dossier %s : \n", tempPath );
                    return ;
                }
                
                if ( chdir( tempPath ) != 0 ) {
                    printf("Abandon");
                    return;
                }
                
                }
            else{
                printf("Fonctionne pour %s\n", tempPath);
            }
            

            tempPath[0] = '\0';
            
            cpt = 0;
            }
        
        }
    
    if ( chdir( tempPath ) != 0 ) {
        printf("Fonctionne pas pour %s\n", tempPath);
        printf("Creation du dossier %s\n", tempPath);
        if ( mkdir( tempPath, 0755 ) != 0 ) {
            printf("Impossible de créer le dossier %s : \n", tempPath );
            return ;
        }
        
        if ( chdir( tempPath ) != 0 ) {
            printf("Abandon");
            return;
        }
        }
    else{
        printf("Fonctionne pour %s\n", tempPath);
        }

	



     // Créer la structure stat pour obtenir des informations sur le fichier source
     struct stat source_stat;
     if (stat(source_path, &source_stat) == -1) {
         perror("Erreur lors de la récupération des informations sur le fichier source");
         exit(EXIT_FAILURE);
     }


     // Vérifier si le fichier source est un répertoire
     if (S_ISDIR(source_stat.st_mode)) {
         // Créer le répertoire de destination s'il n'existe pas
         if (mkdir(destination_path, source_stat.st_mode) == -1) {
             perror("Erreur lors de la création du répertoire de destination");
             exit(EXIT_FAILURE);
         }
     } else {
         // Ouvrir le fichier source en lecture
         int source_fd = open(source_path, O_RDONLY);
         if (source_fd == -1) {
             perror("Erreur lors de l'ouverture du fichier source");
             exit(EXIT_FAILURE);
         }

	//Il faut qu'on récupere le path relatif du fichier de la source pour le remplacé par "test.txt"
         int destination_fd = open(destination_path, O_WRONLY | O_CREAT | O_TRUNC, source_stat.st_mode);
         if (destination_fd == -1) {
             perror("Erreur lors de la création du fichier de destination");
             exit(EXIT_FAILURE);
         }

         // Utiliser sendfile pour copier le contenu du fichier source vers le fichier de destination
         off_t offset = 0;
         ssize_t bytes_sent = sendfile(destination_fd, source_fd, &offset, source_stat.st_size);
         if (bytes_sent == -1) {
             perror("Erreur lors de la copie du fichier");
             exit(EXIT_FAILURE);
         }

         // Fermer les descripteurs de fichier
         close(source_fd);
         close(destination_fd);
     }

     // Copier les attributs de temps du fichier source vers le fichier de destination
     struct timeval times[2];
     times[0].tv_sec = source_stat.st_atime;
     times[0].tv_usec = 0;
     times[1].tv_sec = source_stat.st_mtime;
     times[1].tv_usec = 0;

     if (utimes(destination_path, times) == -1) {
         perror("Erreur lors de la copie des attributs de temps");
         exit(EXIT_FAILURE);
     }
}

/*!
 * @brief make_list lists files in a location (it recurses in directories)
 * It doesn't get files properties, only a list of paths
 * This function is used by make_files_list and make_files_list_parallel
 * @param list is a pointer to the list that will be built
 * @param target is the target dir whose content must be listed
 */
void make_list(files_list_t *list, char *target) {

    DIR *dir = open_dir(target);

    struct dirent *dent;

    while ((dent = get_next_entry(dir)) != NULL) {
        //Si c'est un dossier on parcours le dossier de maniere recurcive
	
        if (dent->d_type == 4) {
            make_list(list, concat_path("", target, dent->d_name));
        } else if (dent->d_type == 8) { //Si c'est un fichier on l'ajoute à la liste            
		add_file_entry(list,concat_path("", target, dent->d_name));
        }
    }
    

    closedir(dir);
}

/*!
 * @brief open_dir opens a dir
 * @param path is the path to the dir
 * @return a pointer to a dir, NULL if it cannot be opened
 */
DIR *open_dir(char *path) {
    return opendir(path);
}

/*!
 * @brief get_next_entry returns the next entry in an already opened dir
 * @param dir is a pointer to the dir (as a result of opendir, @see open_dir)
 * @return a struct dirent pointer to the next relevant entry, NULL if none found (use it to stop iterating)
 * Relevant entries are all regular files and dir, except . and ..
 */
struct dirent *get_next_entry(DIR *dir) {
    struct dirent *dent;
    dent = readdir(dir);

    if(dent == NULL){
	return NULL;
    }	

    if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0){
	dent = get_next_entry(dir);
    }

    return dent;
}
