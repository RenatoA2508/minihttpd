#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include "http.h"



int es_token_http(const char *cadena, size_t longitud_cadena){
    if(longitud_cadena == 0){return 0;}
    const char *char_validos = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&'*+-.^_|~`";
    size_t len_char_validos = strlen(char_validos);
    for(size_t i=0; i<longitud_cadena; i++){
        if(memchr(char_validos, cadena[i], len_char_validos) == NULL){return 0;} /*0-> no es token HTTP*/
    }
    return 1; /*1 -> si es token HTTP*/
}

int validar_caracteres_uri(const char *cadena, size_t longitud_cadena){
    if(longitud_cadena == 0){return 0;}
    const char *char_validos = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~!$&'()*+,;=:/@?%";
    const char *hexadecimales = "0123456789ABCDEFabcdef"; 
    size_t len_char_validos = strlen(char_validos);
    size_t len_hexadecimales = strlen(hexadecimales);
    for(size_t i=0; i<longitud_cadena; i++){
        if(memchr(char_validos, cadena[i],len_char_validos) == NULL){return 0;} /*0-> no es valido para la URI*/
        if(cadena[i] == '%'){
            if(longitud_cadena - i < 3){return 0;}
            if(memchr(hexadecimales, cadena[i+1], len_hexadecimales) == NULL || memchr(hexadecimales, cadena[i+2], len_hexadecimales) == NULL){
                return 0;
            }
            i+=2;
        }
    }
    return 1; /*1 -> Si es valido para URI*/
}

size_t extraer_valor_encabezado(const char **inicio, const char **fin){
    size_t longitud_real = 0;
    while(*inicio < *fin && ((*inicio)[0] == ' ' || (*inicio)[0] == '\t')){
        *inicio = *inicio + 1;
    }
    while(*inicio < *fin && ((*fin)[-1] == ' ' || (*fin)[-1] == '\t')){
        *fin = *fin - 1;
    }
    longitud_real = *fin - *inicio;
    return longitud_real;
}

int validar_valor_host(const char *cadena, size_t longitud_cadena){
    if(longitud_cadena == 0){return 0;}
    const char *dos_puntos = memchr(cadena, ':', longitud_cadena);
    size_t longitud_valor = 0;
    if(dos_puntos != NULL){
        if(dos_puntos == cadena){return 0;} /* Rechaza cuando el campo del nombre o IP este vacio*/
        size_t longitud_puerto = longitud_cadena - (dos_puntos - cadena + 1);
        const char *c_puerto = dos_puntos + 1;
        int i_puerto = 0;
        for(size_t i=0; i<longitud_puerto ; i++){
            if(isdigit((unsigned char)c_puerto[i]) == 0){return 0;}
            
            i_puerto = i_puerto*10 + (c_puerto[i]-'0');
            if(i_puerto > 65535){return 0;}
        }
        if(i_puerto <= 0){ return 0;}

        longitud_valor = dos_puntos - cadena;
        if(longitud_valor == 0){return 0;}
    }else{
        longitud_valor = longitud_cadena;
    }

    int es_ip = 1, tiene_puntos = 0;
    for(size_t i = 0; i<longitud_valor; i++){
        if (cadena[i] == '.') {
            tiene_puntos = 1;
        } else if (!isdigit((unsigned char)cadena[i])) {
            es_ip = 0;
            break;
        }
    }


    if(es_ip == 1 && tiene_puntos == 1){
        char ip[16];
        if(longitud_valor > 15){return 0;}
        strncpy(ip, cadena, longitud_valor);
        ip[longitud_valor] = '\0';
        struct in_addr dir_ip = {0};
        if(inet_pton(AF_INET, ip, &dir_ip) != 1){return 0;}
    }else{
        const char *char_validos = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-";
        size_t len_char_validos = strlen(char_validos);
        size_t longitud_segmento = 0;
        for(size_t i = 0; i < longitud_valor; i++){
            if(cadena[i] == '.'){
                if(longitud_segmento == 0){return 0;}
                if(cadena[i - 1] == '-'){return 0;}
                longitud_segmento = 0;
                
            }else{
                if(memchr(char_validos, cadena[i], len_char_validos) != NULL ){
                    if(cadena[i] == '-' && longitud_segmento == 0){return 0;}
                    longitud_segmento += 1;
                    if(longitud_segmento > 63){return 0;}
                }else{
                    return 0;
                }
            }
        }
        if(cadena[0] == '-' || cadena[0] == '.' || cadena[longitud_valor - 1] == '-' || cadena[longitud_valor - 1] == '.'){return 0;}        
    }
    
    return 1;
}

int validar_valor_user_agent(const char *cadena, size_t longitud_cadena){
    /*Solo validacion de caracteres, no de sintaxis*/
    for(size_t i=0; i<longitud_cadena; i++){
        if((unsigned char)cadena[i] == 0x7F){return 0;}
        if((unsigned char)cadena[i] < 0x20 && (unsigned char)cadena[i] != 0x09){return 0;}
    }
    return 1;
}

int validar_valor_connection(const char *cadena, size_t longitud_cadena, int *cerrar_conexion){
    const char *p = cadena;
    size_t longitud_token = 0;
    const char *fin_token;
    while(p < cadena + longitud_cadena){
            const char *coma =memchr(p, ',', (cadena + longitud_cadena) - p);
            if(coma == NULL){
                fin_token = cadena + longitud_cadena;
            }else{
                fin_token = coma;
            }
            const char *aux_inicio = p, *aux_fin = fin_token;
            longitud_token = extraer_valor_encabezado(&aux_inicio, &aux_fin);
            if(longitud_token > 0){
                if(es_token_http(aux_inicio, longitud_token) == 0){return 0;}
                if(longitud_token == 5 && strncasecmp(aux_inicio, "close", 5) == 0){*cerrar_conexion = 1;}
            } /*Ignora tokens vacios*/
            if(coma == NULL){break;}
            p = coma + 1;
        }
    return 1;
}

int analizar_solicitud_http(const char *buffer, size_t longitud, char *ruta, size_t capacidad_ruta, int *cerrar_conexion_salida){
    int cerrar_conexion = 0; /*Mantener la conexion abierta. Por defecto en HTTP*/

    const char *fin_encabezado = memmem(buffer, longitud, "\r\n\r\n", 4); /*Identifica el fin de la zona de encabezados*/
    if(fin_encabezado == NULL){
        if(longitud >= LIMITE_ENCABEZADOS){return HTTP_INVALIDA;} /*No existe \r\n\r\n y la seccion iguala o supera el limite*/
        return HTTP_INCOMPLETA;
    }
    if(fin_encabezado - buffer + 4 > LIMITE_ENCABEZADOS){return HTTP_INVALIDA;} /*Si existe \r\n\r\n, pero la seccion supera el limite*/
    const char *fin_primera_linea = memmem(buffer, longitud, "\r\n", 2); /*Identifica el fin de la primera linea de la solicitud HTTP-> GET /___ HTTP/1.1 */
    if(fin_primera_linea == NULL){return HTTP_INVALIDA;}
    size_t longitud_primera_linea = fin_primera_linea - buffer;
    if(longitud_primera_linea > LIMITE_PRIMERA_LINEA){return HTTP_INVALIDA;}
    /*Analisis Metodo*/ 
    const char *primer_espacio = memchr(buffer, ' ', longitud_primera_linea); /*Primer espacio -> GET_/ */

    if(primer_espacio == NULL){return HTTP_INVALIDA;}
    size_t longitud_metodo = primer_espacio - buffer;
    
    if(longitud_metodo==0){return HTTP_INVALIDA;}

    if(es_token_http(buffer, longitud_metodo) == 0){return HTTP_INVALIDA;}

    int metodo_no_permitido = 0; // 0 -> el método si es GET, 1 -> el método no es GET
    if(longitud_metodo != 3 || memcmp(buffer, "GET",longitud_metodo) != 0){metodo_no_permitido = 1;}
    
    /*Analisis URI*/
    const char *segundo_espacio = memchr(primer_espacio + 1, ' ', fin_primera_linea - (primer_espacio + 1)); /*Segundo espacio -> /_HTTP/1.1 */
    if(segundo_espacio == NULL){return HTTP_INVALIDA;}
    size_t longitud_uri = segundo_espacio - (primer_espacio + 1);

    if(longitud_uri > LIMITE_URI || longitud_uri == 0){return HTTP_INVALIDA;}
    
    if(primer_espacio[1] != '/'){return HTTP_INVALIDA;}

    const char *signo_interrogacion = memchr(primer_espacio + 1, '?', longitud_uri);
    size_t longitud_ruta = 0;
    if(signo_interrogacion != NULL){
        longitud_ruta = signo_interrogacion - (primer_espacio + 1);
    }else{
        longitud_ruta = longitud_uri;
    }

    if(longitud_ruta >= capacidad_ruta || longitud_ruta == 0){return HTTP_INVALIDA;}

    if(validar_caracteres_uri(primer_espacio + 1, longitud_uri) == 0){return HTTP_INVALIDA;}
    
    /*Analisis HTTP/1.1*/
    size_t longitud_protocolo = fin_primera_linea - (segundo_espacio + 1);
    
    if(longitud_protocolo != 8 || memcmp(segundo_espacio + 1, "HTTP/1.1", longitud_protocolo) != 0){return HTTP_INVALIDA;}

    const char *inicio_linea_encabezado = fin_primera_linea + 2;
    int contador_host = 0;
    
    /*Analisis de encabezados*/
    while(inicio_linea_encabezado < fin_encabezado + 2){
        const char *fin_linea_encabezado = memmem(inicio_linea_encabezado, (fin_encabezado + 2) - (inicio_linea_encabezado), "\r\n", 2);
        if(fin_linea_encabezado == NULL){return HTTP_INVALIDA;}
        size_t longitud_linea_encabezado = fin_linea_encabezado - (inicio_linea_encabezado);
        if(longitud_linea_encabezado > LIMITE_LINEA_ENCABEZADO){return HTTP_INVALIDA;}
        const char *dos_puntos = memchr(inicio_linea_encabezado, ':', longitud_linea_encabezado);
        if(dos_puntos == NULL){return HTTP_INVALIDA;}
        size_t longitud_nombre_encabezado = dos_puntos - inicio_linea_encabezado;
        if(longitud_nombre_encabezado == 0){return HTTP_INVALIDA;}

        if(es_token_http(inicio_linea_encabezado, longitud_nombre_encabezado) == 0){return HTTP_INVALIDA;}


        const char *inicio_valor = dos_puntos + 1;
        const char *fin_valor = fin_linea_encabezado;
        size_t longitud_valor_encabezado = extraer_valor_encabezado(&inicio_valor, &fin_valor);
        for(size_t i=0; i<longitud_valor_encabezado ; i++){
            if((unsigned char)inicio_valor[i] == 0x7F){return HTTP_INVALIDA;}
            if((unsigned char)inicio_valor[i] < 0x20 && (unsigned char)inicio_valor[i] != 0x09){return HTTP_INVALIDA;}
        }
        if(longitud_nombre_encabezado == 4 && strncasecmp(inicio_linea_encabezado, "Host", 4) == 0){
            contador_host += 1;
            if(contador_host > 1){return HTTP_INVALIDA;}
            if(validar_valor_host(inicio_valor, longitud_valor_encabezado) == 0){return HTTP_INVALIDA;}
            if(longitud_valor_encabezado == 0){return HTTP_INVALIDA;}
        }
        if(longitud_nombre_encabezado == 10 && strncasecmp(inicio_linea_encabezado, "User-Agent", 10) == 0){
            if(longitud_valor_encabezado == 0){return HTTP_INVALIDA;}
            if(validar_valor_user_agent(inicio_valor, longitud_valor_encabezado) == 0){return HTTP_INVALIDA;}
        }
        if(longitud_nombre_encabezado == 10 && strncasecmp(inicio_linea_encabezado, "Connection", 10) == 0){
            if(validar_valor_connection(inicio_valor, longitud_valor_encabezado, &cerrar_conexion) == 0){return HTTP_INVALIDA;}
        }
        inicio_linea_encabezado = fin_linea_encabezado + 2;
    }
    if(contador_host == 1){
        memcpy(ruta, primer_espacio + 1, longitud_ruta);
        ruta[longitud_ruta] = '\0';
    }
    if(contador_host != 1){
        return HTTP_INVALIDA;
    }else if (metodo_no_permitido == 0){
        *cerrar_conexion_salida = cerrar_conexion;
        return HTTP_OK;
    }else{
        *cerrar_conexion_salida = cerrar_conexion;
        return HTTP_METODO_NO_PERMITIDO;
    }
}

const char *obtener_cod_estado(int codigo){
    switch(codigo){
        case 200:
            return "200 OK";
        case 400:
            return "400 Bad Request";
        case 403:
            return "403 Forbidden";
        case 404:
            return "404 Not Found";
        case 405:
            return "405 Method Not Allowed";
        case 500:
            return "500 Internal Server Error";
        default:
            return NULL;
    }
}

int generar_respuesta_http(int cod_estado, const char *mime, size_t longitud_cuerpo, int persistencia, char *buf_salida, size_t capacidad_buf_salida){
    if(buf_salida == NULL){return -1;}
    if(capacidad_buf_salida == 0){return -1;}
    const char *protocolo = "HTTP/1.1 ";
    const char *estado = obtener_cod_estado(cod_estado);
    if(estado == NULL){return -1;}
    const char *content_type = "Content-Type: ";
    if(mime == NULL){return -1;}
    const char *content_length = "Content-Length: ";
    const char *connection = "Connection: ";
    const char *valor_connection;
    if(persistencia == 1){
        valor_connection = "keep-alive";
    }else{
        valor_connection = "close";
    }
    int tamaño_respuesta = snprintf(buf_salida, capacidad_buf_salida, "%s%s\r\n%s%s\r\n%s%zu\r\n%s%s\r\n\r\n", protocolo, estado, content_type, 
                                        mime, content_length, longitud_cuerpo, connection, valor_connection);

    if(tamaño_respuesta < 0){
        return -1;
    }else if((size_t) tamaño_respuesta >= capacidad_buf_salida){
        return 0;
    }
    return tamaño_respuesta;
}