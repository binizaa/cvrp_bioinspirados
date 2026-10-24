#include "algoritmo.hpp"

#include <cstdio>
#include <stdexcept>

#include "geneticos.hpp"
#include "hormigas.hpp"
#include "modelo.hpp"

std::string num(double x) {
    char b[32];
    std::snprintf(b, sizeof b, "%g", x);
    return b;
}

std::unique_ptr<Algoritmo> crear_algoritmo(const std::string& metodo, const Opciones& op) {
    if (metodo == "modelo") return std::make_unique<ModeloExacto>(op.modelo_hasta_n);
    if (metodo == "propuesto")
        return std::make_unique<Genetico>(true, op.Np, op.pc, op.pm_propuesto, op.cruce_propuesto,
                                          op.seleccion_propuesto, op.max_iter);
    if (metodo == "ag")
        return std::make_unique<Genetico>(false, op.Np, op.pc, op.pm, op.cruce, op.seleccion,
                                          op.max_iter);
    if (metodo == "aco")
        return std::make_unique<ASrank>(op.alpha, op.beta, op.gamma, op.rho, op.sigma, op.hormigas,
                                        op.candidatos, op.max_iter);
    throw std::runtime_error("Método desconocido: " + metodo + " (usa modelo, propuesto, ag o aco)");
}
