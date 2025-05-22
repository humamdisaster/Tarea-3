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

void mostrarMenuJuego(){
    printf("\nOpciones:\n");
    printf("1. Recoger items\n");
    printf("2. Descartar items\n");
    printf("3. Avanzar\n");
    printf("4. Reiniciar\n");
    printf("5. Salir\n");
    printf("Selecciona una opción del 1 al 5 para continuar...\n");
}

void mostrarEscenarios(Escenario *escenario){
    printf("\n--- %s ---\n", escenario->nombre);
    printf("Descripción: %s\n", escenario->descripcion);

    printf("\nItems disponibles:\n");
    for (Item *item = list_first(escenario->items); item != NULL; item = list_next(escenario->items)){
        printf(" - %s (%d pts, %d kg)\n", item->nombre, item->puntos, item->peso);
    }
    printf("\nDirecciones disponibles:\n");
    MapPair *pair = map_first(escenario->conexiones);
    while (pair != NULL){
        printf(" - %s\n", (char*)pair->key);
        pair = map_next(escenario->conexiones);
    }
    printf("\n");
}

void mostrarEstado(EstadoJuego *estado){
    printf("\nTiempo restante: %d\n", estado->tiempo);
    printf("Puntaje: %d\n", estado->puntaje);
    printf("Peso: %d\n", estado->peso);

    printf("Inventario:\n");
    for (Item *item = list_first(estado->inventario); item != NULL; item = list_next(estado->inventario)){
        printf(" - %s (%d pts, %d kg)\n", item->nombre, item->puntos, item->peso);
    }
}

void recogerItem(EstadoJuego *estado){
    printf("Selecciona un item para recoger:\n");
    int i = 1;
    Item * item = list_first(estado->actual->items);
    while (item != NULL){
        printf("%d. %s\n", i++, item->nombre);
        item = list_next(estado->actual->items);
    }
    int seleccion;
    scanf("%d", &seleccion);
    while (seleccion == 1){
        if (seleccion < 1 || seleccion > list_size(estado->actual->items)) break;

        Item *target = NULL;
        int pos = 1;
        item = list_first(estado->actual->items);
        while (item != NULL && pos < seleccion){
            item = list_next(estado->actual->items);
            pos++;
        }
        if (item != NULL){
            list_pushBack(estado->inventario, item);
            estado->puntaje += item->puntos;
            estado->peso += item->peso;

            List *nuevosItems= list_create();
            Item *current = list_first(estado->actual->items);
            while (current != NULL){
                if (current != item) list_pushBack(nuevosItems, current);
                current = list_next(estado->actual->items);
            }
            list_clean(estado->actual->items);
            estado->actual->items = nuevosItems;
        }
        if (getchar() == '\n') break; // Esperar a que el usuario presione Enter
    }
    estado->tiempo--;
}

void descartarItem(EstadoJuego *estado){
    printf("\nSelecciona un item para descartar:\n");

    int i = 1;
    Item *item = list_first(estado->inventario);
    while (item != NULL){
        printf("%d. %s\n", i++, item->nombre);
        item = list_next(estado->inventario);
    }

    int seleccion;
    scanf("%d", &seleccion);
    while (seleccion == 1){
        if (seleccion < 1 || seleccion > list_size(estado->inventario)) break;

        Item *target = NULL;
        int pos = 1;
        item = list_first(estado->inventario);
        while (item != NULL && pos < seleccion){
            item = list_next(estado->inventario);
            pos++;
        }

        if (item != NULL){
            estado->puntaje -= item->puntos;
            estado->peso -= item->peso;

            List *nuevosItems = list_create();
            Item *current = list_first(estado->inventario);
            while (current != NULL){
                if (current != item) list_pushBack(nuevosItems, current);
                current = list_next(estado->inventario);
            }
            list_clean(estado->inventario);
            estado->inventario = nuevosItems;
        }
        if (getchar() == '\n') break; // Esperar a que el usuario presione Enter
    }
    estado->tiempo--;
}

void avanzar(EstadoJuego *estado, Map *escenarios){
    printf("\nSelecciona una dirección para avanzar:\n");
    char direccion[16];
    scanf("%s", direccion);

    void *target = map_search(estado->actual->conexiones, direccion);
    if (target == NULL){
        printf("No puedes avanzar en esa dirección.\n");
        presioneTeclaParaContinuar();
        return;
    }
    int tiempoConsumido = (estado->peso + 1) / 10;
    if ((estado->peso + 1) % 10 != 0) tiempoConsumido++; // Redondear hacia arriba
    estado->tiempo -= tiempoConsumido;

    int id = (long)target;
    estado->actual = map_search(escenarios, (void*)(long)id);
}

void iniciarPartida(Map *escenarios){
    EstadoJuego estado;
    estado.actual = map_search(escenarios, (void*)(long)1);
    if (!estado.actual) {
        printf("No se pudo encontrar el escenario inicial.\n");
        return;
    }

    estado.inventario = list_create();
    estado.tiempo = TIEMPO_INCIAL;
    estado.puntaje = 0;
    estado.peso = 0;

    while (estado.tiempo > 0 && !estado.actual->esFinal){
        limpiarPantalla();
        mostarEscenario(estado.actual);
        mostrarEstado(&estado);

        mostrarMenuJuego();
        int opcion;
        scanf("%d", &opcion);
        getchar(); //Limpiar el buffer de entrada

        switch (opcion){
            case 1:
                recogerItems(&estado);
                break;
            case 2:
                descartarItems(&estado);
                break;
            case 3:
                avanzar(&estado, escenarios);
                break;
            case 4:
                list_clean(estado.inventario);
                iniciarPartida(escenarios);
                return;
            case 5:
                return;
            default:
                printf("Opcion no valida. Intente de nuevo.\n");
                break;
        }
    }

    limpiarPantalla();
    if (estado.actual->esFinal){
        printf("¡Felicidades! Has llegado al final del laberinto.\n");
        printf("Puntaje total: %d\n", estado.puntaje);
    } else {
        printf("Se acabó el tiempo. Fin del juego.\n");
    }
    presioneTeclaParaContinuar();
    list_clean(estado.inventario); // Limpiar el inventario 
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