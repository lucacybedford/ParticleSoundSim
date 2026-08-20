import glob
import os
import sys

import matplotlib.pyplot as plt
import plot_style
import numpy as np

from plot_rir import resolve_dirs

POOL_GLOB = "variance_pool_*.csv"

MIN_SET_SIZE = 2
MAX_SET_SIZE = 20

# Sets drawn per size
NUM_SETS = 2000

RNG_SEED = 12345


def find_pool(directory: str, tag: str = None):
    """Return (path, tag) for the requested pool, or the only one present."""
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
            f"several pools in {directory}, name one as the third argument:\n  {names}"
        )
    tag = os.path.basename(paths[0])[len("variance_pool_") : -len(".csv")]
    return paths[0], tag


def load_pool(path: str, metric: str):
    """Return the valid metric values from the run pool."""
    data = np.genfromtxt(path, delimiter=",", names=True)
    values = np.atleast_1d(data[metric]).astype(float)

    # rt60 == -1 flags a run that never decayed far enough to fit a slope
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
        picks = np.array(
            [rng.choice(len(pool), size=k, replace=False) for _ in range(num_sets)]
        )
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

    ylabel = r"Mean $T_{30}$ (s)" if metric == "rt60" else r"Mean $C_{50}$ (dB)"

    fig, ax = plt.subplots(figsize=plot_style.FULL)
    ax.boxplot(
        value_arrays,
        positions=set_sizes,
        widths=0.5,
        showmeans=True,
        meanprops={
            "marker": "o",
            "markerfacecolor": "tab:orange",
            "markeredgecolor": "tab:orange",
            "markersize": 3,
        },
        medianprops={"color": "tab:blue"},
        flierprops={"marker": ".", "markersize": 3, "markerfacecolor": "0.5"},
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
    ax.set_xticks(set_sizes)
    ax.set_xticklabels(set_sizes)
    ax.grid(True, axis="y", color="0.85", linewidth=0.5)

    # Print the spread per set size
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
    fig.savefig(out_path, dpi=300)
    print(f"\nWrote {out_path}")
    plt.show()


if __name__ == "__main__":
    main()
