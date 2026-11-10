#define _GNU_SOURCE
#include "server.h"
#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define LIMITE 8192
/*Temporal*/
#define BAD_REQUEST "400 Bad Request\n"
#define OK "200 Ok\n"
#define FORBIDDEN "403 Forbidden\n"
#define NOT_FOUND "404 Not Found\n"
#define NOT_ALLOWED "405 Method Not Allowed\n"
#define SRV_ERR "500 Internal Server Error\n"

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
        int cerrar_conexion_salida = 1;
        char ruta[LIMITE_URI+1];
        while(1){
            if(recibidos == LIMITE){fprintf(stderr, "La petición supera el tamaño del buffer\n"); break;}
            rcv = recv(ns, buffer + recibidos, LIMITE - recibidos, 0);
            if(rcv == -1){
                if(errno == EINTR){
                    fprintf(stderr, "Interrupción por señal\n"); 
                    continue;
                }
                perror("recv"); 
                break;
            }
            if(rcv == 0){
                if(recibidos == 0){
                    fprintf(stderr, "El cliente ha cerrado la conexión sin enviar datos\n"); 
                    break;
                }else{
                    fprintf(stderr, BAD_REQUEST); 
                    break;
                }
            }
            recibidos += rcv;
            
            int tipo_http = analizar_solicitud_http(buffer, recibidos, ruta, sizeof ruta, &cerrar_conexion_salida);

            switch(tipo_http){
                case HTTP_OK:
                    printf("Ruta: %s\n %s", ruta, OK);
                    break;
                case HTTP_METODO_NO_PERMITIDO:
                    fprintf(stderr, NOT_ALLOWED);
                    break;
                case HTTP_INVALIDA:
                    fprintf(stderr, BAD_REQUEST);
                    break;
                case HTTP_INCOMPLETA:
                    
                    break;
            }

            if(tipo_http == HTTP_INCOMPLETA){
                if(recibidos >= LIMITE){fprintf(stderr, BAD_REQUEST); break;}
                continue;
            }
            if(tipo_http == HTTP_OK){
                break;
            }
            break;
        }
        close(ns);
    }
    close(s);
    return EXIT_SUCCESS;
}
