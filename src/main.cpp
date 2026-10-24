// Punto de entrada del proyecto. Los comandos y opciones están en README.md.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "algoritmo.hpp"
#include "experimentos.hpp"
#include "tablas.hpp"

namespace {

const char* AYUDA =
    "Uso: ./build/cvrp <resolver|validar|ajuste-ag|ajuste-aco|comparar|tablas> [opciones]\n"
    "Comandos y opciones en README.md\n";

void imprimir_instancia(const Instancia& in) {
    std::cout << in.nombre << "  n = " << in.n << "  Q = " << in.Q << "  K_min = " << in.k_min()
              << "  distancias " << (in.redondeo ? "redondeadas" : "reales");
    if (in.con_limite()) std::cout << "  L = " << in.L << ", servicio = " << in.s;
    if (!std::isnan(in.bks)) std::cout << "  BKS = " << in.bks;
    std::cout << '\n';
}

void imprimir_corrida(const Resultado& r, const Instancia& in) {
    char b[256];
    double g = gap(r.Z, in.bks);
    std::snprintf(b, sizeof b, "  semilla %-3llu Z = %-10.2f GAP %7.2f %%  rutas %-3zu última mejora: it %ld (%.1f s) de %ld  %.1f s",
                  static_cast<unsigned long long>(r.semilla), r.Z, g, r.rutas.size(), r.it_mejora,
                  r.t_mejora, r.iteraciones, r.segundos);
    std::cout << b << (r.factible ? "" : "  NO FACTIBLE") << '\n';
}

Resultado resolver(const std::string& nombre, const std::string& metodo, const Opciones& op,
                   bool mostrar_rutas = true) {
    Instancia in = leer_cvrplib(buscar_instancia(nombre));
    imprimir_instancia(in);
    auto alg = crear_algoritmo(metodo, op);
    std::cout << metodo << " (" << alg->config() << ")\n";
    Resultado mejor;
    double suma = 0;
    int k_max = alg->estocastico() ? op.corridas : 1;
    for (int k = 0; k < k_max; ++k) {
        Resultado r = alg->resolver(in, op.semilla + k, alg->estocastico() ? op.tiempo : op.tiempo_modelo);
        if (!alg->estocastico()) {
            std::cout << "  estado: " << r.estado;
            if (!std::isnan(r.Z))
                std::printf("  Z = %.2f  cota = %.2f  GAP contra BKS %.2f %%  %.1f s%s",
                            r.Z, r.cota, gap(r.Z, in.bks), r.segundos, r.factible ? "" : "  NO FACTIBLE");
            std::cout << std::endl;
        } else {
            imprimir_corrida(r, in);
        }
        suma += r.Z;
        if (k == 0 || r.Z < mejor.Z) mejor = r;
    }
    if (k_max > 1)
        std::printf("Z promedio %.2f  mejor %.2f  GAP promedio %.2f %%\n", suma / k_max, mejor.Z,
                    gap(suma / k_max, in.bks));
    if (mostrar_rutas && !mejor.rutas.empty()) {
        std::cout << "Mejor solución (" << mejor.rutas.size() << " rutas):\n";
        for (const auto& r : mejor.rutas) std::cout << "  " << texto_ruta(r, in) << '\n';
    }
    return mejor;
}

int validar(Opciones op) {
    op.tiempo = 2;
    op.tiempo_modelo = 60;
    op.corridas = 3;
    bool ok = true;
    for (const auto& m : METODOS) {
        std::cout << "\n== " << m << '\n';
        Resultado r = resolver("CVRP_10", m, op, m == "modelo");
        bool esperado = std::fabs(r.Z - 317) < 1e-6 && r.factible;
        if (m == "modelo" || m == "propuesto") {
            std::cout << (esperado ? "OK: 317" : "FALLA: se esperaba 317") << '\n';
            ok = ok && esperado;
        }
    }
    std::cout << '\n' << (ok ? "Validación correcta" : "Validación con fallas") << '\n';
    return ok ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << AYUDA;
        return 1;
    }
    const std::string cmd = argv[1];
    Opciones op;
    std::string metodo = "propuesto";
    std::vector<std::string> posicionales;
    try {
        for (int a = 2; a < argc; ++a) {
            std::string s = argv[a];
            if (s.rfind("--", 0) != 0) {
                posicionales.push_back(s);
                continue;
            }
            std::string k = s.substr(2);
            if (k == "instancias") {
                while (a + 1 < argc && std::string(argv[a + 1]).rfind("--", 0) != 0)
                    op.instancias.push_back(argv[++a]);
                continue;
            }
            if (a + 1 >= argc) throw std::runtime_error("Falta el valor de --" + k);
            std::string v = argv[++a];
            if (k == "metodo") metodo = v;
            else if (k == "tiempo") op.tiempo = std::stod(v);
            else if (k == "tiempo-modelo") op.tiempo_modelo = std::stod(v);
            else if (k == "modelo-hasta-n") op.modelo_hasta_n = std::stoi(v);
            else if (k == "corridas") op.corridas = std::stoi(v);
            else if (k == "semilla") op.semilla = std::stoull(v);
            else if (k == "hilos") op.hilos = std::stoi(v);
            else if (k == "max-iter") op.max_iter = std::stol(v);
            else if (k == "pm") op.pm = std::stod(v);
            else if (k == "pc") op.pc = std::stod(v);
            else if (k == "Np") op.Np = std::stoi(v);
            else if (k == "cruce") op.cruce = std::stoi(v);
            else if (k == "seleccion") op.seleccion = v;
            else if (k == "alpha") op.alpha = std::stod(v);
            else if (k == "beta") op.beta = std::stod(v);
            else if (k == "gamma") op.gamma = std::stod(v);
            else if (k == "rho") op.rho = std::stod(v);
            else if (k == "sigma") op.sigma = std::stoi(v);
            else if (k == "hormigas") op.hormigas = std::stoi(v);
            else if (k == "candidatos") op.candidatos = std::stoi(v);
            else throw std::runtime_error("Opción desconocida: --" + k);
        }

        if (cmd == "resolver") {
            if (posicionales.empty()) throw std::runtime_error("Indica la instancia, por ejemplo: resolver CMT1");
            resolver(posicionales[0], metodo, op);
        } else if (cmd == "validar") {
            return validar(op);
        } else if (cmd == "ajuste-ag") {
            correr_tareas(tareas_ajuste_ag(op), DIR_RESULTADOS + "/ajuste_ag.csv", op.hilos);
        } else if (cmd == "ajuste-aco") {
            correr_tareas(tareas_ajuste_aco(op), DIR_RESULTADOS + "/ajuste_aco.csv", op.hilos);
        } else if (cmd == "comparar") {
            correr_tareas(tareas_comparacion(op), DIR_RESULTADOS + "/comparacion.csv", op.hilos);
        } else if (cmd == "tablas") {
            generar_tablas();
        } else {
            std::cout << AYUDA;
            return cmd == "ayuda" ? 0 : 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
