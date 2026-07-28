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

import glob
import os
import re
import sys

import matplotlib.pyplot as plt
import numpy as np

from plot_rir import BANDS_HZ, eyring_norris_rt60, resolve_dirs

# Display names. s = scene is the physical coefficient of the room's material;
# the other s values are diagnostics, not configurations anything is reported at.
LABELS = {
    "reference": "Reference (1M, 1 ms)",
    "optimised": "Optimised (200k, 20 ms)",
    "s000": "s = 0 (specular)",
    "scene": "s = 0.1 (scene value)",
    "s050": "s = 0.5",
    "s100": "s = 1.0",
}

# (output filename, title, tags in plotting order)
FIGURES = [
    (
        "config_bands_rt60.png",
        "Per-band RT60 against Eyring-Norris",
        ["reference", "optimised"],
    ),
    (
        "scatter_bands_rt60.png",
        "Per-band RT60 against Eyring-Norris, per scattering coefficient",
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


def plot_figure(found, tags, out_path, title, predicted, bands):
    """Plot the given tags against the prediction. Returns True if written."""
    present = [t for t in tags if t in found]
    if not present:
        print(f"none of {tags} found; skipping {os.path.basename(out_path)}")
        return False

    fig, ax = plt.subplots(figsize=(9, 5))
    # Black dashed so the prediction reads as a reference rather than as
    # another series, and cannot collide with a tab10 colour.
    ax.plot(
        bands,
        predicted,
        color="black",
        linestyle="--",
        linewidth=1.4,
        label="Eyring-Norris",
        zorder=3,
    )

    cmap = plt.get_cmap("tab10")
    print(f"\n=== {title}")
    for i, tag in enumerate(present):
        label = LABELS.get(tag, tag)
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
    ax.set_xticklabels([f"{int(f)}" for f in bands])
    ax.minorticks_off()
    ax.set_xlabel("Octave band centre frequency (Hz)")
    ax.set_ylabel("RT60 (s)")
    ax.set_title(title)
    ax.grid(True, color="0.9", linewidth=0.5)
    ax.legend(fontsize=9)

    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
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

    for filename, title, tags in FIGURES:
        plot_figure(
            found, tags, os.path.join(out_dir, filename), title, predicted, bands
        )

    plt.show()


if __name__ == "__main__":
    main()
