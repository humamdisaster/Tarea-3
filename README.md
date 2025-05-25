# Tarea 3: GraphQuest
# Anyara Guajardo - Paralelo 2

---

## ¿Cómo se juega?
1. **Carga del laberinto**: el jugador debe elegir la primera opción del menú para cargar el archivo que contiene el laberinto
2. **Inicio de la partida**: El jugador comienza en un escenario inicial con un tiempo limitado de 10 unidades.
3. **Exploración**: En cada escenario, puede:
    - Recoger ítems (disminuye el tiempo)
    - Descartar ítems (aumenta el tiempo)
    - Avanzar a un escenario conectado
    - Reiniciar la partida
    - Salir del juego
4. **Objetivo**: Llegar al escenario final acumulando la mayor cantidad de puntos posibles, sin exceder el tiempo límite.

## Funcionalidades:
- Carga de escenarios desde archivo CSV.
- Lectura y procesamiento de ítems.
- Representación del mapa como grafo usando Map y List.
- Mostrar menú principal y menú del juego.
- Mostrar escenario actual e ítems.

## Consideraciones:
1. EL programa requiere un compilador C (como gcc).
2. Asegúrese de tener los siguientes archivos para que el juego funcione:
    - map.h / map.c: implementación de mapas ordenados
    - list.h / list.c: implementación de listas enlazadas
    - extra. h / extra.c: utilidades como lectura de CSV y manejo de strings
    - Archivo graphquest.csv: contiene el laberinto a utilizar

## Ejecución del Programa:
```bash
gcc main.c map.c list.c extra.c -o graphquest  #Compilación
./graphquest  #./graphquest
```

## Ejemplo de uso:
```bash
--- GraphQuest ---
1. Cargar laberinto
2. Iniciar partida
3. Salir
Selecciona una opcion del 1 al 3: 1

Carga completada. Total escenarios cargados: 16
Presione Enter para continuar...

...
Selecciona una opcion del 1 al 3: 2

Iniciando nueva partida...

--- PARTIDA INICIADA ---  
Tiempo inicial: 10
Ubicacion actual: Entrada principal

Presione Enter para continuar...

--- Entrada principal ---
Descripcion: Una puerta rechinante abre paso a esta mansion olvidada por los dioses y los conserjes. El aire huele a humedad y a misterios sin resolver.

Items disponibles:
(ninguno)

Direcciones disponibles:
 - abajo


Tiempo restante: 10
Puntaje: 0
Peso: 0
Inventario:

Opciones:
1. Recoger items
2. Descartar items
3. Avanzar
4. Reiniciar
5. Salir
Selecciona una opcion del 1 al 5 para continuar...

Accion > 3

Escribe una direccion para avanzar:
abajo

--- Cocina ---
Descripcion: Restos de una batalla culinaria...

Items disponibles:
 - Cuchillo (3 pts, 1 kg)
 - Pan (2 pts, 1 kg)

Direcciones disponibles:
 - arriba
 - abajo
 - derecha

Tiempo restante: 9  # -1 por movimiento base
Puntaje: 0
Peso: 0

Accion > 1
Selecciona un item para recoger:
1. Cuchillo
2. Pan
> 1

Tiempo restante: 8  # -1 por recoger item
Puntaje: 3  # +3 del cuchillo
Peso: 1    # +1 kg

Inventario:
 - Cuchillo (3 pts, 1 kg)

Accion > 3
Escribe una direccion para avanzar: derecha

# Tiempo consumido: ceil((1+1)/10) = 1
Tiempo restante: 7

--- Comedor ---
Descripcion: Una mesa elegante cubierta de polvo...

Items disponibles:
 - Copa dorada (8 pts, 3 kg)

Opciones:
...
Accion > 1
Selecciona un item para recoger:
1. Copa dorada
> 1

# Nuevo estado:
Puntaje: 11  # 3 + 8
Peso: 4     # 1 + 3
Tiempo restante: 6 //-1

# Movimiento con mayor peso:
Accion > 3
Escribe una direccion para avanzar: abajo

# Tiempo consumido: ceil((4+1)/10) = 1
Tiempo restante: 5

...

# Final del juego:
¡Felicidades! Has llegado al final del laberinto.
Puntaje total: 61
Tiempo restante: 1
Items recolectados:
 - Cuchillo (3 pts, 1 kg)
 - Copa dorada (8 pts, 3 kg)
 - Corona (50 pts, 10 kg)
 ```

 # Errores conocidos
 En ocaciones el programa solicita al jugador que presione una tecla para continuar, el programa puede tener un error en el que el mensaje se repita por segunda vez, el jugador solo debe presionar la tecla enter hasta que el programa continue.