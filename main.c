#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include "extra.h"
#include "map.h"
#include "list.h"

#define MAX_LIN_LEN 1024
#define TIEMPO_INICIAL 10

 //Estructura para almacenar los items del juego
typedef struct{
    char nombre[128]; //Nombre del item
    int puntos; //Puntos que otorga el item
    int peso; //Peso del item
} Item;

//Estructura para almacenar los escenarios del juego
typedef struct{
    int id; //ID del escenario
    char nombre[128]; //Nombre del escenario
    char descripcion[512]; //Descripcion del escenario
    List* items; //Lista de items disponibles en el escenario
    Map* conexiones; //Mapa de conexiones a otros escenarios
    bool esFinal; //Indica si el escenario es el final del juego
} Escenario; //Grafo

//Estructura para almacenar el estado del juego
typedef struct{
    Escenario* actual; //Escenario actual del juego
    List* inventario; //Lista de items en el inventario
    int tiempo; //Tiempo restante para completar el juego
    int puntaje; //Puntaje acumulado por el jugador
    int peso; //Peso total de los items en el inventario
} EstadoJuego;

//Funciones auxiliares
int mapEmpty(Map* map){
    return map_first(map) == NULL;
}

//Funcion para mostrar el menú principal del programa
void mostrarMenu(){
    printf("\n--- GraphQuest ---\n");
    printf("1. Cargar laberinto\n");
    printf("2. Iniciar partida\n");
    printf("3. Salir\n");
    printf("Selecciona una opcion del 1 al 3: ");
}

//Funcion que compara claves numericas
int compararClavesNumericas(void* clave1, void* clave2){
    return ((long)clave1 == (long)clave2);
}
//Funcion que compara claves de tipo string
int compararClavesStrings(void* clave1, void* clave2){
    return strcmp((char*)clave1, (char*)clave2) == 0;
}

//Funcion que procesa cadena de items
// y devuelve una lista de items
List* analizarElem(char* itemsStr){
    List* items = list_create(); // Crear la lista de items
    if (!items) { // Verificar si la lista se creó correctamente
        printf("ERROR: No se pudo crear la lista de items\n");
        return NULL;
    }
    // Verificar si la cadena de items es nula o vacía
    if (!itemsStr || strlen(itemsStr) == 0) {
        return items;
    }
    // Dividir la cadena de items por el delimitador ";"
    List* stringItems = split_string(itemsStr, ";");
    if (!stringItems){ // Verificar si la división fue exitosa
        printf("ERROR: No se pudo dividir la cadena de items\n");
        list_clean(items);
        return NULL;
    }
    // Iterar sobre cada item en la lista de cadenas
    // y crear un objeto Item para cada uno
    char* itemStr = list_first(stringItems);
    while (itemStr != NULL) {
        char* nombre = itemStr;
        char* ptr = itemStr;
        int puntos, peso;
        // Buscar la primera coma
        char* primeraComa = strchr(ptr, ',');
        if (!primeraComa){
            itemStr = list_next(stringItems);
            continue;
        }
        // Reemplazar la coma por un terminador de cadena
        *primeraComa = '\0';
        ptr = primeraComa + 1;
        // Buscar la segunda coma
        char* segundaComa = strchr(ptr, ',');
        if (!segundaComa){ // Si no hay segunda coma, continuar con el siguiente item
            itemStr = list_next(stringItems);
            continue;
        }
        // Reemplazar la segunda coma por un terminador de cadena
        *segundaComa = '\0';
        // Convertir los valores a enteros
        puntos = atoi(ptr);
        peso = atoi(segundaComa + 1);
        // Crear un nuevo item
        // y agregarlo a la lista de items
        Item* item = malloc(sizeof(Item));
        if (!item){ // Verificar si la asignación de memoria fue exitosa
            printf("ERROR: No se pudo asignar memoria para el item\n");
            itemStr = list_next(stringItems);
            continue;
        }
        //Copiar los valores al nuevo item
        strncpy(item->nombre, nombre, 127); //Copiar el nombre del item
        item->nombre[127] = '\0'; //Asegurarse de que la cadena esté terminada
        item->puntos = puntos; //Asignar los puntos
        item->peso = peso; //Asignar el peso

        list_pushBack(items, item); //Agregar el item a la lista de items
        itemStr = list_next(stringItems); //Pasar al siguiente item
    }
    //Liberar la lista de cadenas
    //y devolver la lista de items
    list_clean(stringItems);
    return items;
}

//Funcion para cargar los escenarios desde un archivo CSV
//y almacenarlos en un mapa
void cargarEscenarios(Map* escenarios, const char* nombreArchivo){
    FILE* archivo = fopen(nombreArchivo, "r"); // Abrir el archivo CSV
    if (!archivo){ // Verificar si el archivo se abrió correctamente
        perror("ERROR: No se pudo abrir el archivo");
        presioneTeclaParaContinuar();
        return;
    }
    char** campos = leer_linea_csv(archivo, ','); //Leer la primera línea (encabezados)
    if (!campos){ //Verificar si la lectura fue exitosa
        printf("ERROR: Archivo CSV vacio o mal formado\n");
        fclose(archivo);
        return;
    }
    liberar_campos(campos); //Liberar los campos leídos

    int escenariosCargados = 0; //Contador de escenarios cargados
    //Leer cada línea del archivo CSV
    while ((campos = leer_linea_csv(archivo, ',')) != NULL){
        int num_campos = 0;
        while (campos[num_campos] != NULL && num_campos < 9) num_campos++;
        //Verificar si la línea tiene el número correcto de campos
        if (num_campos < 9){
            printf("ERROR: Fila mal formada, campos insuficientes\n");
            liberar_campos(campos);
            continue;
        }
        //Crear un nuevo escenario
        //y asignar memoria para él
        Escenario* esc = malloc(sizeof(Escenario));
        if (!esc) {
            printf("ERROR: No se pudo asignar memoria para el escenario\n");
            liberar_campos(campos);
            continue;
        }
        //Inicializar el escenario
        esc->conexiones = map_create(compararClavesStrings);
        if (!esc->conexiones){ //Verificar si el mapa se creó correctamente
            printf("ERROR: No se pudo crear el mapa de conexiones\n");
            free(esc);  //Liberar la memoria del escenario
            liberar_campos(campos); //Liberar los campos leídos
            continue; //Continuar con el siguiente escenario
        }
        //Asignar los valores leídos a los campos del escenario
        esc->items = analizarElem(campos[3]);
        if (!esc->items){ //Verificar si la lista de items se creó correctamente
            esc->items = list_create();
        }
        //Asignar el ID, nombre y descripción del escenario
        esc->id = atoi(campos[0]);
        strncpy(esc->nombre, campos[1], 127);
        strncpy(esc->descripcion, campos[2], 511);
        esc->nombre[127] = '\0'; //Asegurarse de que la cadena esté terminada
        esc->descripcion[511] = '\0'; //Asegurarse de que la cadena esté terminada
        
        //Asignar las conexiones del escenario
        if (campos[4][0] != '\0'){ //Verificar si el campo no está vacío
            int arriba = atoi(campos[4]); // Convertir a entero
            if (arriba >= 0){ //Verificar si el valor es válido
                map_insert(esc->conexiones, "arriba", (void*)(long)arriba); // Guardar la conexión
            }
        }
        if (campos[5][0] != '\0'){ //Verificar si el campo no está vacío
            int abajo = atoi(campos[5]); //Convertir a entero
            if (abajo >= 0){ //Verificar si el valor es válido
                map_insert(esc->conexiones, "abajo", (void*)(long)abajo); //Guardar la conexión
            }
        }
        if (campos[6][0] != '\0') { //Verificar si el campo no está vacío
            int izquierda = atoi(campos[6]); //Convertir a entero
            if (izquierda >= 0){ //Verificar si el valor es válido
                map_insert(esc->conexiones, "izquierda", (void*)(long)izquierda); //Guardar la conexión
            }
        }
        if (campos[7][0] != '\0'){ //Verificar si el campo no está vacío
            int derecha = atoi(campos[7]); //Convertir a entero
            if (derecha >= 0){ //Verificar si el valor es válido
                map_insert(esc->conexiones, "derecha", (void*)(long)derecha); //Guardar la conexión
            }
        }
        //Asignar si el escenario es final
        esc->esFinal = (strcasecmp(campos[8], "Si") == 0);

        //Agregar el escenario al grafo de escenarios
        void* key = (void*)(long)esc->id; //Convertir el ID a puntero
        map_insert(escenarios, key, esc); //Insertar el escenario en el mapa
        escenariosCargados++; //Incrementar el contador de escenarios cargados

        liberar_campos(campos); //Liberar los campos leídos
    }

    fclose(archivo); //Cerrar el archivo CSV
    printf("\nCarga completada. Total escenarios cargados: %d\n", escenariosCargados); // Mostrar el total de escenarios cargados
    presioneTeclaParaContinuar(); //Esperar a que el usuario presione una tecla
}

//Funcion que muestra el menú del juego
void mostrarMenuJuego(){
    printf("\nOpciones:\n");
    printf("1. Recoger items\n");
    printf("2. Descartar items\n");
    printf("3. Avanzar\n");
    printf("4. Reiniciar\n");
    printf("5. Salir\n");
    printf("Selecciona una opcion del 1 al 5 para continuar...\n");
}

//Funcion que muestra el escenario actual
//y los items disponibles en él
void mostrarEscenario(Escenario *escenario){
    printf("\n--- %s ---\n", escenario->nombre); //Mostrar el nombre del escenario
    printf("Descripcion: %s\n", escenario->descripcion); //Mostrar la descripción del escenario

    printf("\nItems disponibles:\n"); //Mostrar los items disponibles
    //Iterar sobre la lista de items
    //y mostrar sus nombres, puntos y peso
    for (Item *item = list_first(escenario->items); item != NULL; item = list_next(escenario->items)){
        printf(" - %s (%d pts, %d kg)\n", item->nombre, item->puntos, item->peso);
    }
    //Mostrar las conexiones disponibles
    //Iterar sobre el mapa de conexiones
    printf("\nDirecciones disponibles:\n");
    MapPair *pair = map_first(escenario->conexiones);
    while (pair != NULL){
        printf(" - %s\n", (char*)pair->key); //Mostrar la dirección
        pair = map_next(escenario->conexiones); //Pasar al siguiente par clave-valor
    }
    printf("\n");
}

//Funcion que muestra el estado actual del juego
//y los items en el inventario
void mostrarEstado(EstadoJuego *estado){
    printf("\nTiempo restante: %d\n", estado->tiempo); //Mostrar el tiempo restante
    printf("Puntaje: %d\n", estado->puntaje); //Mostrar el puntaje acumulado
    printf("Peso: %d\n", estado->peso); //Mostrar el peso total de los items en el inventario

    printf("Inventario:\n");
    //Iterar sobre la lista de items en el inventario
    //y mostrar sus nombres, puntos y peso
    for (Item *item = list_first(estado->inventario); item != NULL; item = list_next(estado->inventario)){
        printf(" - %s (%d pts, %d kg)\n", item->nombre, item->puntos, item->peso);
    }
}

//Funcion que agrega un item al inventario
//y lo elimina del escenario actual
void recogerItem(EstadoJuego *estado){
    printf("Selecciona un item para recoger:\n");
    int i = 1;
    Item *item = list_first(estado->actual->items); //Obtener el primer item
    //Iterar sobre la lista de items
    while (item != NULL){
        printf("%d. %s\n", i++, item->nombre); //Mostrar el nombre del item
        item = list_next(estado->actual->items); //Pasar al siguiente item
    }
    
    int seleccion;
    //Leer la selección del usuario
    //y verificar si es válida
    if (scanf("%d", &seleccion) != 1 || seleccion < 1 || seleccion > list_size(estado->actual->items)){
        printf("Seleccion invalida\n"); //Mostrar mensaje de error
        while (getchar() != '\n'); //Limpiar buffer
        return; //Salir de la función
    }
    while (getchar() != '\n');  // Limpiar buffer después de scanf exitoso
    
    item = list_first(estado->actual->items); //Obtener el primer item de la lista
    //Iterar sobre la lista de items
    for (int j = 1; j < seleccion && item != NULL; j++){
        item = list_next(estado->actual->items); //Pasar al siguiente item
    }
    
    if (item != NULL){ //Verificar si el item es válido
        //Crear un nuevo item
        Item *nuevoItem = malloc(sizeof(Item));
        if (!nuevoItem){ //Verificar si la asignación de memoria fue exitosa
            printf("ERROR: No se pudo asignar memoria para el item\n");
            return;
        }
        memcpy(nuevoItem, item, sizeof(Item)); //Copiar los valores del item
        
        list_pushBack(estado->inventario, nuevoItem); //Agregar el nuevo item al inventario
        estado->puntaje += item->puntos; //Actualizar el puntaje
        estado->peso += item->peso; //Actualizar el peso total
        
        list_popCurrent(estado->actual->items); //Eliminar el item del escenario actual
    }
    estado->tiempo--; //Disminuir el tiempo restante
}

//Funcion que descarta un item del inventario
//y lo elimina
void descartarItem(EstadoJuego *estado){
    if (list_size(estado->inventario) == 0){ //Verificar si el inventario está vacío
        printf("No hay items en el inventario\n");
        return;
    }

    printf("\nSelecciona un item para descartar:\n");
    int i = 1;
    Item *item = list_first(estado->inventario); //Obtener el primer item
    //Iterar sobre la lista de items
    while (item != NULL){
        printf("%d. %s\n", i++, item->nombre); //Mostrar el nombre del item
        item = list_next(estado->inventario); //Pasar al siguiente item
    }

    int seleccion;
    scanf("%d", &seleccion); //Leer la selección del usuario
    //Verificar si la selección es válida
    if (seleccion != 1 || seleccion < 1 || seleccion > list_size(estado->inventario)){
        printf("Seleccion invalida\n");
        while (getchar() != '\n');
        return; //Salir de la función
    }
    while (getchar() != '\n');  // Limpiar buffer después de scanf exitoso

    item = list_first(estado->inventario); //Obtener el primer item
    //Iterar sobre la lista de items
    for (int j = 1; j < seleccion && item != NULL; j++){
        item = list_next(estado->inventario); //Pasar al siguiente item
    }
    if (item != NULL){
        estado->puntaje -= item->puntos; //Actualizar el puntaje
        estado->peso -= item->peso; //Actualizar el peso total
        list_popCurrent(estado->inventario); //Eliminar el item del inventario
    }
    estado->tiempo--; //Disminuir el tiempo restante
}

//Funcion que permite avanzar a otro escenario
//y actualizar el estado del juego
void avanzar(EstadoJuego *estado, Map *escenarios){
    printf("\nEscribe una direccion para avanzar:\n");
    char direccion[16];
    
    //Verificar si la dirección es válida
    if (scanf("%15s", direccion) !=  1){
        printf("Error al leer la direccion\n");
        while (getchar() != '\n'); //Limpiar buffer
        return;
    }
    while (getchar() != '\n'); //Limpiar buffer

    MapPair *pair = map_search(estado->actual->conexiones, direccion); //Buscar la dirección en el mapa de conexiones
    if (pair == NULL){ //Si la dirección no es válida
        printf("No puedes avanzar en esa direccion.\n");
        presioneTeclaParaContinuar();
        return; //Salir de la función
    }

    int id = (long)pair->value; //Obtener el ID del escenario destino
    MapPair* escPair = map_search(escenarios, (void*)(long)id); //Buscar el escenario destino en el mapa de escenarios
    if (!escPair){ //Si el escenario destino no existe
        printf("Error: Escenario destino no existe\n");
        presioneTeclaParaContinuar();
        return; //Salir de la función
    }

    int tiempoConsumido = (int)ceil((estado->peso + 1) / 10.0); // Calcular el tiempo consumido según formula dada y redondear hacia arriba
    estado->tiempo -= tiempoConsumido; //Disminuir el tiempo restante

    estado->actual = (Escenario*)escPair->value; //Actualizar el escenario actual
}

//Funcion principal del juego
//Inicia una nueva partida
//y permite al jugador interactuar con el juego
void iniciarPartida(Map* escenarios){
    printf("\nIniciando nueva partida...\n");
    
    if (mapEmpty(escenarios)){ //Verificar si hay escenarios cargados
        printf("ERROR: No hay escenarios cargados\n");
        presioneTeclaParaContinuar();
        return; //Salir de la función
    }

    MapPair* pair = map_search(escenarios, (void*)(long)1); //Buscar el escenario inicial
    if (pair == NULL){ //Si no se encuentra el escenario inicial
        printf("\nERROR: Escenario inicial no encontrado\n");
        presioneTeclaParaContinuar();
        return; //Salir de la función
    }

    EstadoJuego estado; //Crear un nuevo estado del juego
    estado.actual = (Escenario*)pair->value; //Asignar el escenario inicial

    estado.inventario = list_create(); //Crear la lista del inventario
    if (estado.inventario == NULL) { //Verificar si la lista se creó correctamente
        printf("ERROR: No se pudo crear el inventario\n");
        return; //Salir de la función
    }

    estado.tiempo = TIEMPO_INICIAL; //Asignar el tiempo inicial
    estado.puntaje = 0; //Inicializar el puntaje
    estado.peso = 0; //Inicializar el peso total
    
    //Mostrar el estado inicial del juego
    printf("\n--- PARTIDA INICIADA ---\n");
    printf("Tiempo inicial: %d\n", estado.tiempo);
    printf("Ubicacion actual: %s\n", estado.actual->nombre);
    presioneTeclaParaContinuar();

    //Bucle principal del juego
    //Mientras haya tiempo y el escenario no sea final
    while (estado.tiempo > 0 && !estado.actual->esFinal){
        limpiarPantalla();
        mostrarEscenario(estado.actual); //Mostrar el escenario actual
        mostrarEstado(&estado); //Mostrar el estado del juego
        mostrarMenuJuego(); //Mostrar el menú del juego
        
        printf("\nAccion > ");
        int opcion;
        scanf("%d", &opcion); //Leer la opción del usuario
        if (opcion < 1 || opcion > 5){ //Verificar si la opción es válida
            printf("Entrada invalida\n");
            while (getchar() != '\n');
            continue; //Continuar con el siguiente ciclo
        }
        while (getchar() != '\n');  // Limpiar buffer después de scanf exitoso

        switch (opcion){ //Procesar la opción seleccionada
            case 1: 
                recogerItem(&estado); 
                break; //Recoger un item
            case 2: 
                descartarItem(&estado); 
                break; //Descartar un item
            case 3: 
                avanzar(&estado, escenarios); 
                break; //Avanzar a otro escenario
            case 4: 
                list_clean(estado.inventario); //Reiniciar la partida
                iniciarPartida(escenarios); // Iniciar una nueva partida
                return; //Salir de la función
            case 5: 
                list_clean(estado.inventario); //Salir del juego
                return; //Salir de la función
            default: //Si la opción no es válida
                printf("Opcion no valida\n");
                presioneTeclaParaContinuar();
        }
    }

    limpiarPantalla();
    if (estado.actual->esFinal){ //Si el escenario actual es el final
        printf("¡Felicidades! Has llegado al final del laberinto.\n"); //Mostrar mensaje de éxito
        printf("Puntaje total: %d\n", estado.puntaje); //Mostrar el puntaje total
    } else{ //Si se acabó el tiempo
        printf("¡Se acabo el tiempo! Fin del juego.\n"); //Mostrar mensaje de fin de juego
    }
    list_clean(estado.inventario); //Limpiar el inventario
    presioneTeclaParaContinuar();
}

//Funcion principal
int main(){
    Map* escenarios = map_create(compararClavesNumericas); //Crear el grafo de escenarios
    if (!escenarios){ //Verificar si el mapa se creó correctamente
        printf("ERROR CRITICO: No se pudo crear el mapa de escenarios\n");
        return 1; //Salir del programa con error
    }

    int opcion;
    do{
        limpiarPantalla();
        mostrarMenu(); //Mostrar el menú principal
        scanf("%d", &opcion); //Leer la opción del usuario
        if (opcion < 1 || opcion > 3){ //Si la opción no es válida
            printf("ERROR: Entrada invalida\n");
            presioneTeclaParaContinuar();
            while (getchar() != '\n');
            continue; //Continuar con el siguiente ciclo
        }
        while (getchar() != '\n');  // Limpiar buffer después de scanf exitoso
        
        //Procesar la opción seleccionada
        switch(opcion){
            case 1: 
                cargarEscenarios(escenarios, "graphquest.csv"); //Cargar los escenarios desde el archivo CSV
                presioneTeclaParaContinuar();
                break;
            case 2: 
                if (!mapEmpty(escenarios)){ //Si hay escenarios cargados
                    iniciarPartida(escenarios); //Iniciar una nueva partida
                } else{ //Si no hay escenarios cargados
                    printf("ERROR: Primero debes cargar un laberinto\n");
                }
                presioneTeclaParaContinuar();
                break;
            case 3: 
                printf("Saliendo del juego...\n");
                break; //Salir del juego
            default: //Si la opción no es válida
                printf("ERROR: Opcion no valida\n");
                presioneTeclaParaContinuar();
        }
    } while (opcion != 3); //Mientras la opción no sea salir

    //Limpiar la memoria
    MapPair* pair = map_first(escenarios); //Obtener el primer par clave-valor
    //Iterar sobre el mapa de escenarios
    while (pair){
        Escenario* esc = (Escenario*)pair->value; //Obtener el escenario
        if (esc){ //Verificar si el escenario es válido
            if (esc->items) list_clean(esc->items); //Limpiar la lista de items
            if (esc->conexiones) map_clean(esc->conexiones); //Limpiar el mapa de conexiones
            free(esc); //Liberar la memoria del escenario
        }
        pair = map_next(escenarios); //Pasar al siguiente par clave-valor
    }
    map_clean(escenarios); //Limpiar el mapa de escenarios
    printf("Juego terminado. Gracias por jugar.\n");

    return 0; //Salir del programa
}