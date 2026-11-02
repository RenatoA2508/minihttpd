#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
enum resultados {
    HTTP_OK,
    HTTP_INCOMPLETA,
    HTTP_INVALIDA,
    HTTP_METODO_NO_PERMITIDO,
};

int es_token_http(const char *cadena, size_t longitud_cadena){
    if(longitud_cadena == 0){return 0;}
    const char *char_validos = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#$%&'*+-.^_|~`";
    size_t len_char_validos = strlen(char_validos);
    for(size_t i=0; i<longitud_cadena; i++){
        if(memchr(char_validos, cadena[i], len_char_validos) == NULL){return 0;} /*0-> no es token HTTP*/
    }
    return 1; /*1 -> si es token HTTP*/
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


int analizar_solicitud_http(const char *buffer, size_t longitud, char *ruta, size_t capacidad_ruta){
    
    const char *fin_encabezado = memmem(buffer, longitud, "\r\n\r\n", 4);
    if(fin_encabezado == NULL){return HTTP_INCOMPLETA;}

    const char *fin_primera_linea = memmem(buffer, longitud, "\r\n", 2);
    if(fin_primera_linea == NULL){return HTTP_INVALIDA;}

    size_t longitud_primera_linea = fin_primera_linea - buffer; 
    const char *primer_espacio = memchr(buffer, ' ', longitud_primera_linea);

    if(primer_espacio == NULL){return HTTP_INVALIDA;}
    size_t longitud_metodo = primer_espacio - buffer;
    
    if(longitud_metodo==0){return HTTP_INVALIDA;}

    if(es_token_http(buffer, longitud_metodo) == 0){return HTTP_INVALIDA;}

    int metodo_no_permitido = 0; // 0 -> el método si es GET, 1 -> el método no es GET
    if(longitud_metodo != 3 || memcmp(buffer, "GET",longitud_metodo) != 0){metodo_no_permitido = 1;}

    const char *segundo_espacio = memchr(primer_espacio + 1, ' ', fin_primera_linea - (primer_espacio + 1));
    if(segundo_espacio == NULL){return HTTP_INVALIDA;}
    size_t longitud_uri = segundo_espacio - (primer_espacio + 1);
    if(longitud_uri >= capacidad_ruta || longitud_uri == 0){return HTTP_INVALIDA;}
    
    
    memcpy(ruta, primer_espacio + 1, longitud_uri);
    ruta[longitud_uri] = '\0';

    size_t longitud_protocolo = fin_primera_linea - (segundo_espacio + 1);
    
    if(longitud_protocolo != 8 || memcmp(segundo_espacio + 1, "HTTP/1.1", longitud_protocolo) != 0){return HTTP_INVALIDA;}

    const char *inicio_linea_encabezado = fin_primera_linea + 2;
    int contador_host = 0;
    while(inicio_linea_encabezado < fin_encabezado + 2){
        const char *fin_linea_encabezado = memmem(inicio_linea_encabezado, (fin_encabezado + 2) - (inicio_linea_encabezado), "\r\n", 2);
        if(fin_linea_encabezado == NULL){return HTTP_INVALIDA;}
        size_t longitud_linea_encabezado = fin_linea_encabezado - (inicio_linea_encabezado);
        const char *dos_puntos = memchr(inicio_linea_encabezado, ':', longitud_linea_encabezado);
        if(dos_puntos == NULL){return HTTP_INVALIDA;}
        size_t longitud_nombre_encabezado = dos_puntos - inicio_linea_encabezado;
        if(longitud_nombre_encabezado == 0){return HTTP_INVALIDA;}

        if(es_token_http(inicio_linea_encabezado, longitud_nombre_encabezado) == 0){return HTTP_INVALIDA;}


        const char *inicio_valor = dos_puntos + 1;
        const char *fin_valor = fin_linea_encabezado;
        size_t longitud_valor_encabezado = extraer_valor_encabezado(&inicio_valor, &fin_valor);
        if(longitud_nombre_encabezado == 4 && strncasecmp(inicio_linea_encabezado, "Host", 4) == 0){
            contador_host += 1;
            if(longitud_valor_encabezado == 0){return HTTP_INVALIDA;}
        }
        inicio_linea_encabezado = fin_linea_encabezado + 2;
    }
    if(contador_host != 1){
        return HTTP_INVALIDA;
    }else if (metodo_no_permitido == 0){
        return HTTP_OK;
    }else{
        return HTTP_METODO_NO_PERMITIDO;
    }
}

