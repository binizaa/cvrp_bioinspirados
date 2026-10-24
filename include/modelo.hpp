// Modelo MILP de dos índices del Entregable 1, resuelto con HiGHS.
#pragma once

#include "algoritmo.hpp"

class ModeloExacto : public Algoritmo {
public:
    explicit ModeloExacto(int hasta_n) : hasta_n_(hasta_n) {}
    std::string nombre() const override { return "modelo"; }
    std::string config() const override { return "highs"; }
    bool estocastico() const override { return false; }
    Resultado resolver(const Instancia& in, std::uint64_t semilla, double tiempo) const override;

private:
    int hasta_n_;
};
