"""Plot the room impulse response histogram exported by app_offline.

Usage:
    python plot_rir.py [histogram_receiver0.csv] [cutoff_ms]

If cutoff_ms is given, the x-axis is cut off at that time. Otherwise the
tail is cropped automatically at the -60 dB point.
"""

import sys

import matplotlib.pyplot as plt
import numpy as np

# ISO octave band centre frequencies (Hz) matching the 8 energy bands.
BANDS_HZ = [63, 125, 250, 500, 1000, 2000, 4000, 8000]


def main() -> None:
    path = sys.argv[1] if len(sys.argv) > 1 else "histogram_receiver0.csv"
    cutoff_ms = float(sys.argv[2]) if len(sys.argv) > 2 else None
    data = np.genfromtxt(path, delimiter=",", names=True)

    time_ms = data["time_ms"]
    total = data["total"]

    if cutoff_ms is not None:
        # Manual cutoff: cut the x-axis off at the requested time.
        x_max = cutoff_ms
    else:
        # Crop the empty tail: keep up to the last bin within 60 dB of the peak
        # energy (the T60 point), plus a 10% margin so the decay isn't clipped.
        peak = total.max()
        if peak > 0:
            significant = np.flatnonzero(total > peak * 1e-6)  # -60 dB threshold
            last = significant[-1] if significant.size else len(total) - 1
            x_max = time_ms[last] * 1.1
        else:
            x_max = time_ms[-1]

    fig, (ax_total, ax_bands) = plt.subplots(2, 1, figsize=(10, 7), sharex=True)

    # Broadband RIR energy decay.
    ax_total.bar(time_ms, total, width=np.diff(time_ms, append=time_ms[-1]),
                 align="edge", color="tab:blue")
    ax_total.set_ylabel("Energy")
    ax_total.set_title("Room impulse response (broadband)")

    # Per-band energy, stacked so the spectral content is visible.
    for b, f in enumerate(BANDS_HZ):
        ax_bands.plot(time_ms, data[f"band{b}"], label=f"{f} Hz", linewidth=0.8)
    ax_bands.set_xlabel("Time (ms)")
    ax_bands.set_ylabel("Energy")
    ax_bands.set_title("Per-octave-band energy")
    ax_bands.legend(ncol=4, fontsize=8)

    ax_total.set_xlim(0, x_max)  # shared axis crops both subplots

    fig.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
