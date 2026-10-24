// Interfaz común de los cuatro métodos: modelo, propuesto, AG y ACO.
#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "config.hpp"
#include "instancia.hpp"

struct PuntoHistorial {
    long it;
    double t;
    double f;
};

struct Resultado {
    std::string metodo;
    std::string config;
    std::string estado = "ok";   // modelo: optimo, factible, sin solucion, no escala
    std::uint64_t semilla = 0;
    double Z = std::nan("");
    double cota = std::nan("");  // cota inferior, solo el modelo
    double segundos = 0;
    double t_mejora = 0;         // segundo de la última mejora
    long it_mejora = 0;          // iteración de la última mejora
    long iteraciones = 0;        // modelo: nodos de ramificación
    bool factible = false;
    Rutas rutas;
    std::vector<PuntoHistorial> historial;
};

class Algoritmo {
public:
    virtual ~Algoritmo() = default;
    virtual std::string nombre() const = 0;
    virtual std::string config() const = 0;
    virtual bool estocastico() const { return true; }
    virtual Resultado resolver(const Instancia& in, std::uint64_t semilla, double tiempo) const = 0;
};

inline const std::vector<std::string> METODOS = {"modelo", "propuesto", "ag", "aco"};

std::unique_ptr<Algoritmo> crear_algoritmo(const std::string& metodo, const Opciones& op);

std::string num(double x);
