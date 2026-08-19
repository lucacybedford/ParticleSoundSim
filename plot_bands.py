"""Per-band RT60 from the experiment app, against the Eyring-Norris prediction.

Reads <dir>/config_bands_<tag>.csv (band_index, rt60_mean, rt60_std,
valid_runs) and overlays the per-band prediction from plot_rir. Two figures are
produced, because they answer different questions:

    config_bands_rt60.png    reference vs optimised — does the simulator match
                             theory per band, and does the cheap configuration
                             reproduce the expensive one?
    scatter_bands_rt60.png   the scattering sweep — is the frequency-dependent
                             part of the residual a diffuse-field effect?

Only tags present on disk are plotted, so a figure whose experiment has not
been run is skipped rather than failing.

    python plot_bands.py [glob_dir]
        glob_dir  directory to search (default: EXPERIMENT_DIR for the
                  selected ROOM in plot_rir.py)
"""

import csv
import glob
import os
import re
import sys

import matplotlib.pyplot as plt
import plot_style
import numpy as np

from plot_rir import BANDS_HZ, eyring_norris_rt60, resolve_dirs

# Display names. s = scene is the physical coefficient of the room's material;
# the other s values are diagnostics, not configurations anything is reported at.
#
# The reference and optimised labels carry a particle count and a step, and
# those are read from config_summary.csv rather than written here, because the
# chosen configuration has changed several times and a stale legend is not
# visible in the figure it is wrong in.
# Simulated series are T30, the ISO 3382-1 estimator fitted over -5 to -35 dB
# and extrapolated to a full 60 dB decay. The Eyring-Norris line is a true T60.
# Both sit on the same seconds-to-decay-60-dB scale, which is why the axis is
# generic and the estimator is carried in the legend instead.
LABELS = {
    "reference": r"Reference",
    "optimised": r"Optimised",
    "s000": r"$s = 0$ (specular)",
    "scene": r"$s = 0.1$ (scene)",
    "s050": r"$s = 0.5$",
    "s100": r"$s = 1.0$",
}


def config_labels(directory: str):
    """Return LABELS with the reference and optimised rows annotated.

    Reads the particle count and step from config_summary.csv when it is
    present, so the legend cannot disagree with the data beside it.
    """
    labels = dict(LABELS)
    path = os.path.join(directory, "config_summary.csv")
    if not os.path.exists(path):
        return labels
    with open(path, newline="") as handle:
        for row in csv.DictReader(handle):
            tag = row.get("label")
            if tag not in ("reference", "optimised"):
                continue
            n = int(row["num_particles"])
            count = f"{n // 1000}k" if n < 1_000_000 else f"{n // 1_000_000}M"
            labels[tag] = f"{labels[tag]} ({count}, {int(row['dt_ms'])} ms)"
    return labels

# (output filename, title, tags in plotting order)
FIGURES = [
    (
        "config_bands_rt60.png",
        "Per-band reverberation time against Eyring-Norris",
        ["reference", "optimised"],
    ),
    (
        "scatter_bands_rt60.png",
        "Per-band reverberation time against Eyring-Norris, per scattering coefficient",
        ["s000", "scene", "s050", "s100"],
    ),
]

MARKERS = ["o", "s", "^", "v", "D", "P", "X", "*"]


def discover(directory: str):
    """Map tag -> path for every config_bands_<tag>.csv in `directory`."""
    found = {}
    for path in glob.glob(os.path.join(directory, "config_bands_*.csv")):
        match = re.search(r"config_bands_(.+)\.csv$", os.path.basename(path))
        if match:
            found[match.group(1)] = path
    return found


def plot_figure(found, tags, out_path, title, predicted, bands, labels=None):
    """Plot the given tags against the prediction. Returns True if written."""
    labels = LABELS if labels is None else labels
    present = [t for t in tags if t in found]
    if not present:
        print(f"none of {tags} found; skipping {os.path.basename(out_path)}")
        return False

    fig, ax = plt.subplots(figsize=plot_style.PANEL_LEGEND)
    # Black dashed so the prediction reads as a reference rather than as
    # another series, and cannot collide with a tab10 colour.
    ax.plot(
        bands,
        predicted,
        color="black",
        linestyle="--",
        linewidth=1.4,
        label=r"Eyring-Norris $T_{60}$",
        zorder=3,
    )

    cmap = plt.get_cmap("tab10")
    print(f"\n=== {title}")
    for i, tag in enumerate(present):
        label = labels.get(tag, tag)
        data = np.genfromtxt(found[tag], delimiter=",", names=True)
        mean = np.atleast_1d(data["rt60_mean"])
        std = np.atleast_1d(data["rt60_std"])
        ax.errorbar(
            bands,
            mean,
            yerr=std,
            color=cmap(i % 10),
            marker=MARKERS[i % len(MARKERS)],
            markersize=5,
            capsize=3,
            linewidth=1.2,
            label=label,
            zorder=2,
        )

        deviation = 100.0 * (mean - predicted) / predicted
        print(f"\n{label}")
        print(f"{'band':>8}  {'measured':>9}  {'Eyring':>9}  {'dev':>7}")
        for f, m, p, d in zip(BANDS_HZ, mean, predicted, deviation):
            print(f"{f:>8}  {m:>9.4f}  {p:>9.4f}  {d:>+6.1f}%")
        print(
            f"{'':>8}  mean {deviation.mean():>+6.2f}%   "
            f"spread {deviation.max() - deviation.min():>5.1f} pp"
        )

    ax.set_xscale("log")
    ax.set_xticks(bands)
    ax.set_xticklabels(
        [f"{int(f) // 1000}k" if f >= 1000 else f"{int(f)}" for f in bands]
    )
    ax.minorticks_off()
    ax.set_xlabel("Octave band centre (Hz)")
    ax.set_ylabel(r"$T_{30}$ (s)")
    # title suppressed: the LaTeX caption carries it (see plot_style)
    # ax.set_title(title)
    ax.grid(True, color="0.9", linewidth=0.5)
    # Legend inside the axes in a floating box. Below the axes it took about a
    # third of the figure height, which is height the plot itself can use. The
    # y-limit is opened up first so the box lands in empty space rather than on
    # the curves.
    low, high = ax.get_ylim()
    ax.set_ylim(low, high + 0.58 * (high - low))
    ax.legend(
        loc="upper left",
        ncol=2,
        frameon=True,
        framealpha=0.9,
        edgecolor="0.8",
        borderpad=0.4,
        handlelength=1.6,
        columnspacing=1.0,
        handletextpad=0.5,
    )

    fig.tight_layout()
    fig.savefig(out_path, dpi=300, bbox_inches="tight")
    print(f"Wrote {out_path}")
    return True


def main() -> None:
    directory, out_dir = resolve_dirs(sys.argv[1] if len(sys.argv) > 1 else None)
    os.makedirs(out_dir, exist_ok=True)

    found = discover(directory)
    if not found:
        raise SystemExit(f"no config_bands_*.csv in {directory}")

    predicted = np.array(eyring_norris_rt60())
    bands = np.array(BANDS_HZ, dtype=float)

    labels = config_labels(directory)
    for filename, title, tags in FIGURES:
        plot_figure(
            found,
            tags,
            os.path.join(out_dir, filename),
            title,
            predicted,
            bands,
            labels,
        )

    plt.show()


if __name__ == "__main__":
    main()
