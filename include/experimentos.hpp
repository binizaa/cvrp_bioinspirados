// Diseños experimentales y ejecución en paralelo con resultados en CSV.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "config.hpp"

struct Tarea {
    std::string experimento;
    std::string ruta;     // archivo .vrp
    std::string metodo;
    Opciones op;
    std::uint64_t semilla;
    double tiempo;
};

// 12 configuraciones del AG clásico en INSTANCIAS_AJUSTE (o --instancias)
std::vector<Tarea> tareas_ajuste_ag(const Opciones& op);
// 8 configuraciones del ACO en INSTANCIAS_AJUSTE (o --instancias)
std::vector<Tarea> tareas_ajuste_aco(const Opciones& op);
// Los cuatro métodos en las 24 instancias (o --instancias)
std::vector<Tarea> tareas_comparacion(const Opciones& op);

// Corre los algoritmos estocásticos en paralelo y después el modelo, uno a la
// vez (HiGHS ya usa varios hilos). Cada resultado se agrega al CSV en cuanto
// termina y las tareas que ya están en el CSV se saltan, así que un
// experimento interrumpido se reanuda volviendo a lanzarlo.
void correr_tareas(const std::vector<Tarea>& tareas, const std::string& csv, int hilos);
