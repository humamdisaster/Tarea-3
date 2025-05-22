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
    if (!itemsStr || strlen(itemsStr) == 0) return items;

    List *stringItems = split_string(itemsStr, ";");
    Item *item;
    char *itemStr = list_first(stringItems);
    while (itemStr != NULL) {
        char *paren = strchr(itemStr, '(');
        if (paren) {
            *paren = '\0';
            char *nombre = itemStr;
            int puntos, peso;
            if (sscanf(paren + 1, "%d pts, %d kg", &puntos, &peso) == 2) {
                item = malloc(sizeof(Item));
                strcpy(item->nombre, nombre);
                item->puntos = puntos;
                item->peso = peso;
                list_pushBack(items, item);
            }
        }
        itemStr = list_next(stringItems);
    }
    list_clean(stringItems);
    return items;
}

void cargarEscenarios(Map *escenarios, const char *nombreArchivo) {
    FILE *archivo = fopen(nombreArchivo, "r");
    if (!archivo) {
        perror("Error al abrir el archivo");
        presioneTeclaParaContinuar();
        return;
    }

    leer_linea_csv(archivo, ','); // Saltar cabecera

    char **campos;
    while ((campos = leer_linea_csv(archivo, ',')) != NULL) {
        Escenario *esc = malloc(sizeof(Escenario));
        if (!esc) {
            printf("Error de memoria\n");
            fclose(archivo);
            return;
        }
        
        esc->conexiones = map_create(NULL);
        esc->items = analizarElem(campos[3]);

        esc->id = atoi(campos[0]);
        strcpy(esc->nombre, campos[1]);
        strcpy(esc->descripcion, campos[2]);

        if (campos[4][0] != '\0') map_insert(esc->conexiones, "arriba", (void*)(long)atoi(campos[4]));
        if (campos[5][0] != '\0') map_insert(esc->conexiones, "abajo", (void*)(long)atoi(campos[5]));
        if (campos[6][0] != '\0') map_insert(esc->conexiones, "derecha", (void*)(long)atoi(campos[6]));
        if (campos[7][0] != '\0') map_insert(esc->conexiones, "izquierda", (void*)(long)atoi(campos[7]));

        esc->esFinal = (strcmp(campos[8], "Si") == 0);
        map_insert(escenarios, (void*)(long)esc->id, esc);
    }
    fclose(archivo);
    printf("Laberinto cargado exitosamente.\n");
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

void mostrarEscenario(Escenario *escenario){
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

void recogerItem(EstadoJuego *estado){  // Nombre consistente
    printf("Selecciona un item para recoger:\n");
    int i = 1;
    Item *item = list_first(estado->actual->items);
    while (item != NULL){
        printf("%d. %s\n", i++, item->nombre);
        item = list_next(estado->actual->items);
    }
    
    int seleccion;
    scanf("%d", &seleccion);
    
    // Buscar el item seleccionado
    item = list_first(estado->actual->items);
    for (int j = 1; j < seleccion && item != NULL; j++) {
        item = list_next(estado->actual->items);
    }
    
    if (item != NULL) {
        list_pushBack(estado->inventario, item);
        estado->puntaje += item->puntos;
        estado->peso += item->peso;
        
        // Eliminar de la lista original
        List *nuevosItems = list_create();
        Item *current = list_first(estado->actual->items);
        while (current != NULL) {
            if (current != item) {
                list_pushBack(nuevosItems, current);
            }
            current = list_next(estado->actual->items);
        }
        list_clean(estado->actual->items);
        estado->actual->items = nuevosItems;
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

void avanzar(EstadoJuego *estado, Map *escenarios) {
    printf("\nSelecciona una dirección para avanzar:\n");
    char direccion[16];
    scanf("%s", direccion);

    MapPair *pair = map_search(estado->actual->conexiones, direccion);
    if (pair == NULL) {
        printf("No puedes avanzar en esa dirección.\n");
        presioneTeclaParaContinuar();
        return;
    }

    int tiempoConsumido = (estado->peso + 1) / 10;
    if ((estado->peso + 1) % 10 != 0) tiempoConsumido++; // Redondear hacia arriba
    estado->tiempo -= tiempoConsumido;

    int id = (long)pair->value;
    estado->actual = (Escenario*)map_search(escenarios, (void*)(long)id);
}

void iniciarPartida(Map *escenarios){
    if (mapEmpty(escenarios)) {
        printf("Error: No hay laberinto cargado. Primero carga un laberinto.\n");
        presioneTeclaParaContinuar();
        return;
    }

    EstadoJuego estado;
    estado.actual = (Escenario*)map_search(escenarios, (void*)(long)1);
    if (estado.actual == NULL) {
        printf("No se pudo encontrar el escenario inicial.\n");
        presioneTeclaParaContinuar();
        return;
    }

    estado.inventario = list_create();
    if (!estado.inventario) {
        printf("Error: No se pudo crear el inventario\n");
        return;
    }
    estado.tiempo = TIEMPO_INCIAL;
    estado.puntaje = 0;
    estado.peso = 0;

    while (estado.tiempo > 0 && !estado.actual->esFinal){
        limpiarPantalla();
        mostrarEscenario(estado.actual);
        mostrarEstado(&estado);

        mostrarMenuJuego();
        int opcion;
        scanf("%d", &opcion);
        getchar(); //Limpiar el buffer de entrada

        switch (opcion){
            case 1:
                recogerItem(&estado);
                break;
            case 2:
                descartarItem(&estado);
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
                cargarEscenarios(escenarios, "graphquest.csv");
                presioneTeclaParaContinuar();
                break;
            case 2:
                if (!mapEmpty(escenarios)) {
                    iniciarPartida(escenarios);
                } else {
                    printf("Error: Primero debes cargar un laberinto (Opción 1)\n");
                }
                presioneTeclaParaContinuar();
                break;
            case 3:
                printf("Saliendo del juego...\n");
                break;
            default:
                printf("Opcion no valida. Intente de nuevo.\n");
                presioneTeclaParaContinuar();
                break;
        }
    } while (opcion != 3);

    MapPair *pair = map_first(escenarios);
    while (pair != NULL) {
        Escenario *esc = (Escenario*)pair->value;
        list_clean(esc->items);
        map_clean(esc->conexiones);
        free(esc);
        pair = map_next(escenarios);
    }
    map_clean(escenarios);

    return 0;
}