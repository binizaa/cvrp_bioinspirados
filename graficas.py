# Gráficas del reporte a partir de results/. Se corre desde esta carpeta:
#   python3 graficas.py
# Escribe cada figura en graficas/ como .pdf (para \includegraphics) y .png.
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

RESULTADOS = Path("results")
INSTANCIAS = Path("Reto_cvrp_primer")
SALIDA = Path("graficas")

# Un color fijo por método, en el mismo orden en todas las figuras
METODOS = ["modelo", "propuesto", "ag", "aco"]
NOMBRES = {"modelo": "Modelo (HiGHS)", "propuesto": "Propuesto", "ag": "AG clásico", "aco": "ACO (ASrank)"}
COLORES = {"modelo": "#2a78d6", "propuesto": "#eb6834", "ag": "#1baf7a", "aco": "#eda100"}
MARCAS = {"modelo": "s", "propuesto": "o", "ag": "^", "aco": "D"}

TINTA = "#0b0b0b"
TINTA_2 = "#52514e"
REJILLA = "#e4e3df"

plt.rcParams.update({
    "font.size": 10,
    "axes.edgecolor": TINTA_2,
    "axes.labelcolor": TINTA,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "axes.grid": True,
    "grid.color": REJILLA,
    "grid.linewidth": 0.8,
    "axes.axisbelow": True,
    "xtick.color": TINTA_2,
    "ytick.color": TINTA_2,
    "legend.frameon": False,
    "savefig.bbox": "tight",
    "savefig.dpi": 200,
})


def guardar(fig, nombre):
    for ext in ("pdf", "png"):
        fig.savefig(SALIDA / f"{nombre}.{ext}")
    plt.close(fig)
    print(f"Escrita graficas/{nombre}.pdf")


def orden_instancias(nombres):
    # CMT1..CMT14 en orden numérico y después las X por número de clientes
    def clave(s):
        if s.startswith("CMT"):
            return (0, int(s[3:]))
        return (1, int(s.split("-")[1][1:]))
    return sorted(nombres, key=clave)


def gap_por_metodo(comp):
    # Distribución del GAP de cada corrida, por método
    fig, ax = plt.subplots(figsize=(6.5, 3.8))
    datos = [comp.loc[comp.metodo == m, "gap_bks"].dropna() for m in METODOS]
    caja = ax.boxplot(datos, widths=0.5, patch_artist=True, showfliers=False,
                      medianprops={"color": TINTA, "linewidth": 1.5})
    for parche, m in zip(caja["boxes"], METODOS):
        parche.set_facecolor(COLORES[m])
        parche.set_alpha(0.35)
        parche.set_edgecolor(COLORES[m])
    for i, (d, m) in enumerate(zip(datos, METODOS), start=1):
        x = i + (pd.Series(range(len(d))) % 7 - 3) * 0.04
        ax.scatter(x, d, s=14, color=COLORES[m], edgecolor="white", linewidth=0.5, zorder=3)
    ax.set_xticks(range(1, len(METODOS) + 1), [f"{NOMBRES[m]}\n(n={len(d)})" for m, d in zip(METODOS, datos)])
    ax.set_ylabel("GAP contra BKS (%)")
    ax.set_title("GAP por método en las 24 instancias", loc="left", color=TINTA)
    ax.grid(axis="x", visible=False)
    guardar(fig, "gap_por_metodo")


def gap_por_instancia(comp):
    # GAP promedio de las corridas de cada método en cada instancia
    prom = comp.groupby(["instancia", "metodo"]).gap_bks.mean().unstack()
    orden = orden_instancias(prom.index)
    prom = prom.loc[orden]
    fig, ax = plt.subplots(figsize=(10, 4))
    x = range(len(orden))
    for j, m in enumerate(METODOS):
        if m in prom:
            # Pequeño desplazamiento horizontal para que no se encimen los métodos
            ax.scatter([i + (j - 1.5) * 0.15 for i in x], prom[m], s=36, marker=MARCAS[m], color=COLORES[m],
                       edgecolor="white", linewidth=0.6, label=NOMBRES[m], zorder=3)
    ax.set_xticks(list(x), orden, rotation=60, ha="right")
    ax.set_ylabel("GAP promedio (%)")
    ax.set_title("GAP promedio por instancia", loc="left", color=TINTA)
    ax.legend(ncol=4, loc="upper left", bbox_to_anchor=(0, 1.0))
    guardar(fig, "gap_por_instancia")


def convergencia(comp, instancia, semilla=1):
    # Mejor costo encontrado contra el tiempo, una corrida por método
    dh = RESULTADOS / "historial" / "comparacion"
    fig, ax = plt.subplots(figsize=(6.5, 3.8))
    bks = comp.loc[comp.instancia == instancia, "bks"].iloc[0]
    for m in METODOS[1:]:
        archivos = sorted(dh.glob(f"{instancia}__{m}__*__s{semilla}.csv"))
        if not archivos:
            continue
        h = pd.read_csv(archivos[0])
        fin = comp.loc[(comp.instancia == instancia) & (comp.metodo == m), "segundos"].max()
        t = list(h.t) + [fin]
        f = list(h.f) + [h.f.iloc[-1]]
        ax.step(t, f, where="post", color=COLORES[m], linewidth=2, label=NOMBRES[m])
    ax.axhline(bks, color=TINTA_2, linewidth=1, linestyle="--")
    ax.annotate(f"BKS = {bks:g}", (0.01, bks), xycoords=("axes fraction", "data"),
                xytext=(0, 3), textcoords="offset points",
                ha="left", va="bottom", color=TINTA_2, fontsize=9)
    ax.set_xscale("log")
    ax.set_xlabel("Tiempo (s, escala log)")
    ax.set_ylabel("Mejor costo Z")
    ax.set_title(f"Convergencia en {instancia} (semilla {semilla})", loc="left", color=TINTA)
    ax.legend()
    guardar(fig, f"convergencia_{instancia}")


def ajuste(archivo, nombre, titulo, color):
    # GAP promedio de cada configuración en las instancias de ajuste, de mejor a peor
    df = pd.read_csv(RESULTADOS / archivo)
    prom = df.groupby("config").gap_bks.agg(["mean", "std"]).sort_values("mean", ascending=False)
    fig, ax = plt.subplots(figsize=(6.5, 0.32 * len(prom) + 1.2))
    etiquetas = [c.replace(";", ", ") for c in prom.index]
    ax.barh(etiquetas, prom["mean"], height=0.7, color=color, edgecolor="white", linewidth=2)
    for y, v in enumerate(prom["mean"]):
        ax.text(v, y, f" {v:.2f}", va="center", fontsize=8, color=TINTA_2)
    ax.set_xlabel("GAP promedio (%)")
    ax.set_title(titulo, loc="left", color=TINTA)
    ax.grid(axis="y", visible=False)
    guardar(fig, nombre)


def leer_coordenadas(instancia):
    coords, leyendo = {}, False
    for linea in (INSTANCIAS / f"{instancia}.vrp").read_text().splitlines():
        if linea.startswith("NODE_COORD_SECTION"):
            leyendo = True
        elif leyendo and linea[:1].isalpha():
            break
        elif leyendo and linea.strip():
            i, x, y = linea.split()[:3]
            coords[int(i)] = (float(x), float(y))
    return coords


def rutas(comp, instancia):
    # Mejor solución encontrada por los algoritmos; en el .sol el cliente i es el nodo i+1
    filas = comp[(comp.instancia == instancia) & (comp.metodo != "modelo") & (comp.factible == 1)]
    mejor = filas.loc[filas.Z.idxmin()]
    config = mejor.config.replace("=", "").replace(";", "_")
    sol = RESULTADOS / "soluciones" / "comparacion" / f"{instancia}__{mejor.metodo}__{config}__s{mejor.semilla}.sol"
    coords = leer_coordenadas(instancia)
    deposito = coords[1]
    paleta = ["#2a78d6", "#eb6834", "#1baf7a", "#eda100", "#e87ba4", "#008300", "#6250d6", "#e34948"]
    fig, ax = plt.subplots(figsize=(5.5, 5.5))
    k = 0
    for linea in sol.read_text().splitlines():
        if not linea.startswith("Route"):
            continue
        nodos = [deposito] + [coords[int(c) + 1] for c in linea.split(":")[1].split()] + [deposito]
        xs, ys = zip(*nodos)
        ax.plot(xs, ys, color=paleta[k % len(paleta)], linewidth=1.5, marker="o", markersize=4,
                markeredgecolor="white", markeredgewidth=0.5)
        k += 1
    ax.scatter(*deposito, s=90, marker="s", color=TINTA, zorder=4, label="Depósito")
    ax.set_aspect("equal")
    ax.grid(False)
    ax.set_xticks([])
    ax.set_yticks([])
    ax.legend(loc="upper left", bbox_to_anchor=(0, -0.01))
    ax.set_title(f"{instancia}: {NOMBRES[mejor.metodo]}, Z = {mejor.Z:.2f}, {k} rutas",
                 loc="left", color=TINTA)
    guardar(fig, f"rutas_{instancia}")


def main():
    SALIDA.mkdir(exist_ok=True)
    comp = pd.read_csv(RESULTADOS / "comparacion.csv")
    gap_por_metodo(comp)
    gap_por_instancia(comp)
    for instancia in ("CMT1", "CMT11", "X-n280-k17"):
        convergencia(comp, instancia)
    ajuste("ajuste_ag.csv", "ajuste_ag", "Ajuste del AG: GAP promedio por configuración", COLORES["ag"])
    ajuste("ajuste_aco.csv", "ajuste_aco", "Ajuste del ACO: GAP promedio por configuración", COLORES["aco"])
    for instancia in ("CMT1", "CMT11"):
        rutas(comp, instancia)


if __name__ == "__main__":
    main()
