#include "file-properties.h"

#include <sys/stat.h>
#include <dirent.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "defines.h"
#include <fcntl.h>
#include <stdio.h>
#include "utility.h"

/*!
 * @brief get_file_stats gets all of the required information for a file (inc. directories)
 * @param the files list entry
 * You must get:
 * - for files:
 *   - mode (permissions)
 *   - mtime (in nanoseconds)
 *   - size
 *   - entry type (FICHIER)
 *   - MD5 sum
 * - for directories:
 *   - mode
 *   - entry type (DOSSIER)
 * @return -1 in case of error, 0 else
 */
int get_file_stats(files_list_entry_t *entry) {
    struct stat file_info;

    if(stat(entry->path_and_name, &file_info) == -1) {
        perror("Erreur d'obtention des stats");
        return -1;
    }

    mode_t mode = file_info.st_mode;

    long long mtime = file_info.st_mtime;

    off_t size = file_info.st_size;

    int entry_type;
    if (S_ISDIR(mode)) {
        entry_type = 1; //Pour un dossier
    } else if (S_ISDIR(mode)) {
        entry_type = 0; //Pour un fichier

        FILE *file = fopen(entry->path_and_name, "r");
        if (!file) {
            perror("Erreur lors de l'ouverture du fichier");
            return -1;
        }
        unsigned char md5sum[MD5_DIGEST_LENGTH];

        EVP_MD_CTX *md5_ctx = EVP_MD_CTX_new();
        if (!md5_ctx) {
            perror("Erreur lors de la création du contexte MD5");
            fclose(file);
            return -1;
        }

        EVP_DigestInit(md5_ctx, EVP_md5());

        size_t read_bytes;
        unsigned char buffer[4096];

        while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            EVP_DigestUpdate(md5_ctx, buffer, read_bytes);
        }

        fclose(file);

        EVP_DigestFinal(md5_ctx, md5sum, NULL);
        EVP_MD_CTX_free(md5_ctx);

        fclose(file);

        printf("MD5 Sum: ");
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
            printf("%02x", md5sum[i]);
        }
        printf("\n");
    } else{
        return -1;
    }

    printf("Mode: %o\n", mode);
    printf("Mtime: %lld\n", mtime);
    printf("Size: %ld\n", size);
    printf("Entry Type: %s\n", entry_type == 0 ? "FICHIER" : "DOSSIER");

    return 0;
}

/*!
 * @brief compute_file_md5 computes a file's MD5 sum
 * @param the pointer to the files list entry
 * @return -1 in case of error, 0 else
 * Use libcrypto functions from openssl/evp.h
 */

//Temp
/*
char *filename = argv[1];
    FILE *file = fopen(filename, "rb");

    if (!file) {
        printf("Cannot open file: %s\n", filename);
        return EXIT_FAILURE;
    }

    MD5_CTX mdContext;
    int bytes;
    unsigned char data[MAX_BUF_SIZE];
    unsigned char digest[MD5_DIGEST_LENGTH];

    MD5_Init(&mdContext);

    while ((bytes = fread(data, 1, MAX_BUF_SIZE, file)) != 0) {
        MD5_Update(&mdContext, data, bytes);
    }

    MD5_Final(digest, &mdContext);

    fclose(file);

    printf("MD5 (%s) = ", filename);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");



 */




int compute_file_md5(files_list_entry_t *entry) {
    //Ouvrir le fichier en mode binaire
    FILE *file = fopen(entry->path_and_name, "rb");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    //Créer un context md5
    EVP_MD_CTX *md5_ctx = EVP_MD_CTX_new();
    if (!md5_ctx) {
        perror("Error creating MD5 context");
        fclose(file);
        return -1;
    }

    //Obtenir la fonction de hachage MD5
    const EVP_MD *md = EVP_get_digestbyname("md5");
    if (!md) {
        perror("MD5 not supported");
        EVP_MD_CTX_free(md5_ctx);
        fclose(file);
        return -1;
    }

    //Initialiser le contexte MD5
    if (EVP_DigestInit_ex(md5_ctx, md, NULL) != 1) {
        perror("Error initializing MD5 digest");
        EVP_MD_CTX_free(md5_ctx);
        fclose(file);
        return -1;
    }

    //Lire le fichier par morceaux et mettre à jour le contexte MD5
    size_t read_bytes;
    unsigned char buffer[4096];

    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (EVP_DigestUpdate(md5_ctx, buffer, read_bytes) != 1) {
            perror("Error updating MD5 digest");
            EVP_MD_CTX_free(md5_ctx);
            fclose(file);
            return -1;
        }
    }

    //Finaliser le calcul du hachage MD5
    if (EVP_DigestFinal_ex(md5_ctx, entry->md5sum, NULL) != 1) {
        perror("Error finalizing MD5 digest");
        EVP_MD_CTX_free(md5_ctx);
        fclose(file);
        return -1;
    }

    //Libérer les ressources et fermer le fichier
    EVP_MD_CTX_free(md5_ctx);
    fclose(file);

    //success
    return 0;
}

/*!
 * @brief directory_exists tests the existence of a directory
 * @path_to_dir a string with the path to the directory
 * @return true if directory exists, false else
 */
bool directory_exists(char *path_to_dir) {
    DIR *dir = opendir(path_to_dir);

    if (dir) {
        closedir(dir);
        return true;
    } else {
        return false;
    }
}

/*!
 * @brief is_directory_writable tests if a directory is writable
 * @param path_to_dir the path to the directory to test
 * @return true if dir is writable, false else
 * Hint: try to open a file in write mode in the target directory.
 */
bool is_directory_writable(char *path_to_dir) {
    //Création d'un fichier temporaire pour effectuer les tests
    const char *test_file_name = "test.tmp";

    char test_file_path[256];
    snprintf(test_file_path, sizeof(test_file_path), "%s/%s", path_to_dir, test_file_name);

    // Tente d'ouvrir le fichier en écriture
    FILE *test_file = fopen(test_file_path, "w");

    if (test_file) {
        // Si l'ouverture réussit, ferme le fichier et le supprime
        fclose(test_file);
        remove(test_file_path);
        return true;
    } else {
        return false;
    }
}
