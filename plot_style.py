"""Shared figure style for every plotter in this project.

The report places these figures in paired sub-figures at roughly 0.44-0.46 of a
170 mm text block, so each one lands about 3.0 in wide on the page. Generating
them at the old figsize=(9, 5) meant LaTeX scaled them by 0.33, which put 10 pt
axis labels on the page at 3.3 pt and 8 pt legends at 2.6 pt. That is legible on
a high-DPI laptop panel and on nothing else, which is why the sizes below are
close to the final printed size: the scale factor is now about 0.9, so a 9 pt
label stays a 9 pt label.

Aspect ratios match the old 9:5 (and 7:4.5 for the RIR-length figure) on
purpose. The height a figure occupies on the page follows from its width and its
aspect, so keeping the aspect keeps the page breaks where they are, and the body
is one page from its limit.

Import this module for its side effect before creating any figure, and take
figure sizes from PANEL / WIDE / PANEL_TALL rather than hard-coding them.
"""

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

# Deliberately absent: constrained_layout and savefig.bbox="tight". Every
# plotter here already calls fig.tight_layout(), which conflicts with the first
# and is made redundant by it. The second would crop each figure to its content,
# changing the aspect ratio by an amount that varies per figure, and the aspect
# is what fixes how much vertical space a figure takes on the page.

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

# Appendix figures. Appendix A is excluded from the page limit, so these are
# placed one per row at the full text width (169 mm) instead of paired at half
# width. The scale factor is then about 1.0, which keeps 9 pt text at 9 pt and
# leaves a plot region roughly twice the width of the old paired panels. The
# 2.1 aspect is deliberately squarer than the 2.7 first tried, which left the
# boxes looking squashed.
FULL = (6.5, 3.1)
