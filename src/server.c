#include "server.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

#define QLEN 5

int crear_socket_serv(const char *puerto){
    struct addrinfo hints, *res, *p;
    int s = -1;
    int optval = 1; /*Indica al kernel que active la opcion SO_REUSEADDR*/
    socklen_t optlen = sizeof optval;

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    int gai = getaddrinfo(NULL, puerto, &hints, &res);

    if(gai != 0){fprintf(stderr, "Error en getaddrinfo: %s\n",gai_strerror(gai)); return -1;}

    for(p = res ; p!=NULL; p = p->ai_next){
        s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(s == -1){perror("socket"); continue;}
        if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &optval, optlen) == -1){perror("setsockopt");close(s); s = -1; continue;}
        if(bind(s, p->ai_addr, p->ai_addrlen) == -1){perror("bind");close(s); s = -1; continue;}
        if(listen(s, QLEN) != -1){break;}
        perror("listen");
        close(s);
        s = -1;
    }

    freeaddrinfo(res);

    return s;

}

int enviar(int s, const char *buffer, size_t bytes_enviar){
    size_t enviados = 0;
    ssize_t snd = 0;
    while(enviados < bytes_enviar){
        snd = send(s, buffer + enviados, bytes_enviar - enviados, MSG_NOSIGNAL); /*MSG_NOSIGNAL evita generar una señal SIGPIPE si el cliente cierra la conexion*/
        if(snd == -1){
            if(errno == EINTR){
                fprintf(stderr, "Interrupcion por señal");
                continue;
            }
            perror("send"); 
            return -1;
        }
        if(snd == 0){fprintf(stderr, "Falta de progreso"); return -1;}
        enviados += snd;
    }
    return 1;
}