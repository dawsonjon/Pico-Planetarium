import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib.colors as mcolors
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
from scipy.optimize import newton

num_frames = 2000

# Function to solve Kepler's equation
def kepler_equation(E, M, e):
    return E - e * np.sin(E) - M

# Define orbital elements for major planets (simplified, at epoch J2000)
# a = semi-major axis (AU), e = eccentricity, i = inclination (degrees), T = period (days)
planets = {
    "Mercury": {"a": 0.387, "e": 0.205, "i_deg": 7.0, "T": 88,    "M0_deg": 174.796},
    "Venus":   {"a": 0.723, "e": 0.007, "i_deg": 3.4, "T": 225,   "M0_deg": 50.115},
    "Earth":   {"a": 1.000, "e": 0.017, "i_deg": 0.0, "T": 365,   "M0_deg": 357.517},
    "Mars":    {"a": 1.524, "e": 0.093, "i_deg": 1.9, "T": 687,   "M0_deg": 19.373},
    "Jupiter": {"a": 5.204, "e": 0.049, "i_deg": 1.3, "T": 4333,  "M0_deg": 20.020},
    "Saturn":  {"a": 9.582, "e": 0.056, "i_deg": 2.5, "T": 10759, "M0_deg": 317.020},
    "Uranus":  {"a": 19.20, "e": 0.046, "i_deg": 0.8, "T": 30687, "M0_deg": 142.2386},
    "Neptune": {"a": 30.05, "e": 0.010, "i_deg": 1.8, "T": 60190, "M0_deg": 256.228},
}

# Time array for animation (years converted to days)
t_days = np.linspace(0, 60190, num_frames)

# Function to compute (x, y, z) positions in 3D from Keplerian elements
def planet_position(a, e, i_deg, T, M0_deg, t_array):
    n = 2 * np.pi / T
    i_rad = np.radians(i_deg)
    M0 = np.radians(M0_deg)
    x_list, y_list, z_list = [], [], []

    for t in t_array:
        M = M0 + n * t
        M = np.mod(M, 2 * np.pi)
        E = newton(kepler_equation, M, args=(M, e))
        theta = 2 * np.arctan2(np.sqrt(1+e)*np.sin(E/2),
                               np.sqrt(1-e)*np.cos(E/2))
        r = a * (1 - e * np.cos(E))
        x_orb = r * np.cos(theta)
        y_orb = r * np.sin(theta)
        z_orb = 0
        # Rotate for inclination
        x = x_orb
        y = y_orb * np.cos(i_rad)
        z = y_orb * np.sin(i_rad)
        x_list.append(x)
        y_list.append(y)
        z_list.append(z)
    return np.array(x_list), np.array(y_list), np.array(z_list)

# Compute orbits
orbits = {name: planet_position(**params, t_array=t_days) for name, params in planets.items()}

# Set up 3D plot
fig = plt.figure(figsize=(10, 10))
ax = fig.add_subplot(111, projection='3d')
ax.view_init(elev=15, azim=45)
my_dpi=96
fig.set_size_inches(1920/my_dpi, 1080/my_dpi)

ax.w_xaxis.set_pane_color((0.1, 0.1, 0.1, 0.9))
ax.w_yaxis.set_pane_color((0.1, 0.1, 0.1, 0.9))
ax.w_zaxis.set_pane_color((0.1, 0.1, 0.1, 0.9))
ax.xaxis.pane.set_edgecolor((0.5, 0.5, 0.5, 1.0))
ax.yaxis.pane.set_edgecolor((0.5, 0.5, 0.5, 1.0))
ax.zaxis.pane.set_edgecolor((0.5, 0.5, 0.5, 1.0))

# Bonus: To get rid of the grid as well:
ax.grid(False)


lines = {}
points = {}

colors = ['lightgrey', 'orange', 'teal', 'brown', 'moccasin', 'wheat', 'aqua', 'deepskyblue']

# Initialize plot elements
for (name, (x, y, z)), color in zip(orbits.items(), colors):
    line, = ax.plot(x, y, z, label=name, lw=1.0, alpha=0.3, color = color)
    point, = ax.plot([x[0]], [y[0]], [z[0]], 'o', color = color)
    lines[name] = line
    points[name] = point

ax.scatter(0, 0, 0, color='yellow', s=100, label='Sun')
ax.set_xlim(-25, 25)
ax.set_ylim(-25, 25)
ax.set_zlim(-5, 5)
ax.set_xlabel('X (AU)')
ax.set_ylabel('Y (AU)')
ax.set_zlabel('Z (AU)')
ax.set_title('Solar System Planetary Orbits (3D)')
ax.legend(loc='upper left', bbox_to_anchor=(1, 1))

# Animation function
def update(frame):
    for name, (x, y, z) in orbits.items():
        points[name].set_data([x[frame]], [y[frame]])
        points[name].set_3d_properties([z[frame]])
    return points.values()

anim = FuncAnimation(fig, update, frames=num_frames, interval=50)#, blit=False)
anim.save('planets.mp4', writer='ffmpeg', fps=30, dpi=my_dpi)
#plt.show()
