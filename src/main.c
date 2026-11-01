#define _GNU_SOURCE
#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define LIMITE 8192

int main(){

    int s = crear_socket_serv("8080");

    if(s == -1){fprintf(stderr, "No se logró crear el socket\n"); return EXIT_FAILURE;}
    puts("Servidor escuchando...");
    while(1){
        char buffer[LIMITE];
        ssize_t rcv = 0;
        size_t recibidos = 0;
        int ns = accept(s, NULL, NULL);
        if(ns == -1){
            if(errno == EINTR){
                fprintf(stderr, "Interrupción por señal\n"); 
                continue;
            }
            perror("accept"); 
            continue;
        }
        while(1){
            char *encontrado;
            if(recibidos == LIMITE){fprintf(stderr, "La petición supera el tamaño del buffer\n"); break;}
            rcv = recv(ns, buffer + recibidos, LIMITE - recibidos, 0);
            if(rcv == -1){perror("recv"); break;}
            if(rcv == 0){
                if(recibidos == 0){
                    fprintf(stderr, "El cliente ha cerrado la conexión sin enviar datos\n"); 
                    break;
                }else{
                    fprintf(stderr, "El cliente ha enviado una solicitud incompleta\n"); 
                    break;
                }
            }
            recibidos += rcv;
            encontrado = memmem(buffer, recibidos, "\r\n\r\n", 4);
            if(encontrado == NULL){continue;}
            size_t longitud_encabezado = encontrado - buffer + 4;
            if(longitud_encabezado > LIMITE){fprintf(stderr, "El encabezado supera el limite establecido\n"); break;}
            printf("%zu\n", longitud_encabezado);
            break;
        }
        close(ns);
    }
    close(s);
    return EXIT_SUCCESS;
}
