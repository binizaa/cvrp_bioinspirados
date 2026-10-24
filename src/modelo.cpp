#include "modelo.hpp"

#include <vector>

#include "Highs.h"

Resultado ModeloExacto::resolver(const Instancia& in, std::uint64_t semilla, double tiempo) const {
    Resultado res;
    res.metodo = nombre();
    res.config = config();
    res.semilla = semilla;
    if (in.n > hasta_n_) {
        res.estado = "no escala";
        return res;
    }

    Cronometro reloj;
    const int N = in.n + 1;
    Highs h;
    h.setOptionValue("output_flag", false);
    h.setOptionValue("time_limit", tiempo);
    h.setOptionValue("mip_rel_gap", 1e-6);
    h.setOptionValue("random_seed", static_cast<HighsInt>(semilla));

    std::vector<HighsInt> col(static_cast<size_t>(N) * N, -1);
    HighsInt k = 0;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (i != j) {
                h.addCol(in.d(i, j), 0.0, 1.0, 0, nullptr, nullptr);
                col[static_cast<size_t>(i) * N + j] = k++;
            }
    const HighsInt num_x = k;
    std::vector<HighsInt> u(N, -1);
    for (int i = 1; i < N; ++i) {
        h.addCol(0.0, in.q[i], in.Q, 0, nullptr, nullptr);
        u[i] = k++;
    }
    std::vector<HighsVarType> enteras(num_x, HighsVarType::kInteger);
    h.changeColsIntegrality(0, num_x - 1, enteras.data());

    auto x = [&](int i, int j) { return col[static_cast<size_t>(i) * N + j]; };
    std::vector<HighsInt> idx;
    std::vector<double> val;
    auto fila = [&](double lo, double hi) {
        h.addRow(lo, hi, static_cast<HighsInt>(idx.size()), idx.data(), val.data());
        idx.clear();
        val.clear();
    };

    for (int j = 1; j < N; ++j) {
        for (int i = 0; i < N; ++i)
            if (i != j) { idx.push_back(x(i, j)); val.push_back(1); }
        fila(1, 1);
        for (int i = 0; i < N; ++i)
            if (i != j) { idx.push_back(x(j, i)); val.push_back(1); }
        fila(1, 1);
    }
    for (int j = 1; j < N; ++j) {
        idx.push_back(x(0, j)); val.push_back(1);
        idx.push_back(x(j, 0)); val.push_back(-1);
    }
    fila(0, 0);
    for (int j = 1; j < N; ++j) { idx.push_back(x(0, j)); val.push_back(1); }
    fila(in.k_min(), kHighsInf);
    // u_j - u_i - Q x_ij - (Q - q_i - q_j) x_ji >= q_j - Q
    for (int i = 1; i < N; ++i)
        for (int j = 1; j < N; ++j) {
            if (i == j) continue;
            double lev = in.Q - in.q[i] - in.q[j];
            idx = {u[j], u[i], x(i, j)};
            val = {1.0, -1.0, -static_cast<double>(in.Q)};
            if (lev != 0) { idx.push_back(x(j, i)); val.push_back(-lev); }
            fila(in.q[j] - in.Q, kHighsInf);
        }

    h.run();
    res.segundos = reloj.segundos();
    const auto& info = h.getInfo();
    res.iteraciones = static_cast<long>(info.mip_node_count);
    bool hay_solucion = info.primal_solution_status == kSolutionStatusFeasible;
    if (h.getModelStatus() == HighsModelStatus::kOptimal) res.estado = "optimo";
    else if (hay_solucion) res.estado = "factible";
    else res.estado = "sin solucion";
    if (!hay_solucion) return res;

    res.Z = info.objective_function_value;
    res.cota = res.estado == "optimo" ? res.Z : info.mip_dual_bound;
    const auto& v = h.getSolution().col_value;
    std::vector<int> succ(N, 0);
    for (int i = 1; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (i != j && v[x(i, j)] > 0.5) succ[i] = j;
    for (int j = 1; j < N; ++j)
        if (v[x(0, j)] > 0.5) {
            Ruta r{j};
            int nodo = j;
            while (succ[nodo] != 0 && r.size() <= static_cast<size_t>(in.n)) {
                nodo = succ[nodo];
                r.push_back(nodo);
            }
            res.rutas.push_back(r);
        }
    res.factible = verificar(res.rutas, in);
    res.historial = {{0, res.segundos, res.Z}};
    return res;
}
