import glob
import os
import re
import sys

import matplotlib.pyplot as plt
import plot_style
import numpy as np

from plot_rir import BANDS_HZ, eyring_norris_rt60, resolve_dirs

# "particles" or "dt"
SWEEP_AXIS = "dt"

PLOT_EDC = False

# Eyring-Norris reference for rt60
EYRING_BANDS_HZ = (500, 1000)


def eyring_reference():
    """(value, label) for the RT60 reference line."""
    per_band = eyring_norris_rt60()
    indices = [BANDS_HZ.index(f) for f in EYRING_BANDS_HZ]
    value = float(np.mean([per_band[i] for i in indices]))
    return value, r"Eyring-Norris $T_{60}$ (mid)"


AXES = {
    "particles": {
        "runs_glob": "sweep_runs_*.csv",
        # the dt files live alongside these, so exclude the "dt" prefix
        "runs_re": re.compile(r"sweep_runs_(\d+)\.csv$"),
        "edc_glob": "edc_*.csv",
        "edc_re": re.compile(r"edc_(\d+)\.csv$"),
        "xlabel": "Particle count",
        "title_suffix": "vs particle count",
        "legend_title": "Particles",
        "fig_prefix": "sweep_",
        "tick": lambda v: f"{v:,}",
        "col_header": "count",
    },
    "dt": {
        "runs_glob": "sweep_runs_dt*.csv",
        "runs_re": re.compile(r"sweep_runs_dt(\d+)\.csv$"),
        "edc_glob": "edc_dt*.csv",
        "edc_re": re.compile(r"edc_dt(\d+)\.csv$"),
        "xlabel": "Time step dt (ms)",
        "title_suffix": "vs time step",
        "legend_title": "dt (ms)",
        "fig_prefix": "sweep_dt_",
        "tick": lambda v: f"{v}",
        "col_header": "dt (ms)",
    },
}


def load_run_sets(directory: str, metric: str, axis: dict):
    """Return (axis_values, value_arrays) sorted by axis value."""
    entries = []
    for path in glob.glob(os.path.join(directory, axis["runs_glob"])):
        m = axis["runs_re"].search(os.path.basename(path))
        if not m:
            continue
        value = int(m.group(1))

        data = np.genfromtxt(path, delimiter=",", names=True)
        if data.dtype.names is None or metric not in data.dtype.names:
            continue
        values = np.atleast_1d(data[metric]).astype(float)

        # rt60 == -1 flags a run that never decayed far enough to fit a slope.
        valid = values[values > 0] if metric == "rt60" else values
        dropped = len(values) - len(valid)
        if dropped:
            print(
                f"{os.path.basename(path)}: dropped {dropped} invalid {metric} value(s)"
            )
        if len(valid) == 0:
            print(f"{os.path.basename(path)}: no valid {metric} values, skipping")
            continue

        entries.append((value, valid))

    entries.sort(key=lambda e: e[0])
    axis_values = [v for v, _ in entries]
    value_arrays = [a for _, a in entries]
    return axis_values, value_arrays


def boxplot_metric(directory, metric, ylabel, out_path, axis, reference=None):
    """`reference` draws a horizontal (value, label) line. Eyring for RT60."""
    axis_values, value_arrays = load_run_sets(directory, metric, axis)
    title = f"{ylabel.split(' (')[0]} {axis['title_suffix']}"
    if not axis_values:
        print(f"no {axis['runs_glob']} with valid {metric} in {directory}; skipping")
        return

    fig, ax = plt.subplots(figsize=plot_style.FULL)
    # Evenly spaced positions
    positions = np.arange(1, len(axis_values) + 1)
    ax.boxplot(
        value_arrays,
        positions=positions,
        widths=0.5,
        showmeans=True,
        meanprops={
            "marker": "o",
            "markerfacecolor": "tab:orange",
            "markeredgecolor": "tab:orange",
            "markersize": 4,
        },
        medianprops={"color": "tab:blue"},
        flierprops={"marker": ".", "markersize": 4, "markerfacecolor": "0.5"},
    )
    if reference is not None:
        ref_value, ref_label = reference
        ax.axhline(
            ref_value,
            color="tab:green",
            linestyle="--",
            linewidth=1.2,
            zorder=0,
            label=f"{ref_label} ({ref_value:.3f})",
        )
        ax.legend(fontsize=8, loc="best")

    ax.set_xlabel(axis["xlabel"])
    ax.set_ylabel(ylabel)
    ax.set_xticks(positions)
    ax.set_xticklabels([axis["tick"](v) for v in axis_values], rotation=45, ha="right")
    ax.grid(True, axis="y", color="0.85", linewidth=0.5)

    print(f"\n{title}")
    ref_col = f"  {'vs ref':>8}" if reference is not None else ""
    print(f"{axis['col_header']:>10}  {'n':>4}  {'mean':>10}  {'std':>10}{ref_col}")
    for v, vals in zip(axis_values, value_arrays):
        row = f"{v:>10}  {len(vals):>4}  {np.mean(vals):>10.4f}  {np.std(vals):>10.4f}"
        if reference is not None:
            dev = 100.0 * (np.mean(vals) - reference[0]) / reference[0]
            row += f"  {dev:>+7.1f}%"
        print(row)

    fig.tight_layout()
    fig.savefig(out_path, dpi=300)
    print(f"Wrote {out_path}")


def plot_runtime(directory, out_path, axis):
    """Mean simulation-stage runtime per sweep point, with std error bars."""
    axis_values, value_arrays = load_run_sets(directory, "runtime_ms", axis)
    if not axis_values:
        print(f"no runtime_ms column in {axis['runs_glob']}; skipping runtime plot")
        return

    means = np.array([np.mean(v) for v in value_arrays]) / 1e3  # seconds
    stds = np.array([np.std(v) for v in value_arrays]) / 1e3

    fig, ax = plt.subplots(figsize=plot_style.FULL)
    positions = np.arange(1, len(axis_values) + 1)
    ax.errorbar(
        positions,
        means,
        yerr=stds,
        marker="o",
        markersize=5,
        color="tab:red",
        capsize=3,
        linewidth=1.2,
    )
    ax.set_yscale("log")
    ax.set_xlabel(axis["xlabel"])
    ax.set_ylabel("Simulation runtime (s)")
    ax.set_xticks(positions)
    ax.set_xticklabels([axis["tick"](v) for v in axis_values], rotation=45, ha="right")
    ax.grid(True, which="both", color="0.9", linewidth=0.5)

    print(f"\nRuntime {axis['title_suffix']}")
    print(f"{axis['col_header']:>10}  {'mean (s)':>10}  {'std (s)':>10}")
    for v, m, s in zip(axis_values, means, stds):
        print(f"{v:>10}  {m:>10.3f}  {s:>10.3f}")

    fig.tight_layout()
    fig.savefig(out_path, dpi=300)
    print(f"Wrote {out_path}")


def load_edc_curves(directory: str, axis: dict):
    """Return list of (axis_value, time_ms, edc_db) sorted by axis value."""
    curves = []
    for path in glob.glob(os.path.join(directory, axis["edc_glob"])):
        m = axis["edc_re"].search(os.path.basename(path))
        if not m:
            continue
        value = int(m.group(1))
        data = np.genfromtxt(path, delimiter=",", names=True)
        curves.append(
            (value, np.atleast_1d(data["time_ms"]), np.atleast_1d(data["edc_db"]))
        )
    curves.sort(key=lambda c: c[0])
    return curves


def main() -> None:
    directory, out_dir = resolve_dirs(sys.argv[1] if len(sys.argv) > 1 else None)
    os.makedirs(out_dir, exist_ok=True)

    axis = AXES[SWEEP_AXIS]
    prefix = axis["fig_prefix"]

    boxplot_metric(
        directory,
        "rt60",
        # Boxes are T30 (ISO 3382-1, fitted -5 to -35 dB, extrapolated to 60 dB);
        r"$T_{30}$ (s)",
        os.path.join(out_dir, prefix + "rt60.png"),
        axis,
        reference=eyring_reference(),
    )
    boxplot_metric(
        directory,
        "c50",
        r"$C_{50}$ (dB)",
        os.path.join(out_dir, prefix + "c50.png"),
        axis,
    )

    plot_runtime(directory, os.path.join(out_dir, prefix + "runtime.png"), axis)

    # All averaged EDC curves superimposed.
    curves = load_edc_curves(directory, axis) if PLOT_EDC else []
    if curves:
        fig, ax = plt.subplots(figsize=plot_style.FULL)
        cmap = plt.get_cmap("viridis")
        for i, (value, time_ms, edc_db) in enumerate(curves):
            color = cmap(i / max(1, len(curves) - 1))
            ax.plot(
                time_ms,
                edc_db,
                color=color,
                linewidth=1.0,
                label=axis["tick"](value),
            )
        ax.set_xlabel("Time (ms)")
        ax.set_ylabel("Energy decay (dB)")
        ax.set_ylim(top=1)
        ax.grid(True, color="0.9", linewidth=0.5)
        ax.legend(title=axis["legend_title"], fontsize=8, ncol=2)
        out_path = os.path.join(out_dir, prefix + "edc.png")
        fig.tight_layout()
        fig.savefig(out_path, dpi=300)
        print(f"Wrote {out_path}")
    elif not PLOT_EDC:
        print("PLOT_EDC is off; skipping per-point EDC overlay")
    else:
        print(f"no {axis['edc_glob']} in {directory}; skipping EDC plot")

    plt.show()


if __name__ == "__main__":
    main()
