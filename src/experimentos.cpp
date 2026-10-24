#include "experimentos.hpp"

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

#include "algoritmo.hpp"

namespace fs = std::filesystem;

namespace {

const char* ENCABEZADO =
    "experimento,instancia,n,metodo,config,semilla,Z,bks,gap_bks,cota,estado,rutas,"
    "segundos,t_mejora,it_mejora,iteraciones,factible";

std::vector<std::string> rutas_de(const std::vector<std::string>& elegidas,
                                  const std::vector<std::string>& por_defecto) {
    std::vector<std::string> r;
    for (const auto& s : elegidas.empty() ? por_defecto : elegidas) r.push_back(buscar_instancia(s));
    return r;
}

std::string campo(double x) {
    if (std::isnan(x)) return "";
    char b[32];
    std::snprintf(b, sizeof b, "%.4f", x);
    return b;
}

std::string clave(const std::string& exp, const std::string& inst, const std::string& metodo,
                  const std::string& config, std::uint64_t semilla) {
    return exp + "|" + inst + "|" + metodo + "|" + config + "|" + std::to_string(semilla);
}

std::string seguro(std::string s) {
    std::string r;
    for (char ch : s) {
        if (ch == ';') r += '_';
        else if (ch != '=') r += ch;
    }
    return r;
}

std::vector<std::string> separar(const std::string& linea) {
    std::vector<std::string> v;
    std::stringstream ss(linea);
    std::string x;
    while (std::getline(ss, x, ',')) v.push_back(x);
    return v;
}

}  // namespace

std::vector<Tarea> tareas_ajuste_ag(const Opciones& op) {
    std::vector<Tarea> t;
    for (const auto& ruta : rutas_de(op.instancias, INSTANCIAS_AJUSTE))
        for (int cruce : NIVELES_CRUCE)
            for (double pm : NIVELES_PM)
                for (const auto& sel : NIVELES_SELECCION)
                    for (int k = 0; k < op.corridas; ++k) {
                        Opciones o = op;
                        o.cruce = cruce;
                        o.pm = pm;
                        o.seleccion = sel;
                        t.push_back({"ajuste_ag", ruta, "ag", o, op.semilla + k, op.tiempo});
                    }
    return t;
}

std::vector<Tarea> tareas_ajuste_aco(const Opciones& op) {
    std::vector<Tarea> t;
    for (const auto& ruta : rutas_de(op.instancias, INSTANCIAS_AJUSTE))
        for (double alpha : NIVELES_ALPHA)
            for (double beta : NIVELES_BETA)
                for (double rho : NIVELES_RHO)
                    for (int k = 0; k < op.corridas; ++k) {
                        Opciones o = op;
                        o.alpha = alpha;
                        o.beta = beta;
                        o.rho = rho;
                        t.push_back({"ajuste_aco", ruta, "aco", o, op.semilla + k, op.tiempo});
                    }
    return t;
}

std::vector<Tarea> tareas_comparacion(const Opciones& op) {
    std::vector<Tarea> t;
    auto rutas = op.instancias.empty() ? todas_las_instancias() : rutas_de(op.instancias, {});
    for (const auto& ruta : rutas) {
        t.push_back({"comparacion", ruta, "modelo", op, op.semilla, op.tiempo_modelo});
        for (const auto& metodo : {"propuesto", "ag", "aco"})
            for (int k = 0; k < op.corridas; ++k)
                t.push_back({"comparacion", ruta, metodo, op, op.semilla + k, op.tiempo});
    }
    return t;
}

void correr_tareas(const std::vector<Tarea>& tareas, const std::string& csv, int hilos) {
    std::map<std::string, std::shared_ptr<const Instancia>> instancias;
    for (const auto& t : tareas)
        if (!instancias.count(t.ruta))
            instancias[t.ruta] = std::make_shared<const Instancia>(leer_cvrplib(t.ruta));

    // Tareas que ya están en el CSV
    std::set<std::string> hechas;
    fs::create_directories(fs::path(csv).parent_path());
    if (fs::exists(csv)) {
        std::ifstream fh(csv);
        std::string linea;
        std::getline(fh, linea);
        while (std::getline(fh, linea)) {
            auto c = separar(linea);
            if (c.size() > 5) hechas.insert(clave(c[0], c[1], c[3], c[4], std::stoull(c[5])));
        }
    } else {
        std::ofstream(csv) << ENCABEZADO << '\n';
    }

    std::vector<const Tarea*> estocasticas, modelo;
    for (const auto& t : tareas) {
        auto alg = crear_algoritmo(t.metodo, t.op);
        const auto& in = *instancias[t.ruta];
        if (hechas.count(clave(t.experimento, in.nombre, t.metodo, alg->config(), t.semilla))) continue;
        (alg->estocastico() ? estocasticas : modelo).push_back(&t);
    }
    const size_t total = estocasticas.size() + modelo.size();
    std::cout << tareas.size() << " tareas, " << tareas.size() - total << " ya hechas, " << total
              << " por correr (" << estocasticas.size() << " estocásticas en " << hilos
              << " hilos, " << modelo.size() << " del modelo)\n";

    std::mutex candado;
    std::atomic<size_t> terminadas{0};
    auto correr = [&](const Tarea& t) {
        const auto& in = *instancias[t.ruta];
        auto alg = crear_algoritmo(t.metodo, t.op);
        Resultado r;
        try {
            r = alg->resolver(in, t.semilla, t.tiempo);
        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> g(candado);
            std::cerr << "Error en " << in.nombre << " " << t.metodo << ": " << e.what() << '\n';
            return;
        }
        std::string base = in.nombre + "__" + t.metodo + "__" + seguro(r.config) + "__s" +
                           std::to_string(t.semilla);
        std::lock_guard<std::mutex> g(candado);
        std::ofstream(csv, std::ios::app)
            << t.experimento << ',' << in.nombre << ',' << in.n << ',' << t.metodo << ','
            << r.config << ',' << t.semilla << ',' << campo(r.Z) << ',' << campo(in.bks) << ','
            << campo(gap(r.Z, in.bks)) << ',' << campo(r.cota) << ',' << r.estado << ','
            << r.rutas.size() << ',' << campo(r.segundos) << ',' << campo(r.t_mejora) << ','
            << r.it_mejora << ',' << r.iteraciones << ',' << (r.factible ? 1 : 0) << '\n';

        fs::path dh = fs::path(DIR_RESULTADOS) / "historial" / t.experimento;
        fs::create_directories(dh);
        std::ofstream hf(dh / (base + ".csv"));
        hf << "it,t,f\n";
        for (const auto& p : r.historial) hf << p.it << ',' << p.t << ',' << p.f << '\n';
        if (!r.rutas.empty()) {
            fs::path ds = fs::path(DIR_RESULTADOS) / "soluciones" / t.experimento;
            fs::create_directories(ds);
            escribir_sol((ds / (base + ".sol")).string(), r.rutas, r.Z);
        }

        char linea[256];
        double g_ = gap(r.Z, in.bks);
        std::snprintf(linea, sizeof linea, "[%4zu/%zu] %-11s %-9s %-26s s%-3llu Z=%-10s GAP=%7s%%  t*=%6.1fs  %s",
                      ++terminadas, total, in.nombre.c_str(), t.metodo.c_str(), r.config.c_str(),
                      static_cast<unsigned long long>(t.semilla),
                      std::isnan(r.Z) ? "-" : campo(r.Z).c_str(),
                      std::isnan(g_) ? "-" : campo(g_).c_str(), r.t_mejora, r.estado.c_str());
        std::cout << linea << (r.factible || std::isnan(r.Z) ? "" : "  NO FACTIBLE") << std::endl;
    };

    std::atomic<size_t> siguiente{0};
    std::vector<std::thread> pool;
    for (int h = 0; h < std::max(1, hilos); ++h)
        pool.emplace_back([&]() {
            for (size_t k; (k = siguiente++) < estocasticas.size();) correr(*estocasticas[k]);
        });
    for (auto& th : pool) th.join();
    for (const Tarea* t : modelo) correr(*t);
}
