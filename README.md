# CVRP con algoritmos bioinspirados (Entregable 3)

Equipo 2, MA2015 Gpo 202. Un programa en C++ (`cvrp`) resuelve el CVRP con cuatro métodos y corre los experimentos del reporte:

- **modelo**: MILP de dos índices con MTZ levantado, resuelto con HiGHS y con límite de tiempo.
- **propuesto**: AG del Entregable 2, con 2-opt por ruta y descarte de duplicados.
- **ag**: AG clásico (permutación, PMX, mutación por intercambio, estado estable); a este se le hace el ajuste de 12 configuraciones.
- **aco**: ASrank con flota libre, ahorro de Clarke y Wright, lista de vecinos y 2-opt; a este se le hace el ajuste de 8 configuraciones.

## Requisitos y compilación

```bash
brew install highs cmake
cmake -S . -B build
cmake --build build
```

Todos los comandos se corren desde esta carpeta, porque las rutas a `Reto_cvrp_primer/`, `validacion/`, `results/` y `tablas/` son relativas.

## Comandos

| Comando | Qué hace | Salida |
|---|---|---|
| `resolver <instancia> --metodo M` | Corre una instancia con un método (`modelo`, `propuesto`, `ag`, `aco`) e imprime cada corrida y la mejor solución | consola |
| `validar` | CVRP_10 con los cuatro métodos; el modelo y el propuesto deben dar 317 | consola |
| `ajuste-ag` | 12 configuraciones del AG (cruce × p_m × selección) | `results/ajuste_ag.csv` |
| `ajuste-aco` | 8 configuraciones del ACO (α × β × ρ) | `results/ajuste_aco.csv` |
| `comparar` | Los cuatro métodos en las 24 instancias | `results/comparacion.csv` |
| `tablas` | Tablas LaTeX (booktabs) a partir de los CSV | `tablas/*.tex` |

La instancia se puede dar por nombre (`CMT1`, `X-n459-k26`, `CVRP_10`) o por ruta.

## Opciones

Generales:

| Opción | Por defecto | Significado |
|---|---|---|
| `--instancias A B ...` | las del experimento | nombres o rutas |
| `--tiempo S` | 30 | segundos por corrida estocástica |
| `--tiempo-modelo S` | 300 | límite de HiGHS |
| `--modelo-hasta-n N` | 100 | no correr el modelo con más de N clientes |
| `--corridas K` | 3 | corridas por caso |
| `--semilla S` | 1 | semilla de la primera corrida; las siguientes son S+1, S+2, ... |
| `--hilos H` | 5 | corridas en paralelo |
| `--max-iter I` | 0 | límite de iteraciones además del tiempo (0 = sin límite) |

AG: `--pm P`, `--cruce 1|2`, `--seleccion torneo|ruleta`, `--Np N`, `--pc P`.

ACO: `--alpha A`, `--beta B`, `--gamma G`, `--rho R`, `--sigma S`, `--hormigas M`, `--candidatos K`.

Los niveles del ajuste, las instancias de ajuste y los valores por defecto están en `include/config.hpp`.

## Ejemplos

```bash
./build/cvrp validar
./build/cvrp resolver CMT1 --metodo aco --tiempo 10
./build/cvrp resolver CMT1 --metodo modelo --tiempo-modelo 60
./build/cvrp ajuste-ag --tiempo 30 --corridas 3
./build/cvrp ajuste-aco --tiempo 30 --corridas 3
./build/cvrp comparar --pm 0.05 --cruce 2 --seleccion torneo --alpha 1 --beta 5 --rho 0.1
./build/cvrp tablas
```

## Resultados

Cada corrida se agrega a su CSV en cuanto termina. Si un experimento se interrumpe, al lanzarlo de nuevo se saltan las corridas que ya están en el CSV.

Columnas del CSV: `experimento, instancia, n, metodo, config, semilla, Z, bks, gap_bks, cota, estado, rutas, segundos, t_mejora, it_mejora, iteraciones, factible`.

- `gap_bks` = 100 (Z − BKS) / BKS.
- `t_mejora` e `it_mejora` son el segundo y la iteración de la última mejora; en el modelo, `iteraciones` es el número de nodos de ramificación.
- `estado` del modelo: `optimo`, `factible`, `sin solucion` o `no escala`.

Además se guardan el historial de convergencia (`results/historial/`) y la solución en formato `.sol` de CVRPLIB (`results/soluciones/`) de cada corrida.

## Instancias

- `Reto_cvrp_primer/`: CMT1 a CMT14 (Christofides, Mingozzi y Toth, 1979) y diez instancias X (Uchoa et al., 2017).
- `validacion/CVRP_10.vrp`: instancia didáctica de los entregables 1 y 2 (óptimo 317).

Las CMT usan distancias reales, porque así están calculadas sus BKS (campo `COMMENT`); las demás usan la distancia redondeada al entero más cercano, como en CVRPLIB. CMT6 a CMT10, CMT13 y CMT14 tienen un límite de longitud de ruta (`DISTANCE`, con `SERVICE_TIME` por cliente), que respetan los tres algoritmos; el modelo no lo impone, y si su solución lo viola queda marcada como no factible.

## Estructura

Las declaraciones (`.hpp`) están en `include/` y las implementaciones (`.cpp`) en `src/`:

```
include/config.hpp         niveles, carpetas y valores por defecto
include/instancia.hpp      src/instancia.cpp      lectura de .vrp, distancias, BKS, 2-opt, verificación
include/algoritmo.hpp      src/algoritmo.cpp      interfaz común y fábrica de métodos
include/modelo.hpp         src/modelo.cpp         MILP con HiGHS
include/geneticos.hpp      src/geneticos.cpp      AG clásico y propuesto
include/hormigas.hpp       src/hormigas.cpp       ASrank
include/experimentos.hpp   src/experimentos.cpp   diseños experimentales, ejecución en paralelo, CSV
include/tablas.hpp         src/tablas.cpp         tablas LaTeX
                           src/main.cpp           comandos
```
