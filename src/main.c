#define _GNU_SOURCE
#include "server.h"
#include "http.h"
#include "files.h"
#include "mime.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
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

#define MAX_EVENTS 64
#define ENVIANDO 0
#define RECIBIENDO 1
struct cliente{
    int ns;
    char buffer[LIMITE];
    size_t recibidos;
    int fase;
    int cod_estado;
    int cerrar_conexion_salida;
    char buffer_salida[LIMITE];
    int bytes_generados;
    size_t bytes_enviados;
    int fd;
    char buffer_lectura[LIMITE];
    size_t bytes_leidos;
    size_t bytes_archivo_enviados;
};

void liberar_recursos(int ep, struct cliente *clt){
    if(epoll_ctl(ep, EPOLL_CTL_DEL, clt->ns, NULL) == -1){perror("epoll_ctl");}
    close(clt->ns);
    if(clt->fd != -1){close(clt->fd);}
    free(clt);
}

int main(){

    int s = crear_socket_serv("8080");
    if(s == -1){fprintf(stderr, "No se logró crear el socket\n"); return EXIT_FAILURE;}

    int flags_s = fcntl(s, F_GETFL);
    if(flags_s == -1){perror("fcntl"); close(s); return EXIT_FAILURE;}
    if(fcntl(s, F_SETFL, flags_s | O_NONBLOCK) == -1){perror("fcntl"); close(s); return EXIT_FAILURE;}

    int ep = epoll_create1(EPOLL_CLOEXEC);
    if(ep == -1){perror("epoll_create1"); close(s); return EXIT_FAILURE;}

    struct epoll_event ep_ev;
    ep_ev.events = EPOLLIN;
    ep_ev.data.ptr = NULL;
    if(epoll_ctl(ep, EPOLL_CTL_ADD, s, &ep_ev) == -1){perror("epoll_ctl"); close(s); close(ep);return EXIT_FAILURE;}

    struct epoll_event events[MAX_EVENTS];

    puts("Servidor escuchando...");
    while(1){
        int ep_w = epoll_wait(ep, events, MAX_EVENTS, -1);
        if(ep_w == -1){
            if(errno == EINTR){
                continue;
            }
            perror("epoll_wait");
            break;
        }

        for(int i=0; i<ep_w; i++){
            if(events[i].data.ptr == NULL){
                printf("Conexion pendiente\n");
                while(1){
                    int ns = accept(s, NULL, NULL);
                    if(ns == -1){
                        if(errno == EINTR){continue;}
                        if(errno == EAGAIN || errno == EWOULDBLOCK){break;}
                        perror("accept");
                        break;
                    }
                    int flags_ns = fcntl(ns, F_GETFL);
                    if(flags_ns == -1){perror("fcntl"); close(ns); continue;}
                    if(fcntl(ns, F_SETFL, flags_ns | O_NONBLOCK) == -1){perror("fcntl"); close(ns); continue;}
                    struct cliente *clt = calloc(1, sizeof(struct cliente));
                    if(clt == NULL){close(ns); continue;}
                    clt->ns = ns;
                    clt->fd = -1;
                    clt->fase = RECIBIENDO;
                    ep_ev.events = EPOLLIN;
                    ep_ev.data.ptr = clt;
                    if(epoll_ctl(ep, EPOLL_CTL_ADD, ns, &ep_ev) == -1){ perror("epoll_ctl"); close(ns); free(clt); continue;}
                }
            }else{
                struct cliente *clt = (struct cliente *)events[i].data.ptr;

                if(events[i].events & EPOLLERR){liberar_recursos(ep, clt); continue;}
                if(events[i].events & EPOLLHUP && !(events[i].events & EPOLLIN)){liberar_recursos(ep, clt); continue;}

                ssize_t rcv = 0;
                int tipo_http = 0;
                char ruta[LIMITE_URI+1];
                int term_atencion_clt = 0;
                if(events[i].events & EPOLLIN && clt->fase == RECIBIENDO){
                    procesar_buf_clt:
                    while(1){
                        if(clt->recibidos > 0){
                            size_t bytes_consumidos = 0;
                            tipo_http = analizar_solicitud_http(clt->buffer, clt->recibidos, ruta, sizeof ruta, &clt->cerrar_conexion_salida, &bytes_consumidos);
                            
                            size_t restantes = 0;
                            
                            switch(tipo_http){
                                case HTTP_OK:
                                    printf("Ruta: %s\n %s", ruta, OK);
                                    clt->cod_estado = 200;
                                    if(bytes_consumidos <= 0 || bytes_consumidos > clt->recibidos){liberar_recursos(ep, clt), term_atencion_clt = 1; break;}
                                    restantes = clt->recibidos - bytes_consumidos;
                                    memmove(clt->buffer, clt->buffer + bytes_consumidos, restantes);
                                    clt->recibidos = restantes;
                                    break;
                                case HTTP_METODO_NO_PERMITIDO:
                                    fprintf(stderr, NOT_ALLOWED);
                                    clt->cod_estado = 405;
                                    clt->cerrar_conexion_salida = 1;
                                    break;
                                case HTTP_INVALIDA:
                                    fprintf(stderr, BAD_REQUEST);
                                    clt->cod_estado = 400;
                                    clt->cerrar_conexion_salida = 1;
                                    break;
                                case HTTP_INCOMPLETA:
                                    
                                    break;
                            }
                            if(term_atencion_clt == 1){break;}

                            if(tipo_http == HTTP_INCOMPLETA){
                                if(clt->recibidos >= LIMITE){fprintf(stderr, BAD_REQUEST); clt->cod_estado = 400; clt->cerrar_conexion_salida = 1; break;}
                            }
                            if(tipo_http == HTTP_OK){break;}
                            if(tipo_http == HTTP_INVALIDA){break;}
                            if(tipo_http == HTTP_METODO_NO_PERMITIDO){break;}
                        }

                        if(clt->recibidos == LIMITE){fprintf(stderr, "La petición supera el tamaño del buffer\n"); clt->cod_estado = 400; clt->cerrar_conexion_salida = 1; break;}
                        rcv = recv(clt->ns, clt->buffer + clt->recibidos, LIMITE - clt->recibidos, 0);
                        if(rcv == -1){
                            if(errno == EINTR){
                                fprintf(stderr, "Interrupción por señal\n"); 
                                continue;
                            }
                            if(errno == EAGAIN || errno == EWOULDBLOCK){break;}
                            perror("recv"); 
                            liberar_recursos(ep, clt);
                            term_atencion_clt = 1;
                            break;
                        }
                        if(rcv == 0){
                            if(clt->recibidos == 0){
                                fprintf(stderr, "El cliente ha cerrado la conexión sin enviar datos\n"); 
                                liberar_recursos(ep, clt);
                                term_atencion_clt = 1;
                                break;
                            }else{
                                fprintf(stderr, BAD_REQUEST); 
                                clt->cod_estado = 400;
                                clt->cerrar_conexion_salida = 1;
                                break;
                            }
                        }
                        
                        clt->recibidos += rcv;
                        
                    }
                    if(term_atencion_clt == 1){term_atencion_clt = 0; continue;}
                    size_t tam_archivo = 0;

                    char ruta_efectiva[1024];

                    if(tipo_http == HTTP_OK){    
                        int tipo_files = solicitar_archivo(&clt->fd, ruta, &tam_archivo, ruta_efectiva, sizeof ruta_efectiva);
                        printf("Ruta: %s\n", ruta);
                        switch(tipo_files){
                            case 1: clt->cod_estado = 200; break;
                            case 0: clt->cod_estado = 404; break;
                            case -1: clt->cod_estado = 403; break;
                            case -2: clt->cod_estado = 500; break;
                        }
                    }

                    const char *mime = "text/plain";
                    if(clt->cod_estado == 200 && clt->fd != -1){
                        mime = obtener_tipo_mime(ruta_efectiva);
                    }

                    if(clt->cod_estado != 0){
                        if(clt->cod_estado != 200){tam_archivo = 0;}
                        clt->bytes_generados = generar_respuesta_http(clt->cod_estado, mime, tam_archivo, !clt->cerrar_conexion_salida, clt->buffer_salida, sizeof(clt->buffer_salida)); 
                        if(clt->bytes_generados <= 0){
                            fprintf(stderr, "No se pudo generar la respuesta HTTP");
                            liberar_recursos(ep, clt);
                            continue;
                        }else{
                            clt->fase = ENVIANDO;
                            ep_ev.events = EPOLLOUT;
                            ep_ev.data.ptr = clt;
                            if(epoll_ctl(ep, EPOLL_CTL_MOD, clt->ns, &ep_ev) == -1){liberar_recursos(ep, clt); continue;}
                        }
                    }
                }else if(events[i].events & EPOLLOUT && clt->fase == ENVIANDO){
                    /*Envio de encabezados*/
                    ssize_t snd = 0;
                    int eagain = 0;
                    while(clt->bytes_enviados < (size_t) clt->bytes_generados){
                        snd = send(clt->ns, clt->buffer_salida + clt->bytes_enviados, clt->bytes_generados - clt->bytes_enviados, MSG_NOSIGNAL); /*MSG_NOSIGNAL evita generar una señal SIGPIPE si el cliente cierra la conexion*/
                        if(snd == -1){
                            if(errno == EINTR){
                                fprintf(stderr, "Interrupcion por señal");
                                continue;
                            }
                            if(errno == EAGAIN){eagain = 1; break;}
                            perror("send"); 
                            liberar_recursos(ep, clt);
                            term_atencion_clt = 1;
                            break;
                        }
                        if(snd == 0){fprintf(stderr, "Falta de progreso"); liberar_recursos(ep, clt); term_atencion_clt = 1; break;}
                        clt->bytes_enviados += snd;
                    }
                    if(term_atencion_clt == 1){term_atencion_clt = 0; continue;}
                    /*Envio de archivo*/
                    if(eagain == 1){continue;}
                    ssize_t leidos = 0;
                    int eagain2 = 0;
                    int term_resp = 0;
                    if(clt->cod_estado == 200 && clt->fd != -1){   
                        while(1){
                            if(clt->bytes_archivo_enviados == clt->bytes_leidos){
                                
                                leidos = read(clt->fd, clt->buffer_lectura, sizeof(clt->buffer_lectura));
                                if(leidos == -1){
                                    if(errno == EINTR){
                                        fprintf(stderr, "Interrupción por señal\n"); 
                                        while(leidos == -1 && errno == EINTR){
                                            leidos = read(clt->fd, clt->buffer_lectura, sizeof(clt->buffer_lectura));
                                        }   
                                    }
                                }
                                if(leidos == -1){perror("read"); liberar_recursos(ep, clt); term_atencion_clt = 1; break;}
                                if(leidos == 0){
                                    term_resp = 1;
                                    break;
                                }
                                clt->bytes_archivo_enviados = 0;
                                clt->bytes_leidos = leidos;
                            }
                            if(term_atencion_clt == 1){break;}
                            

                            ssize_t snd2 = 0;
                            while(clt->bytes_archivo_enviados < clt->bytes_leidos){
                                snd2 = send(clt->ns, clt->buffer_lectura + clt->bytes_archivo_enviados, clt->bytes_leidos - clt->bytes_archivo_enviados, MSG_NOSIGNAL); /*MSG_NOSIGNAL evita generar una señal SIGPIPE si el cliente cierra la conexion*/
                                if(snd2 == -1){
                                    if(errno == EINTR){
                                        fprintf(stderr, "Interrupcion por señal");
                                        continue;
                                    }
                                    if(errno == EAGAIN){eagain2 = 1; break;}
                                    perror("send");
                                    liberar_recursos(ep, clt);
                                    term_atencion_clt = 1;
                                    break;
                                }
                                if(snd2 == 0){fprintf(stderr, "Falta de progreso"); liberar_recursos(ep, clt); term_atencion_clt=1; break;}
                                clt->bytes_archivo_enviados += snd2;
                            }
                            
                            if(term_atencion_clt == 1){break;}
                            if(eagain2 == 1){break;}
                        }
                        if(term_atencion_clt == 1){continue;}
                        if(eagain2 == 1){continue;}
                    }
                    if(clt->cod_estado != 200){term_resp = 1;}
                    if(term_resp == 1){
                        if(clt->cerrar_conexion_salida == 1){
                            liberar_recursos(ep, clt);
                            continue;
                        }else{
                            if(clt->fd != -1){close(clt->fd);}
                            clt->fd = -1;
                            clt->cod_estado = 0;
                            clt->bytes_generados = 0;
                            clt->bytes_enviados = 0;
                            clt->bytes_leidos= 0;
                            clt->bytes_archivo_enviados = 0;
                            clt->fase = RECIBIENDO;
                            ep_ev.events = EPOLLIN;
                            ep_ev.data.ptr = clt;
                            if(epoll_ctl(ep, EPOLL_CTL_MOD, clt->ns, &ep_ev) == -1){liberar_recursos(ep, clt); continue;} 
                            if(clt->recibidos == 0){continue;}
                            if(clt->recibidos > 0){goto procesar_buf_clt;}
                        }
                    }
                }
            }            
        }
    }
    close(s);
    return EXIT_SUCCESS;
}
