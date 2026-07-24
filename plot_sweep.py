"""Plots for the experiment app's `sweep` mode.

Reads the sweep outputs from output/experiments:
    sweep_runs_<n>.csv  run_index,seed,rt60,c50   (one row per run, per count)
    edc_<n>.csv         time_ms,edc_db            (one averaged EDC curve/count)
    sweep_summary.csv   num_particles,avg_rt60,avg_c50,valid_rt60_runs (optional)

Produces three figures in output/figures:
    sweep_rt60.png      RT60 spread vs particle count (box plot)
    sweep_c50.png       C50  spread vs particle count (box plot)
    sweep_edc.png       all averaged EDC curves superimposed

    python plot_sweep.py [glob_dir]
        glob_dir  directory to search (default ./output/experiments)
"""

import glob
import os
import re
import sys

import matplotlib.pyplot as plt
import numpy as np

RUNS_RE = re.compile(r"sweep_runs_(\d+)\.csv$")
EDC_RE = re.compile(r"edc_(\d+)\.csv$")


def load_run_sets(directory: str, metric: str):
    """Return (counts, value_arrays) sorted by particle count."""
    entries = []
    for path in glob.glob(os.path.join(directory, "sweep_runs_*.csv")):
        m = RUNS_RE.search(os.path.basename(path))
        if not m:
            continue
        n = int(m.group(1))

        data = np.genfromtxt(path, delimiter=",", names=True)
        values = np.atleast_1d(data[metric]).astype(float)

        # rt60 == -1 flags a run that never decayed far enough to fit a slope.
        valid = values[values > 0] if metric == "rt60" else values
        dropped = len(values) - len(valid)
        if dropped:
            print(f"{os.path.basename(path)}: dropped {dropped} invalid {metric} value(s)")
        if len(valid) == 0:
            print(f"{os.path.basename(path)}: no valid {metric} values, skipping")
            continue

        entries.append((n, valid))

    entries.sort(key=lambda e: e[0])
    counts = [n for n, _ in entries]
    value_arrays = [v for _, v in entries]
    return counts, value_arrays


def boxplot_metric(directory, metric, ylabel, title, out_path):
    counts, value_arrays = load_run_sets(directory, metric)
    if not counts:
        print(f"no sweep_runs_*.csv with valid {metric} in {directory}; skipping")
        return

    fig, ax = plt.subplots(figsize=(9, 5))
    # Evenly spaced positions (counts span orders of magnitude, so real values
    # would bunch the boxes); real counts go on the tick labels instead.
    positions = np.arange(1, len(counts) + 1)
    ax.boxplot(
        value_arrays,
        positions=positions,
        widths=0.5,
        showmeans=True,
        meanprops={"marker": "o", "markerfacecolor": "tab:orange",
                   "markeredgecolor": "tab:orange", "markersize": 4},
        medianprops={"color": "tab:blue"},
        flierprops={"marker": ".", "markersize": 4, "markerfacecolor": "0.5"},
    )
    ax.set_xlabel("Particle count")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.set_xticks(positions)
    ax.set_xticklabels([f"{c:,}" for c in counts], rotation=45, ha="right")
    ax.grid(True, axis="y", color="0.85", linewidth=0.5)

    print(f"\n{title}")
    print(f"{'count':>10}  {'n':>4}  {'mean':>10}  {'std':>10}")
    for c, vals in zip(counts, value_arrays):
        print(f"{c:>10}  {len(vals):>4}  {np.mean(vals):>10.4f}  {np.std(vals):>10.4f}")

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"Wrote {out_path}")


def load_edc_curves(directory: str):
    """Return list of (num_particles, time_ms, edc_db) sorted by count."""
    curves = []
    for path in glob.glob(os.path.join(directory, "edc_*.csv")):
        m = EDC_RE.search(os.path.basename(path))
        if not m:
            continue
        n = int(m.group(1))
        data = np.genfromtxt(path, delimiter=",", names=True)
        curves.append((n, np.atleast_1d(data["time_ms"]), np.atleast_1d(data["edc_db"])))
    curves.sort(key=lambda c: c[0])
    return curves


def main() -> None:
    directory = sys.argv[1] if len(sys.argv) > 1 else "./output/experiments"
    out_dir = "./output/figures"
    os.makedirs(out_dir, exist_ok=True)

    boxplot_metric(
        directory, "rt60", "RT60 (s)", "RT60 spread vs particle count",
        os.path.join(out_dir, "sweep_rt60.png"),
    )
    boxplot_metric(
        directory, "c50", "C50 (dB)", "C50 spread vs particle count",
        os.path.join(out_dir, "sweep_c50.png"),
    )

    # All averaged EDC curves superimposed.
    curves = load_edc_curves(directory)
    if curves:
        fig, ax = plt.subplots(figsize=(9, 5))
        cmap = plt.get_cmap("viridis")
        for i, (n, time_ms, edc_db) in enumerate(curves):
            color = cmap(i / max(1, len(curves) - 1))
            ax.plot(time_ms, edc_db, color=color, linewidth=1.0, label=f"{n:,}")
        ax.set_xlabel("Time (ms)")
        ax.set_ylabel("Energy decay (dB)")
        ax.set_title("Averaged EDC curves by particle count")
        ax.set_ylim(top=1)
        ax.grid(True, color="0.9", linewidth=0.5)
        ax.legend(title="Particles", fontsize=8, ncol=2)
        out_path = os.path.join(out_dir, "sweep_edc.png")
        fig.tight_layout()
        fig.savefig(out_path, dpi=150)
        print(f"Wrote {out_path}")
    else:
        print(f"no edc_*.csv in {directory}; skipping EDC plot")

    plt.show()


if __name__ == "__main__":
    main()
