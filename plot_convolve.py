import os
import sys

import matplotlib.pyplot as plt
import plot_style
import numpy as np

EXPERIMENT_DIR = "./output/experiments"
FIGURE_DIR = "./output/figures"


def load(path: str):
    """Return the CSV as a dict of named columns, or None if absent."""
    if not os.path.exists(path):
        print(f"no {path}; skipping")
        return None

    data = np.genfromtxt(path, delimiter=",", names=True)
    return {name: np.atleast_1d(data[name]).astype(float) for name in data.dtype.names}


def print_table(cols, title):
    print(f"\n{title}")
    print(
        f"{'input (s)':>10}  {'rir (s)':>8}  {'median ms':>10}  "
        f"{'ms/s audio':>11}  {'RTF':>7}"
    )
    for i in range(len(cols["median_ms"])):
        print(
            f"{cols['input_seconds'][i]:>10.2f}  {cols['rir_seconds'][i]:>8.2f}  "
            f"{cols['median_ms'][i]:>10.2f}  "
            f"{cols['ms_per_second_audio'][i]:>11.2f}  {cols['rtf'][i]:>7.0f}"
        )


def plot_time(cols, out_path):
    """Convolution time vs input length, log-log."""
    fig, ax = plt.subplots(figsize=plot_style.FULL)
    ax.plot(
        cols["input_seconds"],
        cols["median_ms"],
        marker="o",
        markersize=5,
        color="tab:red",
        linewidth=1.2,
    )
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Dry input length (s)")
    ax.set_ylabel("Convolution time (ms)")
    ax.grid(True, which="both", color="0.9", linewidth=0.5)

    fig.tight_layout()
    fig.savefig(out_path, dpi=300)
    print(f"Wrote {out_path}")


def plot_rtf(cols, out_path):
    """Real-time factor: seconds of audio convolved per second of wall time."""
    fig, ax = plt.subplots(figsize=plot_style.FULL)
    ax.plot(
        cols["input_seconds"],
        cols["rtf"],
        marker="o",
        markersize=5,
        color="tab:green",
        linewidth=1.2,
    )
    ax.axhline(
        1.0,
        color="0.4",
        linestyle="--",
        linewidth=1.0,
        label="real time (RTF = 1)",
    )
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Dry input length (s)")
    ax.set_ylabel("Real-time factor (x)")
    ax.grid(True, which="both", color="0.9", linewidth=0.5)
    ax.legend(fontsize=8, loc="best")

    fig.tight_layout()
    fig.savefig(out_path, dpi=300)
    print(f"Wrote {out_path}")


def plot_rir(cols, out_path):
    """Cost per second of audio vs RIR length, at a fixed input length."""
    x = cols["rir_seconds"]
    y = cols["ms_per_second_audio"]

    fig, ax = plt.subplots(figsize=plot_style.FULL)
    ax.plot(x, y, marker="o", markersize=5, color="tab:purple", linewidth=1.2)

    # Only the measured RIR lengths get ticks — the default log minor labels
    # crowd four points into an unreadable axis.
    ax.set_xscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels([f"{v:g}" for v in x])
    ax.set_xticks([], minor=True)
    ax.set_xmargin(0.12)
    ax.set_xlabel("RIR length (s)")
    ax.set_ylabel("ms per second of audio")
    ax.set_ylim(0, y.max() * 1.25)
    ax.grid(True, color="0.9", linewidth=0.5)

    fig.tight_layout()
    fig.savefig(out_path, dpi=300)
    print(f"Wrote {out_path}")


def main() -> None:
    directory = sys.argv[1] if len(sys.argv) > 1 else EXPERIMENT_DIR
    os.makedirs(FIGURE_DIR, exist_ok=True)

    inputs = load(os.path.join(directory, "convolve_input_sweep.csv"))
    rirs = load(os.path.join(directory, "convolve_rir_sweep.csv"))

    if inputs is not None:
        print_table(inputs, "Convolution cost vs input length")
        plot_time(inputs, os.path.join(FIGURE_DIR, "convolve_time.png"))
        plot_rtf(inputs, os.path.join(FIGURE_DIR, "convolve_rtf.png"))

    if rirs is not None:
        print_table(rirs, "Convolution cost vs RIR length")
        plot_rir(rirs, os.path.join(FIGURE_DIR, "convolve_rir.png"))

    plt.show()


if __name__ == "__main__":
    main()
