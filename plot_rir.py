import sys

import matplotlib.pyplot as plt
import plot_style
import numpy as np
from matplotlib.ticker import MultipleLocator

BANDS_HZ = [63, 125, 250, 500, 1000, 2000, 4000, 8000]

# Must match kRoom in src/app_experiments.cpp. "standard" is the flat-
# absorption ISM comparison room (make_standard); "real" is the single-material
# room (make_room) whose absorption varies with frequency. Changing this
# switches the room constants below, and therefore every Eyring-Norris
# reference derived from them.
ROOM = "standard"

_STANDARD_LX, _STANDARD_LY, _STANDARD_LZ = 3.432, 5.148, 4.29
_REAL_LX, _REAL_LY, _REAL_LZ = 4.0, 7.0, 3.0
_WOOD = [0.19, 0.19, 0.23, 0.25, 0.30, 0.37, 0.42, 0.42]  # materials::mSolidWood


def _room_geometry(room: str):
    """Return (dimensions, surfaces) for the selected room.

    Surfaces are (area m^2, per-band absorption) pairs. Grouping identical
    surfaces is safe because only the area-weighted mean absorption is used.
    """
    if room == "standard":
        lx, ly, lz = _STANDARD_LX, _STANDARD_LY, _STANDARD_LZ
        surfaces = [
            (lx * ly, [0.51] * 8),  # floor
            (lx * ly, [0.51] * 8),  # ceiling
            (2 * ly * lz + 2 * lx * lz, [0.19] * 8),  # four walls combined
        ]
    elif room == "real":
        lx, ly, lz = _REAL_LX, _REAL_LY, _REAL_LZ
        # make_room applies one material to all six surfaces
        surfaces = [(2 * (lx * ly + ly * lz + lx * lz), _WOOD)]
    else:
        raise ValueError(f"unknown ROOM {room!r}; expected 'standard' or 'real'")
    return (lx, ly, lz), surfaces


(ROOM_LX, ROOM_LY, ROOM_LZ), ROOM_SURFACES = _room_geometry(ROOM)

# Default input/output directories for the selected room, mirroring kOutDir.
# Figures are separated too, so a run in one room cannot overwrite the other's.
EXPERIMENT_DIR = (
    "./output/experiments" if ROOM == "standard" else "./output/experiments/real-room"
)
FIGURE_DIR = "./output/figures" if ROOM == "standard" else "./output/figures/real-room"


def resolve_dirs(directory=None):
    """Return (experiment_dir, figure_dir) for an optional explicit directory.

    Passing a directory overrides where data is read from, but the Eyring-Norris
    reference always follows ROOM — so reading one room's data while ROOM names
    the other silently produces a plot with the wrong reference line. Warn
    loudly and send the figure somewhere that matches the data.
    """
    import os

    if directory is None:
        return EXPERIMENT_DIR, FIGURE_DIR
    if os.path.abspath(directory) == os.path.abspath(EXPERIMENT_DIR):
        return directory, FIGURE_DIR

    is_real = os.path.basename(os.path.normpath(directory)) == "real-room"
    figure_dir = "./output/figures/real-room" if is_real else "./output/figures"
    named = "real" if is_real else "standard"
    if named != ROOM:
        print(
            f"WARNING: reading {directory} (looks like the {named!r} room) but "
            f"ROOM={ROOM!r} in plot_rir.py. The Eyring-Norris reference "
            f"describes the {ROOM!r} room and will be WRONG for this data. "
            f"Set ROOM={named!r} and re-run."
        )
    return directory, figure_dir


ATM_TEMPERATURE_C = 20.0
ATM_HUMIDITY = 50.0  # % relative humidity
ATM_PRESSURE_KPA = 101.325


def sound_speed(temperature_c: float = ATM_TEMPERATURE_C) -> float:
    return 343.2 * np.sqrt((273.15 + temperature_c) / 293.15)


def air_absorption_m(
    f: float,
    temperature_c: float = ATM_TEMPERATURE_C,
    humidity: float = ATM_HUMIDITY,
    pressure_kpa: float = ATM_PRESSURE_KPA,
) -> float:

    pr = 101.325
    T0 = 293.15
    T01 = 273.16

    T = temperature_c + 273.15
    pa_pr = pressure_kpa / pr

    psat_pr = 10.0 ** (-6.8346 * (T01 / T) ** 1.261 + 4.6151)
    h = humidity * psat_pr / pa_pr

    frO = pa_pr * (24.0 + 4.04e4 * h * (0.02 + h) / (0.391 + h))
    frN = (
        pa_pr
        * (T / T0) ** -0.5
        * (9.0 + 280.0 * h * np.exp(-4.170 * ((T / T0) ** (-1.0 / 3.0) - 1.0)))
    )

    f2 = f * f
    alpha_db = (
        8.686
        * f2
        * (
            1.84e-11 / pa_pr * np.sqrt(T / T0)
            + (T / T0) ** -2.5
            * (
                0.01275 * np.exp(-2239.1 / T) / (frO + f2 / frO)
                + 0.1068 * np.exp(-3352.0 / T) / (frN + f2 / frN)
            )
        )
    )
    return alpha_db / 4.343  # dB/m -> nepers/m (energy basis)


def eyring_norris_rt60():
    volume = ROOM_LX * ROOM_LY * ROOM_LZ
    total_area = sum(area for area, _ in ROOM_SURFACES)
    k = 24.0 * np.log(10.0) / sound_speed()

    rt60 = []
    for b, f in enumerate(BANDS_HZ):
        alpha_bar = sum(area * a[b] for area, a in ROOM_SURFACES) / total_area
        m = air_absorption_m(f)
        denom = -total_area * np.log(1.0 - alpha_bar) + 4.0 * m * volume
        rt60.append(k * volume / denom if denom > 0 else float("nan"))
    return rt60


def schroeder_db(energy: np.ndarray) -> np.ndarray:
    edc = np.cumsum(energy[::-1])[::-1]  # energy remaining from time t onward
    total = edc[0]
    if total <= 0:
        return np.full_like(energy, -np.inf, dtype=float)
    return 10.0 * np.log10(edc / total)


def rt60_t30(time_ms: np.ndarray, energy: np.ndarray):
    decay = schroeder_db(energy)
    upper, lower = -5.0, -35.0
    mask = (decay <= upper) & (decay >= lower) & np.isfinite(decay)
    if mask.sum() < 2:
        return None, None
    t = time_ms[mask] / 1000.0  # seconds
    slope, intercept = np.polyfit(t, decay[mask], 1)  # dB per second
    if slope >= 0:
        return None, None
    rt60 = -60.0 / slope
    fit_line = slope * (time_ms / 1000.0) + intercept
    return rt60, fit_line


def main() -> None:
    # Separate flags from positional arguments.
    paper_mode = "--paper" in sys.argv
    paper_band = None
    positional = []
    for arg in sys.argv[1:]:
        if arg == "--paper":
            continue
        if arg.startswith("--band="):
            paper_band = int(arg.split("=", 1)[1])
            continue
        positional.append(arg)

    path = (
        positional[0]
        if positional
        else "./output/standard/histogram_receiver_[0]_1000000.csv"
    )
    cutoff_ms = float(positional[1]) if len(positional) > 1 else None
    data = np.genfromtxt(path, delimiter=",", names=True)

    time_ms = data["time_ms"]
    total = data["total"]

    predicted = eyring_norris_rt60()
    print(f"Predicted: {predicted}")
    print(f"{'Band':>9}  {'measured':>10}  {'slope':>10}  {'Eyring':>10}  {'error':>8}")
    for b, f in enumerate(BANDS_HZ):
        rt60, _ = rt60_t30(time_ms, data[f"band{b}"])
        pred = predicted[b]
        if rt60 is None:
            print(f"{f:>6} Hz  {'n/a':>10}  {'n/a':>10}  {pred:>9.3f}s  {'--':>8}")
        else:
            err = 100.0 * (rt60 - pred) / pred
            print(
                f"{f:>6} Hz  {rt60:>9.3f}s  {-60.0 / rt60:>7.1f} dB/s  "
                f"{pred:>9.3f}s  {err:>+7.1f}%"
            )
    rt60_total, fit_line_total = rt60_t30(time_ms, total)
    if rt60_total is None:
        print(f"{'broadband':>9}  {'n/a':>10}")
    else:
        print(f"{'broadband':>9}  {rt60_total:>9.3f}s  {-60.0 / rt60_total:>7.1f} dB/s")

    if cutoff_ms is not None:
        x_max = cutoff_ms
    else:
        peak = total.max()
        if peak > 0:
            significant = np.flatnonzero(total > peak * 1e-6)  # -60 dB threshold
            last = significant[-1] if significant.size else len(total) - 1
            x_max = time_ms[last] * 1.1
        else:
            x_max = time_ms[-1]

    fig, (ax_total, ax_bands) = plt.subplots(2, 1, figsize=(6, 10), sharex=True)

    # Broadband RIR energy decay.
    ax_total.bar(
        time_ms,
        total,
        width=np.diff(time_ms, append=time_ms[-1]),
        align="edge",
        color="tab:blue",
    )
    ax_total.set_ylabel("Energy")
    ax_total.set_title("Room impulse response (broadband)")

    for b, f in enumerate(BANDS_HZ):
        ax_bands.plot(time_ms, data[f"band{b}"], label=f"{f} Hz", linewidth=0.8)
    ax_bands.set_xlabel("Time (ms)")
    ax_bands.set_ylabel("Energy")
    ax_bands.set_title("Per-octave-band energy")
    ax_bands.legend(ncol=4, fontsize=8)

    # The ISM paper figures all stop at 256 ms, so cap the energy-arrival
    # plots there for comparison.
    x_limit = min(x_max, 256.0)
    ax_total.set_xlim(0, x_limit)  # shared axis crops both subplots
    for ax in (ax_total, ax_bands):
        ax.set_box_aspect(1)  # force each panel to a square box
        # 10 equal divisions along x -> 9 interior gridlines.
        ax.xaxis.set_major_locator(MultipleLocator(x_limit / 10.0))
        ax.grid(True, axis="x", color="0.8", linewidth=0.5)
    fig.tight_layout()
    fig.savefig("./output/figures/rir_plot.png", dpi=300)

    fig_edc, ax_edc = plt.subplots(figsize=(10, 5))
    decay_db = schroeder_db(total)
    ax_edc.plot(time_ms, decay_db, color="tab:blue", label="Schroeder EDC")
    if fit_line_total is not None:
        label = f"T30 fit (RT60 = {rt60_total:.3f} s)"
        ax_edc.plot(time_ms, fit_line_total, "r--", linewidth=1.2, label=label)
    for level in (-5.0, -35.0):
        ax_edc.axhline(level, color="0.7", linewidth=0.6, linestyle=":")
    ax_edc.set_xlim(0, x_max)
    ax_edc.set_ylim(-65, 1)
    ax_edc.set_xlabel("Time (ms)")
    ax_edc.set_ylabel("Energy decay (dB)")
    ax_edc.set_title("Schroeder energy decay curve (broadband)")
    ax_edc.legend(fontsize=8)
    fig_edc.tight_layout()
    fig_edc.savefig("./output/figures/edc_plot.png", dpi=300)

    if paper_mode:
        if paper_band is None:
            curve_energy = total
            curve_name = "broadband"
        elif paper_band in BANDS_HZ:
            curve_energy = data[f"band{BANDS_HZ.index(paper_band)}"]
            curve_name = f"{paper_band} Hz"
        else:
            raise SystemExit(f"--band must be one of {BANDS_HZ}, got {paper_band}")

        paper_decay = schroeder_db(curve_energy)
        rt60_paper, fit_paper = rt60_t30(time_ms, curve_energy)

        fig_paper, ax_paper = plt.subplots(figsize=(7, 7))
        ax_paper.plot(
            time_ms, paper_decay, color="black", label=f"Schroeder EDC ({curve_name})"
        )
        if fit_paper is not None:
            ax_paper.plot(
                time_ms,
                fit_paper,
                "r--",
                linewidth=1.0,
                label=(
                    f"T30 fit: RT60 = {rt60_paper:.3f} s, {-60.0 / rt60_paper:.0f} dB/s"
                ),
            )
        ax_paper.set_xlim(0, 256)
        ax_paper.set_ylim(-80, 0)
        ax_paper.xaxis.set_major_locator(MultipleLocator(32))
        ax_paper.yaxis.set_major_locator(MultipleLocator(10))  # 10 dB / DIV
        ax_paper.grid(True, which="major", color="0.8", linewidth=0.5)
        ax_paper.set_xlabel("Time (ms)")
        ax_paper.set_ylabel("Decay (10 dB / DIV)")
        ax_paper.set_title("Decay curve (image-source paper comparison)")
        ax_paper.legend(fontsize=8, loc="upper right")
        fig_paper.tight_layout()
        fig_paper.savefig("./output/figures/edc_paper_comparison.png", dpi=300)

    plt.show()


if __name__ == "__main__":
    main()
