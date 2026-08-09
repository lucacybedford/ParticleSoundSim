"""Pooling test: how much does averaging k runs tighten the RT60 estimate?

Reads a pool of independent runs written by the experiment app's `variance`
mode as output/experiments/variance_pool_<particles>_dt<ms>.csv (columns:
run_index,seed,rt60,c50,runtime_ms). For every set size k from 2 to 20 it draws
many random sets of k distinct runs from that pool, takes the mean metric of
each set, and draws one box per k. The boxes show the sampling distribution of
a k-run pooled estimate: the median tracks the pool mean while the spread
shrinks, so the plot says how many runs have to be pooled before the estimate
stops moving.

Sets are drawn without replacement within a set (a run never appears twice in
one set) but sets are independent of each other, so k=20 gets the same number
of boxplot samples as k=2. The draw is seeded, so the figure is reproducible.

Several pools coexist, one per configuration, so the tag is carried into the
figure name and a run with more than one pool present refuses to guess.

    python plot_variance.py [metric] [glob_dir] [tag]
        metric    rt60 (default) or c50
        glob_dir  directory to search (default ./output/experiments)
        tag       which pool, e.g. 100000_dt1; required when several exist
"""

import glob
import os
import sys

import matplotlib.pyplot as plt
import numpy as np

from plot_rir import resolve_dirs

# Pools are named variance_pool_<particles>_dt<ms>.csv by the experiment app.
# The tag is carried into the figure name too, so plotting one pool cannot
# overwrite the figure belonging to another.
POOL_GLOB = "variance_pool_*.csv"

# Set sizes to test, inclusive. Matches kVarianceMinRuns/kVariancePrefixMax in
# src/app_experiments.cpp.
MIN_SET_SIZE = 2
MAX_SET_SIZE = 20

# Sets drawn per size. Large enough that the box shape is a property of the
# pool rather than of the draw.
NUM_SETS = 2000

RNG_SEED = 12345


def find_pool(directory: str, tag: str = None):
    """Return (path, tag) for the requested pool, or the only one present.

    Several pools coexist by design — one per configuration — so a bare run has
    to refuse rather than guess when more than one is available.
    """
    if tag:
        path = os.path.join(directory, f"variance_pool_{tag}.csv")
        if not os.path.exists(path):
            raise SystemExit(f"{path} not found")
        return path, tag

    paths = sorted(glob.glob(os.path.join(directory, POOL_GLOB)))
    if not paths:
        raise SystemExit(
            f"no {POOL_GLOB} in {directory} — run the experiment app's "
            f"`variance` mode first"
        )
    if len(paths) > 1:
        names = "\n  ".join(
            os.path.basename(p)[len("variance_pool_") : -len(".csv")] for p in paths
        )
        raise SystemExit(
            f"several pools in {directory}, name one as the third argument:\n"
            f"  {names}"
        )
    tag = os.path.basename(paths[0])[len("variance_pool_") : -len(".csv")]
    return paths[0], tag


def load_pool(path: str, metric: str):
    """Return the valid metric values from the run pool."""
    data = np.genfromtxt(path, delimiter=",", names=True)
    values = np.atleast_1d(data[metric]).astype(float)

    # rt60 == -1 flags a run that never decayed far enough to fit a slope; drop
    # those so they don't distort the pooled means.
    valid = values[values > 0] if metric == "rt60" else values
    name = os.path.basename(path)
    dropped = len(values) - len(valid)
    if dropped:
        print(f"{name}: dropped {dropped} invalid {metric} value(s)")
    if len(valid) == 0:
        raise SystemExit(f"{name}: no valid {metric} values")

    print(f"{name}: pool of {len(valid)} runs")
    return valid


def pooled_means(pool: np.ndarray, set_sizes, num_sets: int, rng):
    """Return a list of per-set mean arrays, one array per set size."""
    means = []
    for k in set_sizes:
        # One row per set, each row k distinct run indices.
        picks = np.array([rng.choice(len(pool), size=k, replace=False)
                          for _ in range(num_sets)])
        means.append(pool[picks].mean(axis=1))
    return means


def main() -> None:
    args = [a for a in sys.argv[1:]]
    metric = args[0] if args else "rt60"
    directory, figure_dir = resolve_dirs(args[1] if len(args) > 1 else None)

    if metric not in ("rt60", "c50"):
        raise SystemExit(f"metric must be 'rt60' or 'c50', got '{metric}'")

    pool_path, tag = find_pool(directory, args[2] if len(args) > 2 else None)
    pool = load_pool(pool_path, metric)

    set_sizes = list(range(MIN_SET_SIZE, MAX_SET_SIZE + 1))
    if max(set_sizes) > len(pool):
        raise SystemExit(
            f"pool holds {len(pool)} valid runs, too few for sets of "
            f"{max(set_sizes)} — lower MAX_SET_SIZE or rerun the pool"
        )

    rng = np.random.default_rng(RNG_SEED)
    value_arrays = pooled_means(pool, set_sizes, NUM_SETS, rng)

    ylabel = "Mean RT60 (s)" if metric == "rt60" else "Mean C50 (dB)"

    fig, ax = plt.subplots(figsize=(9, 5))
    ax.boxplot(
        value_arrays,
        positions=set_sizes,
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

    # The pool mean is what every set size is estimating.
    ax.axhline(
        float(np.mean(pool)),
        color="tab:green",
        linestyle="--",
        linewidth=1,
        label=f"Pool mean ({len(pool)} runs)",
    )
    ax.legend(loc="best", frameon=False)

    ax.set_xlabel("Runs pooled per set")
    ax.set_ylabel(ylabel)
    ax.set_title(f"{ylabel} vs runs pooled per set ({NUM_SETS} sets per size)")
    ax.set_xticks(set_sizes)
    ax.set_xticklabels(set_sizes)
    ax.grid(True, axis="y", color="0.85", linewidth=0.5)

    # Print the spread per set size next to the 1/sqrt(k) prediction, so any
    # departure from ideal independent averaging is visible numerically.
    pool_std = float(np.std(pool, ddof=1))
    print(
        f"\npool mean {np.mean(pool):.4f}  std {pool_std:.4f}\n"
        f"\n{'k':>4}  {'mean':>10}  {'std':>10}  {'std/sqrt(k)':>12}"
    )
    for k, vals in zip(set_sizes, value_arrays):
        print(
            f"{k:>4}  {np.mean(vals):>10.4f}  {np.std(vals, ddof=1):>10.4f}  "
            f"{pool_std / np.sqrt(k):>12.4f}"
        )

    out_dir = figure_dir
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, f"pooling_boxplot_{metric}_{tag}.png")
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"\nWrote {out_path}")
    plt.show()


if __name__ == "__main__":
    main()
