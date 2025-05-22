#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "extra.h"
#include "map.h"
#include "list.h"

#define MAX_LIN_LEN 1024
#define TIEMPO_INCIAL 100

typedef struct{
    char nombre[128];
    int puntos;
    int peso;
} Item;

typedef struct{
    int id;
    char nombre[128];
    char descripcion[512];
    List *items; //Lista en el escenario
    Map *conexiones; //Mapa de conexiones
    bool esFinal;
} Escenario;

typedef struct{
    Escenario *actual;
    List *inventario; //Items recolectados
    int tiempo; //Tiempo restante
    int puntaje;
    int peso;
} EstadoJuego;

int mapEmpty(Map *map) {
    return map_first(map) == NULL;
}

void mostrarMenu() {
    printf("\n--- GraphQuest ---\n");
    printf("1. Cargar laberinto\n");
    printf("2. Iniciar partida\n");
    printf("3. Salir\n");
    printf("Selecciona: ");
}

List *analizarElem(char *itemsStr){
    List *items = list_create();
    if (!itemsStr || strlen(itemsStr) == 0) return items; // Si no hay items, retornar lista vacia

    List *stringItems = split_string(itemsStr, ',');
    for (char *itemsStr = list_first(stringItems); itemsStr != NULL; itemsStr = list_next(itemsStr)){
        char *paren = strchr(itemsStr, '(');
        if (!paren) continue; // Si no hay paréntesis, continuar

        *paren = '\0'; // Terminar la cadena antes del paréntesis
        char *nombre = itemsStr;
        int puntos, peso;
        if (sscanf(paren + 1, "%d pts, %d kg", &puntos, &peso) == 2) {
            Item* item = malloc(sizeof(Item));
            strcpy(item->nombre, nombre);
            item->puntos = puntos;
            item->peso = peso;
            list_push_back(items, item);
        }
    }
    list_clean(stringItems); // Limpiar la lista de strings
    return items;
}

void cargarEscenarios(Map *escenarios, const char *nombreArchivo){
    FILE *archivo = fopen(nombreArchivo, "r");
    if (!archivo) {
        printf("Error al abrir el archivo\n");
        return;
    }

    leer_linea_csv(archivo, ',');

    char **campos = leer_linea_csv(archivo, ',');
    while (campos != NULL){
        Escenario *esc = malloc(sizeof(Escenario));
        esc->conexiones = map_create(NULL);
        esc->items = analizarELem(campos[3]);

        esc->id = atoi(campos[0]);
        strcpy(esc->nombre, campos[1]);
        strcpy(esc->descripcion, campos[2]);

        if (campos[4][0] != '\0') map_insert(esc->conexiones, "arriba", (void*)(long)atoi(campos[4]));
        if (campos[5][0] != '\0') map_insert(esc->conexiones, "abajo", (void*)(long)atoi(campos[5]));
        if (campos[6][0] != '\0') map_insert(esc->conexiones, "derecha", (void*)(long)atoi(campos[6]));
        if (campos[7][0] != '\0') map_insert(esc->conexiones, "izquierda", (void*)(long)atoi(campos[7]));

        esc->esFinal = (strcmp(campos[8], "1") == 0);
        map_insert(escenarios, (void*)(long)esc->id, esc);

    }
    fclose(archivo);
}

int main(){
    Map *escenarios = map_create(NULL);
    int opcion;

    do{
        limpiarPantalla();
        mostrarMenu();
        scanf("%d", &opcion);
        getchar(); //Limpiar el buffer de entrada

        switch(opcion){
            case 1:
                cargarEscenarios(escenarios, "graphquest.cvs");
                printf("Laberinto cargado exitosamente.\n");
                presioneTeclaParaContinuar();
                break;
            case 2:
                if (!mapEmpty(escenarios)) iniciarPartida(escenarios);
                else printf("No hay laberinto cargado. Cargue uno primero.\n");
                presioneTeclaParaContinuar();
                break;
            case 3:
                printf("Inventario: \n");
                break;
            case 4:
                printf("Saliendo del juego...\n");
                break;
            default:
                printf("Opcion no valida. Intente de nuevo.\n");
                presioneTeclaParaContinuar();
                break;
        }
    } while (opcion != 3);

    return 0;
}