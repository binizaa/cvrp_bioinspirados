#include "geneticos.hpp"

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace {

struct Individuo {
    std::vector<int> pi;
    Rutas rutas;
    double f = 0;
    std::uint64_t clave = 0;
};

Rutas decodificar(const std::vector<int>& pi, const Instancia& in) {
    Rutas rutas;
    Ruta actual;
    int carga = 0, prev = 0;
    double dur = 0;
    for (int v : pi) {
        double nueva_dur = dur + in.d(prev, v) + in.s;
        if (!actual.empty() && (carga + in.q[v] > in.Q || nueva_dur + in.d(v, 0) > in.L + EPS)) {
            rutas.push_back(std::move(actual));
            actual = {v};
            carga = in.q[v];
            dur = in.d(0, v) + in.s;
        } else {
            actual.push_back(v);
            carga += in.q[v];
            dur = nueva_dur;
        }
        prev = v;
    }
    if (!actual.empty()) rutas.push_back(std::move(actual));
    return rutas;
}

std::uint64_t mezclar(std::uint64_t x) {   // splitmix64
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

std::uint64_t forma_canonica(const Rutas& rutas) {
    std::vector<std::uint64_t> hs;
    hs.reserve(rutas.size());
    for (const auto& r : rutas) {
        Ruta s = r;
        std::sort(s.begin(), s.end());
        std::uint64_t h = 0;
        for (int v : s) h = mezclar(h ^ static_cast<std::uint64_t>(v));
        hs.push_back(h);
    }
    std::sort(hs.begin(), hs.end());
    std::uint64_t H = 0;
    for (auto h : hs) H = mezclar(H ^ h);
    return H;
}

Individuo evaluar(std::vector<int> pi, const Instancia& in, bool propuesto) {
    Individuo ind;
    ind.rutas = decodificar(pi, in);
    if (propuesto)
        for (auto& r : ind.rutas) dos_opt(r, in);
    ind.f = costo(ind.rutas, in);
    if (propuesto) ind.clave = forma_canonica(ind.rutas);
    ind.pi = std::move(pi);
    return ind;
}

// Hijo de PMX: segmento [a, b] (inclusive, base 0) copiado de p1.
std::vector<int> pmx(const std::vector<int>& p1, const std::vector<int>& p2, int a, int b) {
    const int n = static_cast<int>(p1.size());
    std::vector<int> hijo(n, -1), pos2(n + 1);
    std::vector<char> en_segmento(n + 1, 0);
    for (int i = 0; i < n; ++i) pos2[p2[i]] = i;
    for (int i = a; i <= b; ++i) {
        hijo[i] = p1[i];
        en_segmento[p1[i]] = 1;
    }
    for (int i = a; i <= b; ++i) {
        int v = p2[i];
        if (en_segmento[v]) continue;
        int k = i;
        do k = pos2[p1[k]];
        while (a <= k && k <= b);
        hijo[k] = v;
    }
    for (int i = 0; i < n; ++i)
        if (hijo[i] < 0) hijo[i] = p2[i];
    return hijo;
}

}  // namespace

Resultado Genetico::resolver(const Instancia& in, std::uint64_t semilla, double tiempo) const {
    if (in.n < 2) throw std::runtime_error("Se necesitan al menos 2 clientes");
    Cronometro reloj;
    std::mt19937_64 rng(semilla);
    std::uniform_real_distribution<double> U(0.0, 1.0);
    auto entero = [&](int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); };

    std::vector<int> clientes(in.n);
    std::iota(clientes.begin(), clientes.end(), 1);

    std::vector<Individuo> pob;
    std::unordered_set<std::uint64_t> claves;
    long intentos = 0;
    while (static_cast<int>(pob.size()) < Np_) {
        if (++intentos > 1000L * Np_)
            throw std::runtime_error("No hay suficientes soluciones distintas; reduce Np");
        std::shuffle(clientes.begin(), clientes.end(), rng);
        Individuo ind = evaluar(clientes, in, propuesto_);
        if (!propuesto_ || claves.insert(ind.clave).second) pob.push_back(std::move(ind));
    }

    auto it_mejor = std::min_element(pob.begin(), pob.end(),
                                     [](const Individuo& a, const Individuo& b) { return a.f < b.f; });
    Individuo mejor = *it_mejor;
    Resultado res;
    res.historial.push_back({0, reloj.segundos(), mejor.f});
    long it = 0;

    auto torneo = [&]() -> const Individuo& {
        int a = entero(0, Np_ - 1), b = entero(0, Np_ - 2);
        if (b >= a) ++b;
        return pob[a].f <= pob[b].f ? pob[a] : pob[b];
    };

    while (reloj.segundos() < tiempo && (max_iter_ == 0 || it < max_iter_)) {
        ++it;
        int k = 0;   // peor individuo
        for (int i = 1; i < Np_; ++i)
            if (pob[i].f > pob[k].f) k = i;

        const Individuo *p1, *p2;
        if (seleccion_ == "ruleta") {
            // Escalamiento lineal w = f_peor - f: con 1/f todas las aptitudes
            // quedan casi iguales y la selección se vuelve aleatoria.
            std::vector<double> w(Np_);
            double suma = 0;
            for (int i = 0; i < Np_; ++i) suma += (w[i] = pob[k].f - pob[i].f);
            if (suma <= EPS) std::fill(w.begin(), w.end(), 1.0);
            std::discrete_distribution<int> ruleta(w.begin(), w.end());
            p1 = &pob[ruleta(rng)];
            p2 = &pob[ruleta(rng)];
        } else {
            p1 = &torneo();
            p2 = &torneo();
        }

        bool cruzo = U(rng) < pc_;
        std::vector<int> h1, h2;
        if (cruzo) {
            // 1 punto: el segmento heredado va del inicio al corte;
            // 2 puntos: el tramo entre los dos cortes, como en el Entregable 2.
            int a, b;
            if (cruce_ == 1) {
                a = 0;
                b = entero(0, in.n - 2);
            } else {
                a = entero(0, in.n - 1);
                b = entero(0, in.n - 2);
                if (b >= a) ++b;
                if (a > b) std::swap(a, b);
            }
            h1 = pmx(p1->pi, p2->pi, a, b);
            h2 = pmx(p2->pi, p1->pi, a, b);
        } else {
            h1 = p1->pi;
            h2 = p2->pi;
        }

        std::vector<Individuo> hijos;
        for (auto [h, padre] : {std::pair{&h1, p1}, std::pair{&h2, p2}}) {
            bool muto = U(rng) < pm_;
            if (muto) {
                int i = entero(0, in.n - 1), j = entero(0, in.n - 2);
                if (j >= i) ++j;
                std::swap((*h)[i], (*h)[j]);
            }
            // Sin cruce ni mutación el hijo es copia de su padre
            Individuo e = (cruzo || muto) ? evaluar(*h, in, propuesto_) : *padre;
            if (!propuesto_ || !claves.count(e.clave)) hijos.push_back(std::move(e));
        }

        if (hijos.empty()) continue;
        Individuo& hijo = hijos.size() == 2 && hijos[1].f < hijos[0].f ? hijos[1] : hijos[0];
        if (hijo.f < pob[k].f - EPS) {
            if (propuesto_) {
                claves.erase(pob[k].clave);
                claves.insert(hijo.clave);
            }
            bool mejora = hijo.f < mejor.f - EPS;
            pob[k] = std::move(hijo);
            if (mejora) {
                mejor = pob[k];
                res.it_mejora = it;
                res.t_mejora = reloj.segundos();
                res.historial.push_back({it, res.t_mejora, mejor.f});
            }
        }
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
