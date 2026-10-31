# minihttpd
Servidor HTTP/1.1 concurrente con epoll en C 

# Objetivo
Desarrollar un servidor HTTP/1.1 que implementa únicamente el método GET, utilizando la API de sockets y el lenguaje de programación C.

# Estructura
La estructura del proyecto es la siguiente:

minihttpd/
├── Makefile
├── README.md
├── include/
│ ├── http.h
│ ├── server.h
│ ├── mime.h
│ └── files.h
├── src/
│ ├── main.c
│ ├── server.c
│ ├── http.c
│ ├── mime.c
│ └── files.c
├── www/
├── index.html
├── style.css
└── image.png

