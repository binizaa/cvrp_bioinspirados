// ASrank (Bullnheimer, Hartl y Strauss, 1999) con flota libre.
//
// Cada hormiga elige el siguiente cliente con probabilidad proporcional a
// tau_ij^alpha * eta_ij^beta * mu_ij^gamma, donde eta = 1/d y mu es el ahorro
// de Clarke y Wright. Los candidatos son los vecinos más cercanos factibles; si
// ninguno lo es, se revisan todos los clientes y, si tampoco hay, la hormiga
// regresa al depósito y abre una ruta nueva. Cada ruta se mejora con 2-opt y
// la feromona se actualiza por rangos: depositan las sigma - 1 mejores hormigas
// de la iteración y la mejor solución global.
#pragma once

#include "algoritmo.hpp"

class ASrank : public Algoritmo {
public:
    ASrank(double alpha, double beta, double gamma, double rho, int sigma, int hormigas,
           int candidatos, long max_iter)
        : alpha_(alpha), beta_(beta), gamma_(gamma), rho_(rho), sigma_(sigma),
          hormigas_(hormigas), candidatos_(candidatos), max_iter_(max_iter) {}

    std::string nombre() const override { return "aco"; }
    std::string config() const override {
        return "alpha=" + num(alpha_) + ";beta=" + num(beta_) + ";rho=" + num(rho_);
    }
    Resultado resolver(const Instancia& in, std::uint64_t semilla, double tiempo) const override;

private:
    double alpha_, beta_, gamma_, rho_;
    int sigma_, hormigas_, candidatos_;
    long max_iter_;
};
