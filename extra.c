#include "extra.h"


#define MAX_LINE_LENGTH 4096
#define MAX_FIELDS      128

char **leer_linea_csv(FILE *archivo, char separador) {
    char *linea = malloc(MAX_LINE_LENGTH);
    if (!linea) return NULL;
    
    char **campos = malloc(MAX_FIELDS * sizeof(char *));
    if (!campos) {
        free(linea);
        return NULL;
    }

    if (fgets(linea, MAX_LINE_LENGTH, archivo) == NULL) {
        free(linea);
        free(campos);
        return NULL;
    }

    // Eliminar salto de línea
    linea[strcspn(linea, "\r\n")] = '\0';

    int idx = 0;
    char *ptr = linea;
    campos[idx++] = ptr; // Primer campo comienza al inicio

    while (*ptr && idx < MAX_FIELDS) {
        if (*ptr == '\"') {
            // Campo entrecomillado
            ptr++;
            campos[idx-1] = ptr; // Comienza después de la comilla
            
            // Buscar fin de campo entrecomillado
            while (*ptr && !(*ptr == '\"' && (*(ptr+1) == separador || *(ptr+1) == '\0'))) {
                if (*ptr == '\"' && *(ptr+1) == '\"') ptr++; // Saltar comillas escapadas
                ptr++;
            }
            
            if (*ptr == '\"') {
                *ptr = '\0'; // Terminar cadena
                ptr++;
                if (*ptr == separador) ptr++;
            }
        } else {
            // Campo normal
            while (*ptr && *ptr != separador) ptr++;
            if (*ptr == separador) {
                *ptr = '\0';
                ptr++;
            }
        }
        
        if (*ptr) campos[idx++] = ptr;
    }

    campos[idx] = NULL;
    return campos;
}

void liberar_campos(char **campos) {
    if (campos) {
        // El buffer completo está en campos[0]
        if (campos[0]) free(campos[0]);
        // Liberar el array de punteros
        free(campos);
    }
}

List *split_string(const char *str, const char *delim) {
    List *result = list_create();
    if (!result) return NULL;

    char *temp = strdup(str);
    if (!temp) {
        list_clean(result);
        return NULL;
    }

    char *token = strtok(temp, delim);
    while (token != NULL) {
        // Eliminar espacios al inicio y final
        while (*token == ' ') token++;
        
        char *end = token + strlen(token) - 1;
        while (end > token && *end == ' ') end--;
        *(end + 1) = '\0';

        if (*token) { // Solo agregar si no está vacío
            char *new_token = strdup(token);
            if (!new_token) {
                free(temp);
                list_clean(result);
                return NULL;
            }
            list_pushBack(result, new_token);
        }
        
        token = strtok(NULL, delim);
    }

    free(temp);
    return result;
}

// Función para limpiar la pantalla
void limpiarPantalla() { system("clear"); }

void presioneTeclaParaContinuar() {
    printf("\nPresione Enter para continuar...");
    // Limpiar buffer primero
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    // Esperar nueva entrada
    while ((c = getchar()) != '\n' && c != EOF);
}