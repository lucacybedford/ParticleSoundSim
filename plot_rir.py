"""Plot the room impulse response histogram exported by app_offline.

Usage:
    python plot_rir.py [histogram_receiver0.csv] [cutoff_ms] [--paper] [--band=F]

If cutoff_ms is given, the x-axis is cut off at that time. Otherwise the
tail is cropped automatically at the -60 dB point.

--paper       also write paper_comparison.png: the Schroeder decay curve in a
              fixed 0-256 ms frame with 10 dB/division gridlines, matching the
              decay-curve figure from the Allen & Berkley image-source paper so
              the slopes can be overlaid directly.
--band=F      curve to use in paper mode, F being an octave-band centre
              frequency (e.g. --band=250). Defaults to the broadband total.
              The original ISM figure has no air absorption, so a low band is
              the fairer comparison.
"""

import sys

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MultipleLocator

# ISO octave band centre frequencies (Hz) matching the 8 energy bands.
BANDS_HZ = [63, 125, 250, 500, 1000, 2000, 4000, 8000]

# --- Room and atmosphere for the analytic Eyring-Norris prediction ----------
# Defaults mirror make_standard() in src/Scene.cpp and the Atmosphere defaults
# (src/app_offline.cpp uses make_standard() with a default Atmosphere).
ROOM_LX, ROOM_LY, ROOM_LZ = 3.432, 5.148, 4.29  # room dimensions (m)

# Each entry is (surface area in m^2, absorption per band). Surfaces here are
# frequency-flat (matching the ISM comparison run) but per-band arrays are
# supported, so frequency-dependent materials work without changing the code.
ROOM_SURFACES = [
    (ROOM_LX * ROOM_LY, [0.51] * 8),                 # floor
    (ROOM_LX * ROOM_LY, [0.51] * 8),                 # ceiling
    (2 * ROOM_LY * ROOM_LZ + 2 * ROOM_LX * ROOM_LZ,  # four walls combined
     [0.19] * 8),
]

# Atmosphere (matches struct Atmosphere defaults in include/Atmosphere.hpp).
ATM_TEMPERATURE_C = 20.0
ATM_HUMIDITY = 50.0       # % relative humidity
ATM_PRESSURE_KPA = 101.325


def sound_speed(temperature_c: float = ATM_TEMPERATURE_C) -> float:
    """Speed of sound (m/s), matching Atmosphere::sound_speed()."""
    return 343.2 * np.sqrt((273.15 + temperature_c) / 293.15)


def air_absorption_m(f: float,
                     temperature_c: float = ATM_TEMPERATURE_C,
                     humidity: float = ATM_HUMIDITY,
                     pressure_kpa: float = ATM_PRESSURE_KPA) -> float:
    """ISO 9613-1 air energy-attenuation coefficient m (nepers/m).

    Direct port of Atmosphere::absorption_dB_per_m / absorption_m in
    src/Atmosphere.cpp, so the prediction uses the same m the sim applies.
    """
    pr = 101.325   # reference pressure, kPa
    T0 = 293.15    # reference temperature, K (20 C)
    T01 = 273.16   # triple-point isotherm, K

    T = temperature_c + 273.15
    pa_pr = pressure_kpa / pr

    psat_pr = 10.0 ** (-6.8346 * (T01 / T) ** 1.261 + 4.6151)
    h = humidity * psat_pr / pa_pr

    frO = pa_pr * (24.0 + 4.04e4 * h * (0.02 + h) / (0.391 + h))
    frN = (pa_pr * (T / T0) ** -0.5 *
           (9.0 + 280.0 * h * np.exp(-4.170 * ((T / T0) ** (-1.0 / 3.0) - 1.0))))

    f2 = f * f
    alpha_db = (8.686 * f2 *
                (1.84e-11 / pa_pr * np.sqrt(T / T0) +
                 (T / T0) ** -2.5 *
                 (0.01275 * np.exp(-2239.1 / T) / (frO + f2 / frO) +
                  0.1068 * np.exp(-3352.0 / T) / (frN + f2 / frN))))
    return alpha_db / 4.343  # dB/m -> nepers/m (energy basis)


def eyring_norris_rt60():
    """Per-band Eyring-Norris RT60 (s) for the configured room and atmosphere.

    RT60 = K*V / (-S*ln(1 - alpha_bar) + 4*m*V), with K = 24*ln(10)/c.
    The 4*m*V term carries the frequency dependence when surfaces are flat.
    """
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
    """Schroeder energy decay curve in dB, normalised to 0 dB at t=0.

    The histogram bins already hold energy (not pressure), so the curve is the
    backward cumulative sum of the energy, normalised by the total energy.
    """
    edc = np.cumsum(energy[::-1])[::-1]  # energy remaining from time t onward
    total = edc[0]
    if total <= 0:
        return np.full_like(energy, -np.inf, dtype=float)
    return 10.0 * np.log10(edc / total)


def rt60_t30(time_ms: np.ndarray, energy: np.ndarray):
    """Estimate RT60 by a least-squares fit to the -5..-35 dB region (T30).

    Returns (rt60_seconds, fit_line_db) or (None, None) if the decay never
    reaches -35 dB (too few particles / run too short for a reliable fit).
    """
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
    paper_band = None  # None => broadband total
    positional = []
    for arg in sys.argv[1:]:
        if arg == "--paper":
            continue
        if arg.startswith("--band="):
            paper_band = int(arg.split("=", 1)[1])
            continue
        positional.append(arg)

    path = positional[0] if positional else "histogram_receiver0.csv"
    cutoff_ms = float(positional[1]) if len(positional) > 1 else None
    data = np.genfromtxt(path, delimiter=",", names=True)

    time_ms = data["time_ms"]
    total = data["total"]

    # Measured RT60 (Schroeder + T30 fit, ISO 3382), decay slope, and the
    # Eyring-Norris prediction. Slope (dB/s) = -60 / RT60.
    predicted = eyring_norris_rt60()
    print(f"{'Band':>9}  {'measured':>10}  {'slope':>10}  "
          f"{'Eyring':>10}  {'error':>8}")
    for b, f in enumerate(BANDS_HZ):
        rt60, _ = rt60_t30(time_ms, data[f"band{b}"])
        pred = predicted[b]
        if rt60 is None:
            print(f"{f:>6} Hz  {'n/a':>10}  {'n/a':>10}  "
                  f"{pred:>9.3f}s  {'--':>8}")
        else:
            err = 100.0 * (rt60 - pred) / pred
            print(f"{f:>6} Hz  {rt60:>9.3f}s  {-60.0 / rt60:>7.1f} dB/s  "
                  f"{pred:>9.3f}s  {err:>+7.1f}%")
    rt60_total, fit_line_total = rt60_t30(time_ms, total)
    if rt60_total is None:
        print(f"{'broadband':>9}  {'n/a':>10}")
    else:
        print(f"{'broadband':>9}  {rt60_total:>9.3f}s  "
              f"{-60.0 / rt60_total:>7.1f} dB/s")

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

    fig, (ax_total, ax_bands) = plt.subplots(2, 1, figsize=(6, 10), sharex=True)

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

    # The ISM paper figures all stop at 256 ms, so cap the energy-arrival
    # plots there for a like-for-like comparison.
    x_limit = min(x_max, 256.0)
    ax_total.set_xlim(0, x_limit)  # shared axis crops both subplots
    for ax in (ax_total, ax_bands):
        ax.set_box_aspect(1)  # force each panel to a square box
        # 10 equal divisions along x -> 9 interior gridlines.
        ax.xaxis.set_major_locator(MultipleLocator(x_limit / 10.0))
        ax.grid(True, axis="x", color="0.8", linewidth=0.5)
    fig.tight_layout()
    fig.savefig("rir_plot.png", dpi=150)

    # Schroeder energy decay curve + T30 fit, as a separate figure and file.
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
    fig_edc.savefig("edc_plot.png", dpi=150)

    # Paper-comparison mode: the decay curve in the same frame as the Allen &
    # Berkley image-source figure (0-256 ms, 10 dB/division) for a slope overlay.
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
        ax_paper.plot(time_ms, paper_decay, color="black",
                      label=f"Schroeder EDC ({curve_name})")
        if fit_paper is not None:
            ax_paper.plot(time_ms, fit_paper, "r--", linewidth=1.0,
                          label=(f"T30 fit: RT60 = {rt60_paper:.3f} s, "
                                 f"{-60.0 / rt60_paper:.0f} dB/s"))
        # Match the figure: fixed 0-256 ms span, 10 dB per division, curve
        # anchored at 0 dB so only the slope (not the offset) is compared.
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
        fig_paper.savefig("paper_comparison.png", dpi=150)

    plt.show()


if __name__ == "__main__":
    main()
