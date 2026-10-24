// Algoritmo genético clásico y algoritmo propuesto.
//
// Los dos usan permutación de clientes, decodificación secuencial por
// capacidad y longitud de ruta, cruce PMX de 1 o 2 puntos, mutación por
// intercambio y reemplazo de estado estable. El propuesto agrega lo del
// Entregable 2: 2-opt por ruta al evaluar (sin reescribir el cromosoma) y
// descarte de hijos cuya forma canónica ya está en la población.
#pragma once

#include "algoritmo.hpp"

class Genetico : public Algoritmo {
public:
    Genetico(bool propuesto, int Np, double pc, double pm, int cruce, std::string seleccion,
             long max_iter)
        : propuesto_(propuesto), Np_(Np), pc_(pc), pm_(pm), cruce_(cruce),
          seleccion_(std::move(seleccion)), max_iter_(max_iter) {}

    std::string nombre() const override { return propuesto_ ? "propuesto" : "ag"; }
    std::string config() const override {
        return "cruce=" + std::to_string(cruce_) + ";pm=" + num(pm_) + ";sel=" + seleccion_;
    }
    Resultado resolver(const Instancia& in, std::uint64_t semilla, double tiempo) const override;

private:
    bool propuesto_;
    int Np_;
    double pc_, pm_;
    int cruce_;
    std::string seleccion_;
    long max_iter_;
};
