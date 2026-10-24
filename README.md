# CVRP con algoritmos bioinspirados

Este programa resuelve el **problema de ruteo de vehículos con capacidad (CVRP)**: dado un depósito, un conjunto de clientes con demanda y una flota de vehículos con capacidad limitada, encontrar las rutas que cubren a todos los clientes recorriendo la menor distancia posible.

Todo se maneja con un solo ejecutable, `cvrp`, que incluye cuatro métodos de solución:

| Método | Nombre en el programa | En pocas palabras |
|---|---|---|
| Modelo matemático | `modelo` | MILP de dos índices con restricciones MTZ levantadas, resuelto con HiGHS. Da el óptimo en instancias chicas, pero no escala. |
| Algoritmo propuesto | `propuesto` | El AG del Entregable 2, mejorado con 2-opt en cada ruta y descarte de soluciones duplicadas. |
| Algoritmo genético | `ag` | AG clásico: permutaciones, cruce PMX, mutación por intercambio y reemplazo de estado estable. Es el que se ajusta con 12 configuraciones. |
| Colonia de hormigas | `aco` | ASrank con flota libre, ahorros de Clarke y Wright como heurística, lista de vecinos y 2-opt. Es el que se ajusta con 8 configuraciones. |

---

## Inicio rápido

```bash
# 1. Instalar dependencias (macOS)
brew install highs cmake

# 2. Compilar
cmake -S . -B build
cmake --build build

# 3. Comprobar que todo funciona
./build/cvrp validar

# 4. Resolver una instancia
./build/cvrp resolver CMT1 --metodo aco --tiempo 10
```

> **Importante:** corre todos los comandos desde esta carpeta. Las rutas a `Reto_cvrp_primer/`, `validacion/`, `results/` y `tablas/` son relativas, así que desde otro lugar el programa no encuentra las instancias.

---

## Flujo para el entregable

Este es el orden recomendado para generar todos los resultados del reporte:

**1. Validar.** Corre la instancia didáctica CVRP_10 con los cuatro métodos. El modelo y el propuesto deben dar **317**, que es el óptimo conocido.

```bash
./build/cvrp validar
```

**2. Ajustar hiperparámetros.** Prueba las 12 configuraciones del AG y las 8 del ACO en las instancias de ajuste.

```bash
./build/cvrp ajuste-ag  --tiempo 30 --corridas 3
./build/cvrp ajuste-aco --tiempo 30 --corridas 3
```

Los resultados quedan en `results/ajuste_ag.csv` y `results/ajuste_aco.csv`.

**3. Comparar los cuatro métodos.** Con la mejor configuración que encontraste en el paso 2, corre todas las instancias:

```bash
./build/cvrp comparar --pm 0.05 --cruce 2 --seleccion torneo --alpha 1 --beta 5 --rho 0.1
```

(Cambia los valores por los que hayan ganado en tu ajuste.) Los resultados quedan en `results/comparacion.csv`.

**4. Generar las tablas para LaTeX.**

```bash
./build/cvrp tablas
```

Crea archivos `.tex` con formato booktabs en `tablas/`, listos para usar con `\input{}` en el reporte.

---

## Comandos

| Comando | Qué hace | Dónde deja el resultado |
|---|---|---|
| `resolver <instancia> --metodo M` | Corre una instancia con un método (`modelo`, `propuesto`, `ag` o `aco`), muestra cada corrida y la mejor solución | consola |
| `validar` | Corre CVRP_10 con los cuatro métodos como prueba de que todo funciona | consola |
| `ajuste-ag` | Corre las 12 configuraciones del AG (cruce × probabilidad de mutación × selección) | `results/ajuste_ag.csv` |
| `ajuste-aco` | Corre las 8 configuraciones del ACO (α × β × ρ) | `results/ajuste_aco.csv` |
| `comparar` | Corre los cuatro métodos en las 24 instancias | `results/comparacion.csv` |
| `tablas` | Convierte los CSV en tablas LaTeX | `tablas/*.tex` |

Puedes dar la instancia por nombre (`CMT1`, `X-n459-k26`, `CVRP_10`) o por su ruta.

---

## Opciones

### Generales

| Opción | Valor por defecto | Para qué sirve |
|---|---|---|
| `--tiempo S` | 30 | Segundos que dura cada corrida de un método estocástico |
| `--corridas K` | 3 | Cuántas veces se repite cada caso (con semillas distintas) |
| `--semilla S` | 1 | Semilla de la primera corrida; las siguientes usan S+1, S+2, ... |
| `--hilos H` | 5 | Cuántas corridas se ejecutan en paralelo |
| `--max-iter I` | 0 | Límite de iteraciones además del de tiempo (0 = sin límite) |
| `--instancias A B ...` | las del experimento | Correr solo ciertas instancias, por nombre o ruta |
| `--tiempo-modelo S` | 300 | Tiempo máximo que se le da a HiGHS |
| `--modelo-hasta-n N` | 100 | El modelo no se corre en instancias con más de N clientes, porque no terminaría |

### Del algoritmo genético

| Opción | Significado |
|---|---|
| `--cruce 1\|2` | Número de puntos de cruce |
| `--pm P` | Probabilidad de mutación |
| `--seleccion torneo\|ruleta` | Método de selección de padres |
| `--Np N` | Tamaño de la población |
| `--pc P` | Probabilidad de cruce |

### De la colonia de hormigas

| Opción | Significado |
|---|---|
| `--alpha A` | Peso de la feromona al elegir el siguiente cliente |
| `--beta B` | Peso de la distancia (qué tanto prefieren al cliente más cercano) |
| `--gamma G` | Peso del ahorro de Clarke y Wright |
| `--rho R` | Tasa de evaporación de la feromona |
| `--sigma S` | Número de hormigas élite que depositan feromona en ASrank |
| `--hormigas M` | Hormigas por iteración |
| `--candidatos K` | Tamaño de la lista de vecinos candidatos |

Los niveles del ajuste, qué instancias se usan para ajustar y todos los valores por defecto se cambian en `include/config.hpp`.

---

## Cómo leer la salida de `resolver`

```
CMT1  n = 50  Q = 160  K_min = 5  distancias reales  BKS = 524.61
aco (alpha=1;beta=5;rho=0.1)
  semilla 1   Z = 540.67     GAP    3.06 %  rutas 5   última mejora: it 8587 (1.0 s) de 82504  10.0 s
```

- **n**: número de clientes (el depósito es el nodo 1).
- **Q**: capacidad de cada vehículo.
- **K_min**: mínimo de vehículos necesarios, ⌈demanda total / Q⌉.
- **BKS**: mejor solución conocida en la literatura.
- **Z**: distancia total de la solución encontrada (menor es mejor).
- **GAP**: qué tan lejos está Z de la BKS, en porcentaje: (Z − BKS) / BKS × 100.
- **última mejora**: en qué iteración y segundo se encontró la mejor solución, y cuántas iteraciones se hicieron en total. Si la última mejora llega muy temprano, el algoritmo se estancó.

Al final se imprime cada ruta como una secuencia que sale del depósito y regresa a él.

---

## Instancias

- **`Reto_cvrp_primer/`**: CMT1 a CMT14 (Christofides, Mingozzi y Toth, 1979) y diez instancias X (Uchoa et al., 2017).
- **`validacion/CVRP_10.vrp`**: la instancia didáctica de los entregables 1 y 2, con óptimo 317.

Dos detalles a tener en cuenta:

- **Distancias.** Las CMT usan distancias reales sin redondear, porque así están calculadas sus BKS (viene indicado en el campo `COMMENT`). Las demás usan la distancia redondeada al entero más cercano, como en CVRPLIB.
- **Límite de longitud de ruta.** CMT6 a CMT10, CMT13 y CMT14 limitan cuánto puede medir cada ruta (campo `DISTANCE`, con un `SERVICE_TIME` por cliente). Los tres algoritmos respetan ese límite; el modelo matemático no lo impone, así que si su solución lo viola se marca como **no factible**.

---

## Organización del código

Las declaraciones (`.hpp`) están en `include/` y las implementaciones (`.cpp`) en `src/`:

| Archivo | Contenido |
|---|---|
| `config.hpp` | Niveles del ajuste, carpetas y valores por defecto |
| `instancia.hpp` / `.cpp` | Lectura de archivos `.vrp`, distancias, BKS, 2-opt y verificación de soluciones |
| `algoritmo.hpp` / `.cpp` | Interfaz común para los cuatro métodos y la fábrica que los crea |
| `modelo.hpp` / `.cpp` | El MILP con HiGHS |
| `geneticos.hpp` / `.cpp` | El AG clásico y el propuesto |
| `hormigas.hpp` / `.cpp` | La colonia de hormigas (ASrank) |
| `experimentos.hpp` / `.cpp` | Diseños experimentales, ejecución en paralelo y escritura de CSV |
| `tablas.hpp` / `.cpp` | Generación de tablas LaTeX |
| `main.cpp` | Punto de entrada y comandos |