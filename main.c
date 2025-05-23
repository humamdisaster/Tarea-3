#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "extra.h"
#include "map.h"
#include "list.h"

#define MAX_LIN_LEN 1024
#define TIEMPO_INICIAL 20

typedef struct {
    char nombre[128];
    int puntos;
    int peso;
} Item;

typedef struct {
    int id;
    char nombre[128];
    char descripcion[512];
    List* items;
    Map* conexiones;
    bool esFinal;
} Escenario;

typedef struct {
    Escenario* actual;
    List* inventario;
    int tiempo;
    int puntaje;
    int peso;
} EstadoJuego;

int mapEmpty(Map* map) {
    return map_first(map) == NULL;
}

void mostrarMenu() {
    printf("\n--- GraphQuest ---\n");
    printf("1. Cargar laberinto\n");
    printf("2. Iniciar partida\n");
    printf("3. Salir\n");
    printf("Selecciona: ");
}

int compararClavesNumericas(void* clave1, void* clave2) {
    return ((long)clave1 == (long)clave2);
}

int compararClavesStrings(void* clave1, void* clave2) {
    return strcmp((char*)clave1, (char*)clave2) == 0;
}

List* analizarElem(char* itemsStr) {
    List* items = list_create();
    if (!items) {
        printf("ERROR: No se pudo crear la lista de items\n");
        return NULL;
    }

    if (!itemsStr || strlen(itemsStr) == 0) {
        return items;
    }

    List* stringItems = split_string(itemsStr, ";");
    if (!stringItems) {
        printf("ERROR: No se pudo dividir la cadena de items\n");
        list_clean(items);
        return NULL;
    }

    char* itemStr = list_first(stringItems);
    while (itemStr != NULL) {
        char* nombre = itemStr;
        char* ptr = itemStr;
        int puntos, peso;

        char* primeraComa = strchr(ptr, ',');
        if (!primeraComa) {
            itemStr = list_next(stringItems);
            continue;
        }
        *primeraComa = '\0';
        ptr = primeraComa + 1;

        char* segundaComa = strchr(ptr, ',');
        if (!segundaComa) {
            itemStr = list_next(stringItems);
            continue;
        }
        *segundaComa = '\0';

        puntos = atoi(ptr);
        peso = atoi(segundaComa + 1);

        Item* item = malloc(sizeof(Item));
        if (!item) {
            printf("ERROR: No se pudo asignar memoria para el item\n");
            itemStr = list_next(stringItems);
            continue;
        }

        strncpy(item->nombre, nombre, 127);
        item->nombre[127] = '\0';
        item->puntos = puntos;
        item->peso = peso;

        list_pushBack(items, item);
        itemStr = list_next(stringItems);
    }
    
    list_clean(stringItems);
    return items;
}

void cargarEscenarios(Map* escenarios, const char* nombreArchivo) {
    printf("\nCargando laberinto desde: %s\n", nombreArchivo);
    
    FILE* archivo = fopen(nombreArchivo, "r");
    if (!archivo) {
        perror("ERROR: No se pudo abrir el archivo");
        presioneTeclaParaContinuar();
        return;
    }

    char** campos = leer_linea_csv(archivo, ',');
    if (!campos) {
        printf("ERROR: Archivo CSV vacio o mal formado\n");
        fclose(archivo);
        return;
    }
    liberar_campos(campos);

    int escenariosCargados = 0;
    while ((campos = leer_linea_csv(archivo, ',')) != NULL) {
        int num_campos = 0;
        while (campos[num_campos] != NULL && num_campos < 9) num_campos++;
        
        if (num_campos < 9) {
            printf("ERROR: Fila mal formada, campos insuficientes (%d/9)\n", num_campos);
            liberar_campos(campos);
            continue;
        }

        Escenario* esc = malloc(sizeof(Escenario));
        if (!esc) {
            printf("ERROR: No se pudo asignar memoria para el escenario\n");
            liberar_campos(campos);
            continue;
        }

        esc->conexiones = map_create(compararClavesStrings);
        if (!esc->conexiones) {
            printf("ERROR: No se pudo crear el mapa de conexiones\n");
            free(esc);
            liberar_campos(campos);
            continue;
        }

        esc->items = analizarElem(campos[3]);
        if (!esc->items) {
            esc->items = list_create();
        }

        esc->id = atoi(campos[0]);
        strncpy(esc->nombre, campos[1], 127);
        strncpy(esc->descripcion, campos[2], 511);
        esc->nombre[127] = '\0';
        esc->descripcion[511] = '\0';

        if (campos[4][0] != '\0') {
            int arriba = atoi(campos[4]);
            if (arriba >= 0){
                map_insert(esc->conexiones, "arriba", (void*)(long)arriba);
            }
        }
        if (campos[5][0] != '\0') {
            int abajo = atoi(campos[5]);
            if (abajo >= 0){
                map_insert(esc->conexiones, "abajo", (void*)(long)abajo);
            }
        }
        if (campos[6][0] != '\0') {
            int derecha = atoi(campos[6]);
            if (derecha >= 0){
                map_insert(esc->conexiones, "derecha", (void*)(long)derecha);
            }
        }
        if (campos[7][0] != '\0') {
            int izquierda = atoi(campos[7]);
            if (izquierda >= 0){
                map_insert(esc->conexiones, "izquierda", (void*)(long)izquierda);
            }
        }

        esc->esFinal = (strcasecmp(campos[8], "Si") == 0);

        void* key = (void*)(long)esc->id;
        map_insert(escenarios, key, esc);
        escenariosCargados++;

        liberar_campos(campos);
    }

    fclose(archivo);
    printf("\nCarga completada. Total escenarios cargados: %d\n", escenariosCargados);
    presioneTeclaParaContinuar();
}

void mostrarMenuJuego(){
    printf("\nOpciones:\n");
    printf("1. Recoger items\n");
    printf("2. Descartar items\n");
    printf("3. Avanzar\n");
    printf("4. Reiniciar\n");
    printf("5. Salir\n");
    printf("Selecciona una opcion del 1 al 5 para continuar...\n");
}

void mostrarEscenario(Escenario *escenario){
    printf("\n--- %s ---\n", escenario->nombre);
    printf("Descripcion: %s\n", escenario->descripcion);

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

void recogerItem(EstadoJuego *estado) {
    printf("Selecciona un item para recoger:\n");
    int i = 1;
    Item *item = list_first(estado->actual->items);
    while (item != NULL) {
        printf("%d. %s\n", i++, item->nombre);
        item = list_next(estado->actual->items);
    }
    
    int seleccion;
    if (scanf("%d", &seleccion) != 1 || seleccion < 1 || seleccion > list_size(estado->actual->items)) {
        printf("Seleccion invalida\n");
        while (getchar() != '\n');
        return;
    }
    
    item = list_first(estado->actual->items);
    for (int j = 1; j < seleccion && item != NULL; j++) {
        item = list_next(estado->actual->items);
    }
    
    if (item != NULL) {
        Item *nuevoItem = malloc(sizeof(Item));
        if (!nuevoItem) {
            printf("ERROR: No se pudo asignar memoria para el item\n");
            return;
        }
        memcpy(nuevoItem, item, sizeof(Item));
        
        list_pushBack(estado->inventario, nuevoItem);
        estado->puntaje += item->puntos;
        estado->peso += item->peso;
        
        list_popCurrent(estado->actual->items);
    }
    estado->tiempo--;
}

void descartarItem(EstadoJuego *estado) {
    if (list_size(estado->inventario) == 0) {
        printf("No hay items en el inventario\n");
        return;
    }

    printf("\nSelecciona un item para descartar:\n");
    int i = 1;
    Item *item = list_first(estado->inventario);
    while (item != NULL) {
        printf("%d. %s\n", i++, item->nombre);
        item = list_next(estado->inventario);
    }

    int seleccion;
    if (scanf("%d", &seleccion) != 1 || seleccion < 1 || seleccion > list_size(estado->inventario)) {
        printf("Seleccion invalida\n");
        while (getchar() != '\n');
        return;
    }

    item = list_first(estado->inventario);
    for (int j = 1; j < seleccion && item != NULL; j++) {
        item = list_next(estado->inventario);
    }

    if (item != NULL) {
        estado->puntaje -= item->puntos;
        estado->peso -= item->peso;
        list_popCurrent(estado->inventario);
    }
    estado->tiempo--;
}

void avanzar(EstadoJuego *estado, Map *escenarios) {
    printf("\nSelecciona una direccion para avanzar:\n");
    char direccion[16];
    scanf("%s", direccion);

    MapPair *pair = map_search(estado->actual->conexiones, direccion);
    if (pair == NULL) {
        printf("No puedes avanzar en esa direccion.\n");
        presioneTeclaParaContinuar();
        return;
    }

    int id = (long)pair->value;
    
    MapPair* escPair = map_search(escenarios, (void*)(long)id);
    if (!escPair) {
        printf("Error: Escenario destino no existe\n");
        presioneTeclaParaContinuar();
        return;
    }

    int tiempoConsumido = (estado->peso + 1) / 10;
    if ((estado->peso + 1) % 10 != 0) tiempoConsumido++;
    estado->tiempo -= tiempoConsumido;

    estado->actual = (Escenario*)escPair->value;
}

void iniciarPartida(Map* escenarios) {
    printf("\nIniciando nueva partida...\n");
    
    if (mapEmpty(escenarios)) {
        printf("ERROR: No hay escenarios cargados\n");
        presioneTeclaParaContinuar();
        return;
    }

    MapPair* pair = map_search(escenarios, (void*)(long)1);
    if (pair == NULL) {
        printf("\nERROR: Escenario inicial no encontrado\n");
        presioneTeclaParaContinuar();
        return;
    }

    EstadoJuego estado;
    estado.actual = (Escenario*)pair->value;
    printf("\nEscenario inicial encontrado:\n");
    printf("ID: %d\n", estado.actual->id);
    printf("Nombre: %s\n", estado.actual->nombre);
    printf("Descripcion: %s\n", estado.actual->descripcion);

    estado.inventario = list_create();
    if (estado.inventario == NULL) {
        printf("ERROR: No se pudo crear el inventario\n");
        return;
    }

    estado.tiempo = TIEMPO_INICIAL;
    estado.puntaje = 0;
    estado.peso = 0;
    
    printf("\n--- PARTIDA INICIADA ---\n");
    printf("Tiempo inicial: %d\n", estado.tiempo);
    printf("Ubicacion actual: %s\n", estado.actual->nombre);
    presioneTeclaParaContinuar();

    while (estado.tiempo > 0 && !estado.actual->esFinal) {
        limpiarPantalla();
        mostrarEscenario(estado.actual);
        mostrarEstado(&estado);
        mostrarMenuJuego();
        
        printf("\nAccion > ");
        int opcion;
        if (scanf("%d", &opcion) != 1) {
            printf("Entrada invalida\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        switch (opcion) {
            case 1: recogerItem(&estado); break;
            case 2: descartarItem(&estado); break;
            case 3: avanzar(&estado, escenarios); break;
            case 4: 
                list_clean(estado.inventario);
                iniciarPartida(escenarios);
                return;
            case 5: 
                list_clean(estado.inventario);
                return;
            default:
                printf("Opcion no valida\n");
                presioneTeclaParaContinuar();
        }
    }

    limpiarPantalla();
    if (estado.actual->esFinal) {
        printf("¡Felicidades! Has llegado al final del laberinto.\n");
        printf("Puntaje total: %d\n", estado.puntaje);
    } else {
        printf("¡Se acabo el tiempo! Fin del juego.\n");
    }
    list_clean(estado.inventario);
    presioneTeclaParaContinuar();
}

int main() {
    Map* escenarios = map_create(compararClavesNumericas);
    if (!escenarios) {
        printf("ERROR CRITICO: No se pudo crear el mapa de escenarios\n");
        return 1;
    }

    int opcion;
    do {
        limpiarPantalla();
        mostrarMenu();
        
        if (scanf("%d", &opcion) != 1) {
            printf("ERROR: Entrada invalida\n");
            while (getchar() != '\n');
            continue;
        }
        getchar();

        switch(opcion) {
            case 1: 
                cargarEscenarios(escenarios, "graphquest.csv");
                presioneTeclaParaContinuar();
                break;
            case 2: 
                if (!mapEmpty(escenarios)) {
                    iniciarPartida(escenarios);
                } else {
                    printf("ERROR: Primero debes cargar un laberinto\n");
                }
                presioneTeclaParaContinuar();
                break;
            case 3: 
                printf("Saliendo del juego...\n");
                break;
            default:
                printf("ERROR: Opcion no valida\n");
                presioneTeclaParaContinuar();
        }
    } while (opcion != 3);

    MapPair* pair = map_first(escenarios);
    while (pair) {
        Escenario* esc = (Escenario*)pair->value;
        if (esc) {
            if (esc->items) list_clean(esc->items);
            if (esc->conexiones) map_clean(esc->conexiones);
            free(esc);
        }
        pair = map_next(escenarios);
    }
    map_clean(escenarios);

    return 0;
}