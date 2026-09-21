import json
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


PROJECT_ROOT = Path(__file__).resolve().parents[1]

CONFIG_PATH = (
        PROJECT_ROOT
        / "config"
        / "control_parameters.json"
)


def load_parameters():
    with open(CONFIG_PATH, "r") as file:
        config = json.load(file)

    return (
        float(config["kp"]),
        int(config["control_period_ms"])
    )


def closed_loop_poles(k: float):
    # Simplified charger current plant:
    #
    #           1
    # G(s) = --------
    #        s(s + 2)
    #
    # With proportional gain K and unity feedback:
    #
    # s^2 + 2s + K = 0

    return np.roots([1.0, 2.0, k])


kp, control_period_ms = load_parameters()

gains = np.linspace(0.0, 10.0, 500)

branch_1 = []
branch_2 = []

for gain in gains:
    poles = closed_loop_poles(gain)

    branch_1.append(poles[0])
    branch_2.append(poles[1])

branch_1 = np.asarray(branch_1)
branch_2 = np.asarray(branch_2)

selected_poles = closed_loop_poles(kp)

natural_frequency = np.sqrt(kp)
damping_ratio = 1.0 / natural_frequency

overshoot = np.exp(
    -damping_ratio * np.pi
    / np.sqrt(1.0 - damping_ratio ** 2)
) * 100.0


print(f"Configuration: {CONFIG_PATH}")
print(f"Selected gain: Kp = {kp}")
print(f"Embedded control period: {control_period_ms} ms")

for index, pole in enumerate(selected_poles, start=1):
    print(f"Closed-loop pole {index}: {pole:.3f}")

print(
    f"Natural frequency: "
    f"{natural_frequency:.3f} rad/s"
)

print(
    f"Damping ratio: "
    f"{damping_ratio:.3f}"
)

print(
    f"Theoretical step overshoot: "
    f"{overshoot:.2f}%"
)


# -------------------------
# Plot root locus
# -------------------------

fig, ax = plt.subplots(figsize=(8, 6))

# Root-locus branches
# Both branches use the same color because they belong
# to the same root locus.
ax.plot(
    branch_1.real,
    branch_1.imag,
    linewidth=2.0,
    color="C0",
    label="Root locus",
    zorder=2
)

ax.plot(
    branch_2.real,
    branch_2.imag,
    linewidth=2.0,
    color="C0",
    zorder=2
)

# Open-loop poles: s = -2 and s = 0
ax.scatter(
    [-2.0, 0.0],
    [0.0, 0.0],
    marker="x",
    s=110,
    linewidths=2.2,
    color="C1",
    zorder=4,
    label="Open-loop poles"
)

# Selected closed-loop poles for the Kp loaded
# from control_parameters.json.
#
# White-filled circles with black borders make them
# clearly visible on top of the root-locus branches.
ax.scatter(
    selected_poles.real,
    selected_poles.imag,
    marker="o",
    s=160,
    facecolors="white",
    edgecolors="black",
    linewidths=2.5,
    zorder=10,
    label=f"Selected Kp = {kp}"
)

# Annotate selected poles
for pole in selected_poles:
    ax.annotate(
        f"{pole.real:.1f} {pole.imag:+.1f}j",
        xy=(pole.real, pole.imag),
        xytext=(12, 8),
        textcoords="offset points",
        fontsize=10
    )

# Real and imaginary axes.
# Keep these visually in the background so they cannot
# be mistaken for root-locus branches.
ax.axhline(
    0.0,
    color="gray",
    linewidth=0.9,
    alpha=0.6,
    zorder=0
)

ax.axvline(
    0.0,
    color="gray",
    linewidth=0.9,
    alpha=0.6,
    zorder=0
)

ax.set_title(
    "Root Locus of Simplified Charger Current Loop",
    fontsize=14
)

ax.set_xlabel("Real Axis")
ax.set_ylabel("Imaginary Axis")

ax.set_xlim(-2.1, 0.1)
ax.set_ylim(-3.2, 3.2)

ax.grid(
    True,
    linestyle="--",
    alpha=0.30,
    zorder=0
)

ax.legend(
    loc="upper left",
    frameon=True
)

fig.tight_layout()

fig.savefig(
    PROJECT_ROOT / "analysis" / "root_locus.png",
    dpi=200,
    bbox_inches="tight"
)

plt.show()