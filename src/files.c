#define _DEFAULT_SOURCE
#include "files.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

int solicitar_archivo(int *fd, const char *ruta, size_t *tam_archivo, char *ruta_efectiva, size_t tam_ruta_efectiva){
    *fd = -1;
    *tam_archivo = 0;
    char directorio_permitido[PATH_MAX];
    if(realpath("www", directorio_permitido)==NULL){
        perror("realpath");
        return -2;
    }
    char ruta_candidata[PATH_MAX];
    int bytes_ruta_candidata = 0;

    if(strcmp(ruta, "/") == 0){
        bytes_ruta_candidata = snprintf(ruta_candidata, PATH_MAX, "%s", "www/index.html");
    }else{
        bytes_ruta_candidata = snprintf(ruta_candidata, PATH_MAX, "%s%s", "www", ruta);
    }
    if(bytes_ruta_candidata < 0 || bytes_ruta_candidata >= PATH_MAX){
        return -2;
    }
    
    
    char canon_archivo[PATH_MAX];
    if(realpath(ruta_candidata, canon_archivo)==NULL){
        if(errno == ENOENT){return 0;}
        if(errno == EACCES){return -1;}
        perror("realpath");
        return -2;
    }
    size_t len_dir_permitido = strlen(directorio_permitido);
    if(strncmp(directorio_permitido, canon_archivo, len_dir_permitido) != 0 || canon_archivo[len_dir_permitido] != '/'){return -1;}

    *fd = open(canon_archivo, O_RDONLY);
    if(*fd == -1){
        if(errno == ENOENT){return 0;}
        if(errno == EACCES){return -1;}
        perror("open"); 
        return -2;
    }

    struct stat buf;

    if(fstat(*fd, &buf) == -1){perror("fstat"); close(*fd); *fd = -1; return -2;}

    if(!S_ISREG(buf.st_mode)){close(*fd); *fd = -1; return -1;}

    if(buf.st_size < 0 || (uintmax_t)buf.st_size > (uintmax_t)SIZE_MAX){close(*fd); *fd = -1; return -2;}

    *tam_archivo = buf.st_size;
    int bytes_ruta_efectiva = snprintf(ruta_efectiva, tam_ruta_efectiva, "%s", canon_archivo);
    if(bytes_ruta_efectiva < 0 || (size_t) bytes_ruta_efectiva >= tam_ruta_efectiva){
        close(*fd); 
        *fd = -1;
        return -2;
    }
    return 1;
}

