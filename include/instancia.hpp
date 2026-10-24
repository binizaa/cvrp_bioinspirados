// Instancias CVRPLIB, utilidades sobre rutas y mejores soluciones conocidas.
#pragma once

#include <chrono>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

constexpr double EPS = 1e-9;

using Ruta = std::vector<int>;
using Rutas = std::vector<Ruta>;

// Depósito en el índice 0 y clientes 1..n; ids[v] es el número de nodo del
// archivo (el depósito es el nodo 1). Si el archivo trae DISTANCE, una ruta es
// factible cuando su longitud + s * (número de clientes) <= L.
struct Instancia {
    std::string nombre;
    int n = 0;
    int Q = 0;
    std::vector<int> q;
    std::vector<double> c;   // matriz (n+1) x (n+1) en un solo arreglo
    std::vector<std::pair<double, double>> xy;
    std::vector<int> ids;
    double L = std::numeric_limits<double>::infinity();
    double s = 0;
    double bks = std::nan("");
    bool redondeo = true;

    double d(int i, int j) const { return c[static_cast<size_t>(i) * (n + 1) + j]; }
    bool con_limite() const { return std::isfinite(L); }
    int k_min() const;
};

// redondear: -1 = automático (CMT con distancias reales, como sus BKS; las
// demás redondeadas al entero más cercano, como en CVRPLIB), 0 = no, 1 = sí.
Instancia leer_cvrplib(const std::string& ruta, int redondear = -1);

// Acepta una ruta o un nombre ("CMT1", "X-n459-k26", "CVRP_10").
std::string buscar_instancia(const std::string& nombre);

// Las 24 instancias del curso: CMT1..CMT14 y luego las X por tamaño.
std::vector<std::string> todas_las_instancias();

double longitud(const Ruta& r, const Instancia& in);
double costo(const Rutas& rs, const Instancia& in);
bool verificar(const Rutas& rs, const Instancia& in, std::string* error = nullptr);

// 2-opt intra-ruta hasta un óptimo local. Solo acorta la ruta, así que
// conserva la factibilidad respecto a L.
void dos_opt(Ruta& r, const Instancia& in);

double gap(double Z, double ref);
std::string texto_ruta(const Ruta& r, const Instancia& in);

// Formato .sol de CVRPLIB (clientes numerados 1..n, depósito omitido).
void escribir_sol(const std::string& ruta, const Rutas& rs, double Z);

class Cronometro {
    std::chrono::steady_clock::time_point t0_ = std::chrono::steady_clock::now();

public:
    double segundos() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0_).count();
    }
};
