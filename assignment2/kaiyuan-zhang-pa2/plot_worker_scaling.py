#!/usr/bin/env python3
"""Plot predicted-vs-measured completion time for 100 concurrent requests."""

import csv
import os
import statistics

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
DATA = os.path.join(HERE, "worker-scaling-100.csv")
OUT = os.environ.get(
    "PA2_PLOT_OUT",
    os.path.join(ROOT, "output", "pdf", "pa2-worker-scaling-100.pdf"),
)

# Match the visual language of fig-6-cut-model-atoms.pdf.
EST, MEASURED = "#555555", "#e94b4b"
BAR_EDGE, BAR_FACE = "#8a5724", "#f1e2cf"
INK, GRID, AXIS, SURF = "#111111", "#bbb8b0", "#8f8c84", "#ffffff"


rows = []
with open(DATA, newline="", encoding="utf-8") as f:
    for row in csv.DictReader(f):
        trials = [float(row[f"trial{i}_s"]) for i in range(1, 4)]
        rows.append(
            {
                "workers": int(row["workers"]),
                "predicted": float(row["predicted_s"]),
                "average": statistics.mean(trials),
            }
        )

plt.rcParams.update(
    {
        "font.family": "sans-serif",
        "font.size": 9,
        "font.weight": "bold",
        "axes.titleweight": "bold",
        "axes.labelweight": "bold",
        "axes.edgecolor": AXIS,
        "axes.linewidth": 0.9,
        "xtick.color": EST,
        "ytick.color": EST,
        "axes.labelcolor": INK,
        "text.color": INK,
    }
)

fig, ax = plt.subplots(figsize=(4.7, 2.65))
x = list(range(len(rows)))
workers = [r["workers"] for r in rows]
predicted = [r["predicted"] for r in rows]
measured = [r["average"] for r in rows]
acceleration = [measured[0] / value for value in measured]

ax2 = ax.twinx()
# A twin axis is normally painted after the primary axis. Move the entire bar
# axis behind the primary axis so both time curves remain unobstructed.
ax2.set_zorder(0)
ax.set_zorder(2)
ax.patch.set_visible(False)
bars = ax2.bar(
    x,
    acceleration,
    width=0.42,
    facecolor=BAR_FACE,
    edgecolor=BAR_EDGE,
    linewidth=1.3,
    hatch="///",
    label="Acceleration ratio $T_1/T_p$",
    zorder=1,
)
ax2.set_ylabel("Acceleration Ratio ($\\times$)")
ax2.set_ylim(0, 55)
ax2.set_yticks([0, 10, 20, 30, 40, 50])
ax2.tick_params(axis="y", colors=EST, width=0.8, direction="in", length=3.5)
ax2.spines["right"].set_color(AXIS)
ax2.grid(False)

ax.plot(
    x,
    predicted,
    color=EST,
    lw=1.2,
    marker="o",
    ms=4.2,
    mfc=SURF,
    mec=EST,
    mew=1.2,
    label="Ideal model $T_{\\mathrm{pred}}$",
    zorder=5,
)
ax.plot(
    x,
    measured,
    color=MEASURED,
    lw=2.0,
    marker="s",
    ms=5.0,
    mfc=MEASURED,
    mec=MEASURED,
    label="Measured average $T_{\\mathrm{meas}}$",
    zorder=4,
)

ax.set_xticks(x, workers)
ax.set_xlabel("Thread Pool Size")
ax.set_ylabel("Time $T$ (s)")
ax.set_ylim(0, 110)
ax.grid(color=GRID, lw=0.8)
ax.set_axisbelow(True)
ax.set_facecolor(SURF)
ax.tick_params(width=0.8, direction="in", length=3.5)
line_handles, line_labels = ax.get_legend_handles_labels()
ax.legend(
    line_handles + [bars],
    line_labels + ["Acceleration ratio $T_1/T_p$"],
    frameon=False,
    fontsize=7.2,
    loc="upper center",
    bbox_to_anchor=(0.5, -0.22),
    ncol=2,
    handlelength=1.8,
    columnspacing=1.0,
)

fig.patch.set_facecolor(SURF)
fig.tight_layout()
os.makedirs(os.path.dirname(OUT), exist_ok=True)
fig.savefig(OUT, bbox_inches="tight")
fig.savefig(OUT.replace(".pdf", ".png"), dpi=180, bbox_inches="tight")
print("wrote", OUT)
