#include "hormigas.hpp"

#include <algorithm>
#include <numeric>
#include <random>

namespace {

constexpr double EPS_AHORRO = 1.0;   // cota inferior del ahorro mu_ij

struct Solucion {
    Rutas rutas;
    double f = 0;
};

template <class Peso, class Elegir>
Rutas construir(const Instancia& in, const std::vector<std::vector<int>>& vecinos, Peso peso,
                Elegir elegir) {
    std::vector<char> visitado(in.n + 1, 0);
    Rutas rutas;
    Ruta ruta;
    int i = 0, carga = 0, restantes = in.n;
    double dur = 0;
    std::vector<int> cand;
    std::vector<double> w;
    auto factible = [&](int j) {
        return !visitado[j] && carga + in.q[j] <= in.Q &&
               dur + in.d(i, j) + in.s + in.d(j, 0) <= in.L + EPS;
    };
    while (restantes > 0) {
        cand.clear();
        w.clear();
        for (int j : vecinos[i])
            if (factible(j)) { cand.push_back(j); w.push_back(peso(i, j)); }
        if (cand.empty())
            for (int j = 1; j <= in.n; ++j)
                if (factible(j)) { cand.push_back(j); w.push_back(peso(i, j)); }
        if (cand.empty()) {   // vehículo lleno: regresa y sale otro
            rutas.push_back(std::move(ruta));
            ruta.clear();
            i = carga = 0;
            dur = 0;
            continue;
        }
        int j = cand[elegir(cand, w)];
        ruta.push_back(j);
        visitado[j] = 1;
        carga += in.q[j];
        dur += in.d(i, j) + in.s;
        i = j;
        --restantes;
    }
    if (!ruta.empty()) rutas.push_back(std::move(ruta));
    return rutas;
}

}  // namespace

Resultado ASrank::resolver(const Instancia& in, std::uint64_t semilla, double tiempo) const {
    Cronometro reloj;
    std::mt19937_64 rng(semilla);
    const int N = in.n + 1;
    auto at = [N](int i, int j) { return static_cast<size_t>(i) * N + j; };

    // Lista de los vecinos más cercanos de cada nodo (solo clientes)
    std::vector<std::vector<int>> vecinos(N);
    for (int i = 0; i < N; ++i) {
        std::vector<int> v;
        for (int j = 1; j < N; ++j)
            if (j != i) v.push_back(j);
        size_t k = std::min<size_t>(candidatos_, v.size());
        std::partial_sort(v.begin(), v.begin() + k, v.end(),
                          [&](int a, int b) { return in.d(i, a) < in.d(i, b); });
        v.resize(k);
        vecinos[i] = std::move(v);
    }

    // eta^beta * mu^gamma, con mu_0j = 1 desde el depósito
    std::vector<double> heur(static_cast<size_t>(N) * N, 0.0);
    for (int i = 0; i < N; ++i)
        for (int j = 1; j < N; ++j) {
            if (i == j) continue;
            double eta = 1.0 / std::max(in.d(i, j), EPS);
            double mu = i == 0 ? 1.0 : std::max(in.d(i, 0) + in.d(0, j) - in.d(i, j), EPS_AHORRO);
            heur[at(i, j)] = std::pow(eta, beta_) * std::pow(mu, gamma_);
        }

    auto evaluar = [&](Rutas rutas) {
        Solucion s;
        for (auto& r : rutas) dos_opt(r, in);
        s.f = costo(rutas, in);
        s.rutas = std::move(rutas);
        return s;
    };

    // Vecino más cercano (peso -d, se toma el máximo): define tau_0 = 1 / L(S_vc).
    // Como la lista de candidatos son los k más cercanos, si alguno es
    // factible, el más cercano factible está en ella.
    Rutas r_vc = construir(in, vecinos, [&](int i, int j) { return -in.d(i, j); },
                           [](const std::vector<int>&, const std::vector<double>& w) {
                               return static_cast<size_t>(std::max_element(w.begin(), w.end()) - w.begin());
                           });
    double f_vc = costo(r_vc, in);
    Solucion mejor{r_vc, f_vc};

    std::vector<double> tau(static_cast<size_t>(N) * N, 1.0 / f_vc);
    std::vector<double> peso(static_cast<size_t>(N) * N);
    auto recalcular_peso = [&]() {
        for (size_t t = 0; t < peso.size(); ++t)
            peso[t] = (alpha_ == 1 ? tau[t] : std::pow(tau[t], alpha_)) * heur[t];
    };
    recalcular_peso();

    std::uniform_real_distribution<double> U(0.0, 1.0);
    auto ruleta = [&](const std::vector<int>&, const std::vector<double>& w) {
        double suma = std::accumulate(w.begin(), w.end(), 0.0);
        if (!(suma > 0) || !std::isfinite(suma))
            return static_cast<size_t>(std::max_element(w.begin(), w.end()) - w.begin());
        double r = U(rng) * suma;
        for (size_t t = 0; t < w.size(); ++t) {
            r -= w[t];
            if (r <= 0) return t;
        }
        return w.size() - 1;
    };
    auto peso_arco = [&](int i, int j) { return peso[at(i, j)]; };

    Resultado res;
    res.historial.push_back({0, reloj.segundos(), mejor.f});
    long it = 0;
    std::vector<Solucion> sols(hormigas_);
    while (reloj.segundos() < tiempo && (max_iter_ == 0 || it < max_iter_)) {
        ++it;
        for (auto& s : sols) s = evaluar(construir(in, vecinos, peso_arco, ruleta));
        std::sort(sols.begin(), sols.end(), [](const Solucion& a, const Solucion& b) { return a.f < b.f; });
        if (sols[0].f < mejor.f - EPS) {
            mejor = sols[0];
            res.it_mejora = it;
            res.t_mejora = reloj.segundos();
            res.historial.push_back({it, res.t_mejora, mejor.f});
        }

        // Evaporación y depósito por rangos: (sigma - r) / f(S_r) para las
        // sigma - 1 mejores de la iteración y sigma / f(S*) para la global
        for (auto& t : tau) t *= 1.0 - rho_;
        auto depositar = [&](const Solucion& s, double w) {
            for (const auto& r : s.rutas) {
                int prev = 0;
                for (int v : r) {
                    tau[at(prev, v)] += w;
                    tau[at(v, prev)] += w;
                    prev = v;
                }
                tau[at(prev, 0)] += w;
                tau[at(0, prev)] += w;
            }
        };
        for (int r = 1; r < sigma_ && r <= hormigas_; ++r) depositar(sols[r - 1], (sigma_ - r) / sols[r - 1].f);
        depositar(mejor, sigma_ / mejor.f);
        recalcular_peso();
    }

    res.metodo = nombre();
    res.config = config();
    res.semilla = semilla;
    res.segundos = reloj.segundos();
    res.iteraciones = it;
    res.Z = mejor.f;
    res.rutas = mejor.rutas;
    res.factible = verificar(res.rutas, in);
    res.historial.push_back({it, res.segundos, mejor.f});
    return res;
}
