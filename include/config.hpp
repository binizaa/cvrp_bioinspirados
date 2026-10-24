// Parámetros del proyecto en un solo lugar: carpetas, niveles del ajuste de
// parámetros y valores por defecto de cada algoritmo.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Carpetas, relativas a la carpeta desde donde se corre el programa
inline const std::string DIR_INSTANCIAS = "Reto_cvrp_primer";
inline const std::string DIR_VALIDACION = "validacion";
inline const std::string DIR_RESULTADOS = "results";
inline const std::string DIR_TABLAS = "tablas";

// Niveles del ajuste del AG (2 x 3 x 2 = 12 configuraciones)
inline const std::vector<int> NIVELES_CRUCE = {1, 2};
inline const std::vector<double> NIVELES_PM = {0.01, 0.05, 0.1};
inline const std::vector<std::string> NIVELES_SELECCION = {"torneo", "ruleta"};

// Niveles del ajuste del ACO (2 x 2 x 2 = 8 configuraciones)
inline const std::vector<double> NIVELES_ALPHA = {1, 5};
inline const std::vector<double> NIVELES_BETA = {2, 5};
inline const std::vector<double> NIVELES_RHO = {0.1, 0.25};

// Instancias del ajuste: una CMT aleatoria, una CMT agrupada y una X
inline const std::vector<std::string> INSTANCIAS_AJUSTE = {"CMT1", "CMT11", "X-n280-k17"};

struct Opciones {
    // AG clásico; pm, cruce y seleccion se cambian por los mejores niveles del ajuste
    int Np = 100;
    double pc = 0.9;
    double pm = 0.1;
    int cruce = 2;
    std::string seleccion = "torneo";

    // Algoritmo propuesto: parámetros del Entregable 2
    double pm_propuesto = 0.2;
    int cruce_propuesto = 2;
    std::string seleccion_propuesto = "torneo";

    // ASrank; alpha, beta y rho se cambian por los mejores niveles del ajuste
    double alpha = 1;
    double beta = 5;
    double gamma = 5;
    double rho = 0.1;
    int sigma = 6;
    int hormigas = 25;
    int candidatos = 25;   // tamaño de la lista de vecinos más cercanos

    // Presupuesto y ejecución
    double tiempo = 30;            // segundos por corrida de los algoritmos estocásticos
    double tiempo_modelo = 300;    // límite del solver MILP
    int modelo_hasta_n = 100;      // arriba de este número de clientes el modelo no se corre
    long max_iter = 0;             // 0 = sin límite de iteraciones, solo de tiempo
    int corridas = 3;
    std::uint64_t semilla = 1;
    int hilos = 5;                 // núcleos de rendimiento de la Mac
    std::vector<std::string> instancias;   // vacío = las del experimento
};
