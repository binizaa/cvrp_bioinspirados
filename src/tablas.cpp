#include "tablas.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "config.hpp"

namespace fs = std::filesystem;

namespace {

using Fila = std::map<std::string, std::string>;

std::vector<Fila> leer_csv(const fs::path& ruta) {
    std::vector<Fila> filas;
    std::ifstream fh(ruta);
    std::string linea;
    if (!std::getline(fh, linea)) return filas;
    std::vector<std::string> cols;
    std::stringstream sc(linea);
    for (std::string x; std::getline(sc, x, ',');) cols.push_back(x);
    while (std::getline(fh, linea)) {
        Fila f;
        std::stringstream ss(linea);
        std::string x;
        for (size_t k = 0; k < cols.size(); ++k) {
            if (!std::getline(ss, x, ',')) x.clear();
            f[cols[k]] = x;
        }
        filas.push_back(f);
    }
    return filas;
}

double valor(const Fila& f, const std::string& k) {
    auto it = f.find(k);
    return it == f.end() || it->second.empty() ? std::nan("") : std::stod(it->second);
}

struct Media {
    double s = 0;
    int n = 0;
    void add(double x) {
        if (!std::isnan(x)) { s += x; ++n; }
    }
    double v() const { return n ? s / n : std::nan(""); }
};

std::string fmt(double x, const char* f = "%.2f") {
    if (std::isnan(x)) return "---";
    char b[32];
    std::snprintf(b, sizeof b, f, x);
    return b;
}

std::string esc(std::string s) {
    std::string r;
    for (char ch : s) {
        if (ch == '_' || ch == '%' || ch == '&') r += '\\';
        r += ch;
    }
    return r;
}

// "cruce=2;pm=0.1;sel=torneo" -> {cruce: 2, pm: 0.1, sel: torneo}
std::map<std::string, std::string> parametros(const std::string& config) {
    std::map<std::string, std::string> m;
    std::stringstream ss(config);
    for (std::string par; std::getline(ss, par, ';');) {
        auto e = par.find('=');
        if (e != std::string::npos) m[par.substr(0, e)] = par.substr(e + 1);
    }
    return m;
}

template <class T>
void agregar_unico(std::vector<T>& v, const T& x) {
    for (const auto& y : v)
        if (y == x) return;
    v.push_back(x);
}

struct Factor {
    std::string clave;    // nombre en config
    std::string titulo;   // encabezado LaTeX
};

// Tabla de configuraciones (GAP por instancia y promedios) y tabla de
// efectos principales (promedio por nivel de cada factor).
void tablas_ajuste(const std::string& exp, const std::vector<Factor>& factores,
                   const std::string& alg) {
    auto filas = leer_csv(fs::path(DIR_RESULTADOS) / (exp + ".csv"));
    if (filas.empty()) return;

    std::vector<std::string> configs, insts;
    std::map<std::pair<std::string, std::string>, Media> gap_ci;
    std::map<std::string, Media> gap_c, t_c, it_c;
    for (const auto& f : filas) {
        const auto &c = f.at("config"), &i = f.at("instancia");
        agregar_unico(configs, c);
        agregar_unico(insts, i);
        gap_ci[{c, i}].add(valor(f, "gap_bks"));
        gap_c[c].add(valor(f, "gap_bks"));
        t_c[c].add(valor(f, "t_mejora"));
        it_c[c].add(valor(f, "it_mejora"));
    }
    std::string mejor = configs.front();
    for (const auto& c : configs)
        if (gap_c[c].v() < gap_c[mejor].v()) mejor = c;

    std::ofstream t(fs::path(DIR_TABLAS) / (exp + ".tex"));
    t << "\\begin{table}[htbp]\n\\centering\n\\small\n\\begin{tabular}{"
      << std::string(factores.size(), 'c') << std::string(insts.size() + 3, 'r') << "}\n\\toprule\n";
    for (const auto& fa : factores) t << fa.titulo << " & ";
    for (const auto& i : insts) t << esc(i) << " & ";
    t << "GAP prom. & $\\bar t^*$ (s) & $\\bar\\imath^*$ \\\\\n\\midrule\n";
    for (const auto& c : configs) {
        auto p = parametros(c);
        for (const auto& fa : factores) t << p[fa.clave] << " & ";
        for (const auto& i : insts) t << fmt(gap_ci[{c, i}].v()) << " & ";
        std::string g = fmt(gap_c[c].v());
        t << (c == mejor ? "\\textbf{" + g + "}" : g) << " & " << fmt(t_c[c].v(), "%.1f") << " & "
          << fmt(it_c[c].v(), "%.0f") << " \\\\\n";
    }
    t << "\\bottomrule\n\\end{tabular}\n\\caption{Ajuste de parámetros del " << alg
      << ": GAP promedio (\\%) contra la BKS por instancia, segundo $t^*$ e iteración "
         "$i^*$ de la última mejora, promediados sobre las corridas.}\n\\label{tab:"
      << exp << "}\n\\end{table}\n";

    std::ofstream e(fs::path(DIR_TABLAS) / ("efectos_" + exp.substr(exp.find('_') + 1) + ".tex"));
    e << "\\begin{table}[htbp]\n\\centering\n\\small\n\\begin{tabular}{llrrr}\n\\toprule\n"
         "Hiperparámetro & Nivel & GAP prom. & $\\bar t^*$ (s) & $\\bar\\imath^*$ \\\\\n\\midrule\n";
    for (size_t k = 0; k < factores.size(); ++k) {
        const auto& fa = factores[k];
        std::vector<std::string> niveles;
        std::map<std::string, Media> g, tt, ii;
        for (const auto& f : filas) {
            auto nivel = parametros(f.at("config"))[fa.clave];
            agregar_unico(niveles, nivel);
            g[nivel].add(valor(f, "gap_bks"));
            tt[nivel].add(valor(f, "t_mejora"));
            ii[nivel].add(valor(f, "it_mejora"));
        }
        std::string mejor_nivel = niveles.front();
        for (const auto& nv : niveles)
            if (g[nv].v() < g[mejor_nivel].v()) mejor_nivel = nv;
        for (size_t j = 0; j < niveles.size(); ++j) {
            const auto& nv = niveles[j];
            std::string gs = fmt(g[nv].v());
            e << (j == 0 ? fa.titulo : "") << " & " << nv << " & "
              << (nv == mejor_nivel ? "\\textbf{" + gs + "}" : gs) << " & " << fmt(tt[nv].v(), "%.1f")
              << " & " << fmt(ii[nv].v(), "%.0f") << " \\\\\n";
        }
        if (k + 1 < factores.size()) e << "\\midrule\n";
    }
    e << "\\bottomrule\n\\end{tabular}\n\\caption{Efecto principal de cada hiperparámetro del " << alg
      << ": promedio sobre todas las corridas con ese nivel.}\n\\label{tab:efectos_"
      << exp.substr(exp.find('_') + 1) << "}\n\\end{table}\n";
    std::cout << "Escritas tablas/" << exp << ".tex y tablas/efectos_" << exp.substr(exp.find('_') + 1)
              << ".tex\n";
}

void tablas_comparacion() {
    auto filas = leer_csv(fs::path(DIR_RESULTADOS) / "comparacion.csv");
    if (filas.empty()) return;
    const std::vector<std::string> estoc = {"propuesto", "ag", "aco"};

    std::vector<std::string> insts;
    std::map<std::string, Fila> modelo;
    std::map<std::string, double> n, bks;
    std::map<std::pair<std::string, std::string>, Media> Z, G, T, I;
    for (const auto& f : filas) {
        const auto &i = f.at("instancia"), &m = f.at("metodo");
        agregar_unico(insts, i);
        n[i] = valor(f, "n");
        bks[i] = valor(f, "bks");
        if (m == "modelo") {
            modelo[i] = f;
            continue;
        }
        Z[{i, m}].add(valor(f, "Z"));
        G[{i, m}].add(valor(f, "gap_bks"));
        T[{i, m}].add(valor(f, "t_mejora"));
        I[{i, m}].add(valor(f, "it_mejora"));
    }

    std::ofstream t(fs::path(DIR_TABLAS) / "comparacion_gap.tex");
    t << "\\begin{table}[htbp]\n\\centering\n\\small\n\\begin{tabular}{lrrrrrrr}\n\\toprule\n"
         "Instancia & $n$ & BKS & $Z$ modelo & \\multicolumn{4}{c}{GAP (\\%)} \\\\\n"
         "\\cmidrule(lr){5-8}\n & & & & Modelo & Propuesto & AG & ACO \\\\\n\\midrule\n";
    std::map<std::string, Media> prom;
    for (const auto& i : insts) {
        std::string zm = "---", gm = "---";
        if (modelo.count(i)) {
            const auto& f = modelo[i];
            if (f.at("estado") == "no escala") zm = "n.e.";
            else if (!std::isnan(valor(f, "Z"))) {
                bool fact = f.at("factible") == "1";
                zm = fmt(valor(f, "Z")) + (f.at("estado") == "optimo" ? "$^*$" : "") +
                     (fact ? "" : "$^\\dagger$");
                gm = fmt(valor(f, "gap_bks"));
            }
        }
        t << esc(i) << " & " << fmt(n[i], "%.0f") << " & " << fmt(bks[i]) << " & " << zm << " & " << gm;
        double mejor = 1e300;
        for (const auto& m : estoc) mejor = std::min(mejor, G[{i, m}].v());
        for (const auto& m : estoc) {
            double g = G[{i, m}].v();
            prom[m].add(g);
            t << " & " << (g == mejor ? "\\textbf{" + fmt(g) + "}" : fmt(g));
        }
        t << " \\\\\n";
    }
    t << "\\midrule\nPromedio & & & & ";
    for (const auto& m : estoc) t << " & " << fmt(prom[m].v());
    t << " \\\\\n\\bottomrule\n\\end{tabular}\n\\caption{GAP contra la BKS. Para los algoritmos "
         "estocásticos es el promedio de las corridas. $^*$: óptimo probado; $^\\dagger$: la "
         "solución del modelo viola el límite de longitud de ruta; n.e.: el modelo no escala a "
         "ese tamaño.}\n\\label{tab:comparacion_gap}\n\\end{table}\n";

    std::ofstream u(fs::path(DIR_TABLAS) / "comparacion_tiempo.tex");
    u << "\\begin{table}[htbp]\n\\centering\n\\small\n\\begin{tabular}{lrrrrrrr}\n\\toprule\n"
         "Instancia & \\multicolumn{3}{c}{$\\bar t^*$ (s)} & \\multicolumn{3}{c}{$\\bar\\imath^*$} & "
         "Modelo (s) \\\\\n\\cmidrule(lr){2-4}\\cmidrule(lr){5-7}\n & Prop. & AG & ACO & Prop. & AG "
         "& ACO & \\\\\n\\midrule\n";
    for (const auto& i : insts) {
        u << esc(i);
        for (const auto& m : estoc) u << " & " << fmt(T[{i, m}].v(), "%.1f");
        for (const auto& m : estoc) u << " & " << fmt(I[{i, m}].v(), "%.0f");
        double tm = modelo.count(i) ? valor(modelo[i], "segundos") : std::nan("");
        u << " & " << (modelo.count(i) && modelo[i].at("estado") == "no escala" ? "n.e." : fmt(tm, "%.1f"))
          << " \\\\\n";
    }
    u << "\\bottomrule\n\\end{tabular}\n\\caption{Segundo $t^*$ e iteración $i^*$ de la última "
         "mejora, promediados sobre las corridas, y tiempo del modelo.}\n"
         "\\label{tab:comparacion_tiempo}\n\\end{table}\n";
    std::cout << "Escritas tablas/comparacion_gap.tex y tablas/comparacion_tiempo.tex\n";
}

}  // namespace

void generar_tablas() {
    fs::create_directories(DIR_TABLAS);
    tablas_ajuste("ajuste_ag", {{"cruce", "Cruce"}, {"pm", "$p_m$"}, {"sel", "Selección"}}, "AG");
    tablas_ajuste("ajuste_aco", {{"alpha", "$\\alpha$"}, {"beta", "$\\beta$"}, {"rho", "$\\rho$"}}, "ACO");
    tablas_comparacion();
}
