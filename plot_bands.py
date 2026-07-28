"""Per-band RT60 from the experiment app's `config` mode, against Eyring-Norris.

Reads output/experiments/config_bands_<label>.csv (band_index, rt60_mean,
rt60_std, valid_runs) for each configuration and overlays the per-band
Eyring-Norris prediction from plot_rir.

The only frequency-dependent term in the prediction is ISO 9613 air
absorption — the room's absorption coefficients are flat across bands — so the
shape of this curve is largely a test of the air-absorption model.

    python plot_bands.py [glob_dir]
        glob_dir  directory to search (default ./output/experiments)
"""

import os
import sys

import matplotlib.pyplot as plt
import numpy as np

from plot_rir import BANDS_HZ, eyring_norris_rt60

CONFIGS = [
    ("config_bands_reference.csv", "Reference (1M, 1 ms)", "tab:blue", "o"),
    ("config_bands_optimised.csv", "Optimised (200k, 20 ms)", "tab:orange", "s"),
]


def main() -> None:
    directory = sys.argv[1] if len(sys.argv) > 1 else "./output/experiments"
    out_dir = "./output/figures"
    os.makedirs(out_dir, exist_ok=True)

    predicted = np.array(eyring_norris_rt60())
    bands = np.array(BANDS_HZ, dtype=float)

    fig, ax = plt.subplots(figsize=(9, 5))
    ax.plot(
        bands,
        predicted,
        color="tab:green",
        linestyle="--",
        linewidth=1.4,
        marker="^",
        markersize=5,
        label="Eyring-Norris",
        zorder=1,
    )

    for filename, label, color, marker in CONFIGS:
        path = os.path.join(directory, filename)
        if not os.path.exists(path):
            print(f"{path} not found; skipping")
            continue
        data = np.genfromtxt(path, delimiter=",", names=True)
        mean = np.atleast_1d(data["rt60_mean"])
        std = np.atleast_1d(data["rt60_std"])
        ax.errorbar(
            bands,
            mean,
            yerr=std,
            color=color,
            marker=marker,
            markersize=5,
            capsize=3,
            linewidth=1.2,
            label=label,
            zorder=2,
        )

        print(f"\n{label}")
        print(f"{'band':>8}  {'measured':>9}  {'Eyring':>9}  {'dev':>7}")
        for f, m, p in zip(BANDS_HZ, mean, predicted):
            print(f"{f:>8}  {m:>9.4f}  {p:>9.4f}  {100 * (m - p) / p:>+6.1f}%")

    ax.set_xscale("log")
    ax.set_xticks(bands)
    ax.set_xticklabels([f"{int(f)}" for f in bands])
    ax.minorticks_off()
    ax.set_xlabel("Octave band centre frequency (Hz)")
    ax.set_ylabel("RT60 (s)")
    ax.set_title("Per-band RT60 against Eyring-Norris")
    ax.grid(True, color="0.9", linewidth=0.5)
    ax.legend(fontsize=9)

    out_path = os.path.join(out_dir, "config_bands_rt60.png")
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"\nWrote {out_path}")

    plt.show()


if __name__ == "__main__":
    main()
