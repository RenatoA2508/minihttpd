#define _GNU_SOURCE
#include "server.h"
#include "http.h"
#include "files.h"
#include "mime.h"
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
        char buffer_salida[LIMITE];
        int tipo_http = 0;
        int bytes_generados = 0;
        int cod_estado = 0;
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
            if(recibidos == LIMITE){fprintf(stderr, "La petición supera el tamaño del buffer\n"); cod_estado = 400; break;}
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
                    cod_estado = 400;
                    break;
                }
            }
            recibidos += rcv;
            
            tipo_http = analizar_solicitud_http(buffer, recibidos, ruta, sizeof ruta, &cerrar_conexion_salida);

            switch(tipo_http){
                case HTTP_OK:
                    printf("Ruta: %s\n %s", ruta, OK);
                    cod_estado = 200;
                    break;
                case HTTP_METODO_NO_PERMITIDO:
                    fprintf(stderr, NOT_ALLOWED);
                    cod_estado = 405;
                    break;
                case HTTP_INVALIDA:
                    fprintf(stderr, BAD_REQUEST);
                    cod_estado = 400;
                    break;
                case HTTP_INCOMPLETA:
                    
                    break;
            }

            if(tipo_http == HTTP_INCOMPLETA){
                if(recibidos >= LIMITE){fprintf(stderr, BAD_REQUEST); cod_estado = 400; break;}
                continue; /*Continua si la solicitud esta incompleta pero todavia hay espacio para recibirla*/
            }
            if(tipo_http == HTTP_OK){
                break;
            }
            break;
        }
        int fd = -1;
        char buffer_lectura[LIMITE];
        size_t tam_archivo = 0;
        int tipo_files = 0;
        char ruta_efectiva[1024];
        if(tipo_http == HTTP_OK){    
            tipo_files = solicitar_archivo(&fd, ruta, &tam_archivo, ruta_efectiva, sizeof ruta_efectiva);
            printf("Ruta: %s\n", ruta);
            switch(tipo_files){
                case 1: cod_estado = 200; break;
                case 0: cod_estado = 404; break;
                case -1: cod_estado = 403; break;
                case -2: cod_estado = 500; break;
            }
        }
        const char *mime = "text/plain";
        if(cod_estado == 200 && tipo_files == 1){
            mime = obtener_tipo_mime(ruta_efectiva);
        }
        if(cod_estado != 0){
            bytes_generados = generar_respuesta_http(cod_estado, mime, tam_archivo, 0, buffer_salida, sizeof buffer_salida); 
            if(bytes_generados <= 0){
                fprintf(stderr, "No se pudo generar la respuesta HTTP");
            }else{
                if(enviar(ns, buffer_salida, bytes_generados) != 1){
                    fprintf(stderr, "Error al enviar encabezados de respuesta HTTP");
                }else{
                    ssize_t leidos = 0;
                if(cod_estado == 200 && tipo_files == 1){   
                    while(1){
                        leidos = read(fd, buffer_lectura, sizeof buffer_lectura);
                        if(leidos == -1){
                            if(errno == EINTR){
                                fprintf(stderr, "Interrupción por señal\n"); 
                                continue;
                            }
                            perror("read"); 
                            break;
                        }
                        if(leidos == 0){break;}
                        if(enviar(ns, buffer_lectura, leidos) != 1){fprintf(stderr, "Error al enviar el cuerpo de respuesta HTTP"); break;}
                    }
                }
                    
                }
            }
        
            
        }
        close(ns);
        if(fd != -1){close(fd);}
    }
    close(s);
    return EXIT_SUCCESS;
}
