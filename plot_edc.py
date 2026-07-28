"""Averaged energy decay curves from the experiment app's `edc` mode.

Two curves at the same configuration, differing only in scattering:

    edc_specular.csv    s = 0   — comparable to a specular ISM baseline
    edc_scattering.csv  s > 0   — the scene's own scattering coefficients

The s = 0 curve is the one to hold against the Allen 1979 figures; the gap
between the two is what diffuse scattering does to the decay.

    python plot_edc.py [glob_dir]
        glob_dir  directory to search (default ./output/experiments)
"""

import os
import sys

import matplotlib.pyplot as plt
import numpy as np

from plot_rir import BANDS_HZ, eyring_norris_rt60, resolve_dirs

# The ISM paper figures stop at 256 ms; matching that makes the visual
# comparison direct. Set to None to plot the full decay.
XLIM_MS = 256

CURVES = [
    ("edc_specular.csv", "s = 0 (specular)", "tab:blue"),
    ("edc_scattering.csv", "s > 0 (scattering)", "tab:orange"),
]


def load_curve(path):
    data = np.genfromtxt(path, delimiter=",", names=True)
    return np.atleast_1d(data["time_ms"]), np.atleast_1d(data["edc_db"])


def main() -> None:
    directory, out_dir = resolve_dirs(sys.argv[1] if len(sys.argv) > 1 else None)
    os.makedirs(out_dir, exist_ok=True)

    fig, ax = plt.subplots(figsize=(9, 5))
    plotted = 0
    for filename, label, color in CURVES:
        path = os.path.join(directory, filename)
        if not os.path.exists(path):
            print(f"{path} not found; skipping")
            continue
        time_ms, edc_db = load_curve(path)
        ax.plot(time_ms, edc_db, color=color, linewidth=1.2, label=label)
        plotted += 1

        # decay rate over the T30 window (-5 to -35 dB, ISO 3382-1), matching
        # metrics::rt60 and plot_rir.rt60_t30 so numbers are comparable
        window = (edc_db <= -5.0) & (edc_db >= -35.0)
        if window.sum() >= 2:
            slope = np.polyfit(time_ms[window] / 1e3, edc_db[window], 1)[0]
            print(f"{label}: RT60 from T30 slope = {-60.0 / slope:.4f} s")

    if plotted == 0:
        raise SystemExit(
            f"no EDC curves in {directory}; run ParticleSoundSimExperiments edc"
        )

    # Eyring-Norris decay for reference, drawn as a straight -60 dB/RT60 line
    mid = np.mean([eyring_norris_rt60()[BANDS_HZ.index(f)] for f in (500, 1000)])
    t = np.linspace(0, XLIM_MS if XLIM_MS else 1000, 2)
    ax.plot(
        t,
        -60.0 * (t / 1e3) / mid,
        color="tab:green",
        linestyle="--",
        linewidth=1.0,
        label=f"Eyring-Norris ({mid:.3f} s)",
    )

    if XLIM_MS:
        ax.set_xlim(0, XLIM_MS)
    ax.set_ylim(-60, 1)
    ax.set_xlabel("Time (ms)")
    ax.set_ylabel("Energy decay (dB)")
    ax.set_title("Averaged energy decay curve: specular vs scattering")
    ax.grid(True, color="0.9", linewidth=0.5)
    ax.legend(fontsize=9)

    out_path = os.path.join(out_dir, "edc_scattering_comparison.png")
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    print(f"Wrote {out_path}")

    plt.show()


if __name__ == "__main__":
    main()
