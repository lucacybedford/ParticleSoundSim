import matplotlib

matplotlib.rcParams.update(
    {
        "font.size": 9,
        "axes.labelsize": 9,
        "axes.titlesize": 9,
        "xtick.labelsize": 8,
        "ytick.labelsize": 8,
        "legend.fontsize": 7.5,
        "legend.title_fontsize": 8,
        "lines.linewidth": 1.0,
        "lines.markersize": 3,
        "savefig.dpi": 300,
    }
)

# Paired sub-figure, ~3.0 in on the page. 9:5, as before.
PANEL = (3.4, 1.9)

# Single sub-figure at 0.6\textwidth, ~4.0 in on the page. 9:5, as before.
WIDE = (4.4, 2.45)

# The response-length figure, which was 7:4.5 rather than 9:5.
PANEL_TALL = (3.4, 2.19)

# A body panel carrying a legend inside the axes. Taller than PANEL so the
# legend can sit in reclaimed head-room rather than in a strip below the plot,
# which is where a third of the figure's height was going.
PANEL_LEGEND = (3.4, 2.2)

FULL = (6.5, 3.1)
