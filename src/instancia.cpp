#include "instancia.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

#include "config.hpp"

namespace fs = std::filesystem;

namespace {

const std::map<std::string, double> BKS = {
    {"X-n280-k17", 33503},  {"X-n303-k21", 21736}, {"X-n308-k13", 25859},
    {"X-n327-k20", 27532},  {"X-n336-k84", 139111}, {"X-n401-k29", 66154},
    {"X-n411-k19", 19712},  {"X-n429-k61", 65449}, {"X-n449-k29", 55233},
    {"X-n459-k26", 24139},  {"CVRP_10", 317},
};

std::string recortar(const std::string& s) {
    const char* ws = " \t\r\n";
    auto a = s.find_first_not_of(ws);
    if (a == std::string::npos) return "";
    auto b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

std::string mayusculas(std::string s) {
    for (auto& ch : s) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return s;
}

bool empieza(const std::string& s, const std::string& p) { return s.rfind(p, 0) == 0; }

}  // namespace

int Instancia::k_min() const {
    long total = 0;
    for (int v : q) total += v;
    return static_cast<int>((total + Q - 1) / Q);
}

Instancia leer_cvrplib(const std::string& ruta, int redondear) {
    std::ifstream fh(ruta);
    if (!fh) throw std::runtime_error("No se pudo abrir " + ruta);

    std::map<std::string, std::string> meta;
    std::map<int, std::pair<double, double>> coords;
    std::map<int, int> dem;
    std::vector<int> depositos;
    std::string seccion, linea;
    while (std::getline(fh, linea)) {
        std::string l = recortar(linea);
        if (l.empty()) continue;
        std::string u = mayusculas(l);
        if (empieza(u, "NODE_COORD_SECTION")) { seccion = "coord"; continue; }
        if (empieza(u, "DEMAND_SECTION")) { seccion = "dem"; continue; }
        if (empieza(u, "DEPOT_SECTION")) { seccion = "dep"; continue; }
        if (empieza(u, "EOF")) break;
        auto dp = l.find(':');
        if (dp != std::string::npos) {
            meta[mayusculas(recortar(l.substr(0, dp)))] = recortar(l.substr(dp + 1));
            seccion.clear();
            continue;
        }
        std::istringstream is(l);
        if (seccion == "coord") {
            int i; double a, b;
            is >> i >> a >> b;
            coords[i] = {a, b};
        } else if (seccion == "dem") {
            int i, v;
            is >> i >> v;
            dem[i] = v;
        } else if (seccion == "dep") {
            int i;
            is >> i;
            if (i != -1) depositos.push_back(i);
        }
    }

    auto tipo = meta.count("EDGE_WEIGHT_TYPE") ? mayusculas(meta["EDGE_WEIGHT_TYPE"]) : "EUC_2D";
    if (tipo != "EUC_2D") throw std::runtime_error("Solo se soporta EDGE_WEIGHT_TYPE EUC_2D");

    Instancia in;
    in.nombre = meta.count("NAME") ? meta["NAME"] : fs::path(ruta).stem().string();
    bool es_cmt = empieza(mayusculas(in.nombre), "CMT");
    in.redondeo = redondear < 0 ? !es_cmt : redondear == 1;
    in.Q = std::stoi(meta.at("CAPACITY"));

    int deposito = depositos.empty() ? coords.begin()->first : depositos.front();
    in.ids.push_back(deposito);
    for (auto& [k, _] : coords)
        if (k != deposito) in.ids.push_back(k);
    in.n = static_cast<int>(in.ids.size()) - 1;
    for (int k : in.ids) {
        in.xy.push_back(coords.at(k));
        in.q.push_back(k == deposito ? 0 : dem.at(k));
    }

    const int m = in.n + 1;
    in.c.assign(static_cast<size_t>(m) * m, 0.0);
    for (int i = 0; i < m; ++i)
        for (int j = i + 1; j < m; ++j) {
            double dij = std::hypot(in.xy[i].first - in.xy[j].first, in.xy[i].second - in.xy[j].second);
            if (in.redondeo) dij = std::floor(dij + 0.5);
            in.c[static_cast<size_t>(i) * m + j] = in.c[static_cast<size_t>(j) * m + i] = dij;
        }

    if (es_cmt && meta.count("COMMENT")) in.bks = std::stod(meta["COMMENT"]);
    else if (BKS.count(in.nombre)) in.bks = BKS.at(in.nombre);
    if (meta.count("DISTANCE")) in.L = std::stod(meta["DISTANCE"]);
    if (meta.count("SERVICE_TIME")) in.s = std::stod(meta["SERVICE_TIME"]);

    for (int i = 1; i <= in.n; ++i) {
        if (in.q[i] > in.Q)
            throw std::runtime_error("El nodo " + std::to_string(in.ids[i]) + " excede la capacidad");
        if (2 * in.d(0, i) + in.s > in.L + EPS)
            throw std::runtime_error("El nodo " + std::to_string(in.ids[i]) + " no cabe en una ruta");
    }
    return in;
}

std::string buscar_instancia(const std::string& nombre) {
    if (fs::is_regular_file(nombre)) return nombre;
    for (const auto& dir : {DIR_INSTANCIAS, DIR_VALIDACION})
        for (const auto& ext : {std::string(".vrp"), std::string("")}) {
            fs::path p = fs::path(dir) / (nombre + ext);
            if (fs::is_regular_file(p)) return p.string();
        }
    throw std::runtime_error("No encuentro la instancia " + nombre);
}

std::vector<std::string> todas_las_instancias() {
    std::vector<std::pair<std::pair<int, int>, std::string>> v;
    for (const auto& e : fs::directory_iterator(DIR_INSTANCIAS)) {
        if (e.path().extension() != ".vrp") continue;
        std::string s = e.path().stem().string();
        // CMT primero por número; luego las X por número de nodos
        if (empieza(s, "CMT")) v.push_back({{0, std::stoi(s.substr(3))}, e.path().string()});
        else if (empieza(s, "X-n")) v.push_back({{1, std::stoi(s.substr(3))}, e.path().string()});
        else v.push_back({{2, 0}, e.path().string()});
    }
    std::sort(v.begin(), v.end());
    std::vector<std::string> r;
    for (auto& [_, p] : v) r.push_back(p);
    return r;
}

double longitud(const Ruta& r, const Instancia& in) {
    double total = 0;
    int prev = 0;
    for (int v : r) {
        total += in.d(prev, v);
        prev = v;
    }
    return total + in.d(prev, 0);
}

double costo(const Rutas& rs, const Instancia& in) {
    double total = 0;
    for (const auto& r : rs) total += longitud(r, in);
    return total;
}

bool verificar(const Rutas& rs, const Instancia& in, std::string* error) {
    auto falla = [&](const std::string& m) {
        if (error) *error = m;
        return false;
    };
    std::vector<int> visto(in.n + 1, 0);
    for (const auto& r : rs) {
        long carga = 0;
        for (int v : r) {
            if (v < 1 || v > in.n) return falla("cliente fuera de rango");
            ++visto[v];
            carga += in.q[v];
        }
        if (carga > in.Q) return falla("ruta excede la capacidad");
        if (longitud(r, in) + in.s * r.size() > in.L + 1e-6) return falla("ruta excede DISTANCE");
    }
    for (int v = 1; v <= in.n; ++v)
        if (visto[v] != 1) return falla("clientes faltantes o repetidos");
    return true;
}

void dos_opt(Ruta& ruta, const Instancia& in) {
    if (ruta.size() < 3) return;
    std::vector<int> r;
    r.reserve(ruta.size() + 2);
    r.push_back(0);
    r.insert(r.end(), ruta.begin(), ruta.end());
    r.push_back(0);
    const int L = static_cast<int>(r.size());
    bool mejora = true;
    while (mejora) {
        mejora = false;
        for (int i = 0; i < L - 3; ++i)
            for (int j = i + 2; j < L - 1; ++j) {
                int a = r[i], b = r[i + 1], x = r[j], y = r[j + 1];
                if (in.d(a, x) + in.d(b, y) < in.d(a, b) + in.d(x, y) - EPS) {
                    std::reverse(r.begin() + i + 1, r.begin() + j + 1);
                    mejora = true;
                }
            }
    }
    std::copy(r.begin() + 1, r.end() - 1, ruta.begin());
}

double gap(double Z, double ref) {
    if (std::isnan(Z) || std::isnan(ref) || ref == 0) return std::nan("");
    return 100.0 * (Z - ref) / ref;
}

std::string texto_ruta(const Ruta& r, const Instancia& in) {
    std::string s = std::to_string(in.ids[0]);
    for (int v : r) s += " -> " + std::to_string(in.ids[v]);
    return s + " -> " + std::to_string(in.ids[0]);
}

void escribir_sol(const std::string& ruta, const Rutas& rs, double Z) {
    std::ofstream fh(ruta);
    for (size_t k = 0; k < rs.size(); ++k) {
        fh << "Route #" << k + 1 << ":";
        for (int v : rs[k]) fh << ' ' << v;
        fh << '\n';
    }
    fh << "Cost " << Z << '\n';
}
