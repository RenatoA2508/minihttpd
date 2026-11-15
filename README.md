# minihttpd
Servidor HTTP/1.1 concurrente con epoll en C 

## Objetivo
Desarrollar un servidor HTTP/1.1 que implementa únicamente el método GET, utilizando la API de sockets y el lenguaje de programación C.

## Características
- Implementa únicamente el método GET
- Permite conexiones persistentes y concurrentes con epoll. 
- Identifica tipos MIME y códigos 400, 403, 404, 405, 500

## Requisitos
- Linux
- GCC

# Compilación
En la raíz del proyecto: make
Borrar el compilado: make clean

# Ejecución
En la raíz del proyecto: ./minihttpd

Luego, en cualquier navegador: http://{ip-maquina}:8080/
Ejemplos
- http://127.0.0.1:8080/ (forma local)
- http://192.168.100.106:8080/ (forma local o en la misma red)

# Estructura
## main.c
Arquitectura de epoll y lógica de recepción/envío de flujo de bytes

## files.c
Interacción con el sistema de archivos

## mime.c
Reconocimiento de tipos MIME

## server.c
Creación del socket del servidor listo para aceptar conexiones y reutilizar el puerto 8080

## http.c
Parsing de la solicitud HTTP y generación de encabezados de respuesta HTTP

# Limitaciones
El servidor no implementa otros métodos HTTP y responderá con 405 Method Not Allowed