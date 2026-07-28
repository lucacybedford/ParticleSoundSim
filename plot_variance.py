"""Box plot of RT60 spread versus the number of runs in a set.

Reads every output/experiments/variance_<nRuns>.csv produced by the experiment
app's `variance` mode. Each file holds one row per run (columns:
run_index,seed,rt60,c50). One box is drawn per file: the x position is that
file's run count, the box summarises the RT60 values from those runs. Comparing
the boxes shows how the spread of RT60 settles as more runs are added.

    python plot_variance.py [metric] [glob_dir]
        metric    rt60 (default) or c50
        glob_dir  directory to search (default ./output/experiments)
"""

import glob
import os
import re
import sys

import matplotlib.pyplot as plt
import numpy as np

from plot_rir import resolve_dirs

FNAME_RE = re.compile(r"variance_(\d+)\.csv$")


def load_sets(directory: str, metric: str):
    """Return (run_counts, value_arrays) sorted by run count."""
    entries = []
    for path in glob.glob(os.path.join(directory, "variance_*.csv")):
        m = FNAME_RE.search(os.path.basename(path))
        if not m:
            continue
        n_runs = int(m.group(1))

        data = np.genfromtxt(path, delimiter=",", names=True)
        values = np.atleast_1d(data[metric]).astype(float)

        # rt60 == -1 flags a run that never decayed far enough to fit a slope;
        # drop those so they don't distort the box.
        valid = values[values > 0] if metric == "rt60" else values
        dropped = len(values) - len(valid)
        if dropped:
            print(
                f"{os.path.basename(path)}: dropped {dropped} invalid {metric} value(s)"
            )
        if len(valid) == 0:
            print(f"{os.path.basename(path)}: no valid {metric} values, skipping")
            continue

        entries.append((n_runs, valid))

    entries.sort(key=lambda e: e[0])
    run_counts = [n for n, _ in entries]
    value_arrays = [v for _, v in entries]
    return run_counts, value_arrays


def main() -> None:
    args = [a for a in sys.argv[1:]]
    metric = args[0] if args else "rt60"
    directory, figure_dir = resolve_dirs(args[1] if len(args) > 1 else None)

    if metric not in ("rt60", "c50"):
        raise SystemExit(f"metric must be 'rt60' or 'c50', got '{metric}'")

    run_counts, value_arrays = load_sets(directory, metric)
    if not run_counts:
        raise SystemExit(f"no variance_*.csv files found in {directory}")

    ylabel = "RT60 (s)" if metric == "rt60" else "C50 (dB)"

    fig, ax = plt.subplots(figsize=(9, 5))
    ax.boxplot(
        value_arrays,
        positions=run_counts,
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

    ax.set_xlabel("Number of runs in set")
    ax.set_ylabel(ylabel)
    ax.set_title(f"{ylabel} vs number of runs")
    ax.set_xticks(run_counts)
    ax.set_xticklabels(run_counts)
    ax.grid(True, axis="y", color="0.85", linewidth=0.5)

    # Print the spread (std) per set so the stabilisation is visible numerically.
    print(f"\n{'nRuns':>6}  {'n':>4}  {'mean':>10}  {'std':>10}")
    for n, vals in zip(run_counts, value_arrays):
        print(f"{n:>6}  {len(vals):>4}  {np.mean(vals):>10.4f}  {np.std(vals):>10.4f}")

    out_dir = figure_dir
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"variance_boxplot_{metric}.png")
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"\nWrote {out_path}")
    plt.show()


if __name__ == "__main__":
    main()
