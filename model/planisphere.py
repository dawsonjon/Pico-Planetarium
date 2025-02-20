from matplotlib import pyplot as plt
import numpy as np

maglimit = 5

def read_stars():
  with open("catalog") as input_file:
    data = input_file.read()

  data = data.splitlines()
  stars = {}
  for line in data:
    number = line[:4]
    name = line[4:14]
    constellation = name[-3:]
    ra_hr = line[75:77]
    ra_min = line[77:79]
    ra_sec = line[79:83]
    dec_deg = line[83:86]
    dec_min = line[86:88]
    dec_sec = line[88:90]

    try:
      number = int(number)
      ra = int(ra_hr)*15 + int(ra_min)*15/60 + float(ra_sec)*15/3600
      dec = int(dec_deg) + int(dec_min)/60 + float(dec_sec)/3600
      magnitude = float(line[102:107])
    except ValueError:
      continue

    stars[number] = (ra, dec, constellation, magnitude)

  return stars

def fix_ra(points):
  #handle case where lines wrap from 360 to 0
  min_ra = min([i[0] for i in points])
  max_ra = max([i[0] for i in points])
  if max_ra - min_ra > 180:
    left_points = [(ra, dec) if ra < 180 else (ra - 360, dec) for ra, dec in points] 
    right_points = [(ra, dec) if ra > 180 else (ra + 360, dec) for ra, dec in points] 
    return [left_points, right_points]
  else:
    return [points]

def read_constellations(stars):

  with open("constellations") as input_file:
    data = input_file.read()

  data = data.splitlines()
  constellations = []
  for line in data:
    points = line.split()[2:]
    points = [stars[int(number)][:2] for number in points]
    constellations.extend(fix_ra(points))
  return constellations

stars = read_stars()
constellations = read_constellations(stars)

#plot northern hemisphere
ax = plt.subplot(121, projection = 'polar')
for points in constellations:
  ax.plot([ra*np.pi/180 for ra, dec in points if dec > 0], [dec for ra, dec in points if dec > 0], "b-")
for star in stars.values():
  ra, dec, constellation, magnitude = star
  if magnitude > maglimit:
    continue
  if dec > 0:
    ax.plot(ra*np.pi/180, dec, ".w", markersize=1+maglimit-magnitude)
ax.set_rmax(90)
ax.set_rlim(bottom=90, top=0)
ax.set_rticks([0, 30, 60, 90])
ax.set_theta_direction(-1)
ax.set_theta_offset(np.pi/2.0)
ax.set_facecolor("#261945")
ax.figure.set_facecolor("black")
[i.set_color("orange") for i in ax.get_xticklabels()]
[i.set_color("orange") for i in ax.get_yticklabels()]
ax.grid(which='major', axis='both', linestyle='-', color="darkblue")
ax.set_title("Northern Hemisphere", color="orange")

#plot southern hemisphere
ax = plt.subplot(122, projection = 'polar')
for points in constellations:
  ax.plot([ra*np.pi/180 for ra, dec in points if dec < 0], [dec for ra, dec in points if dec < 0], "b-")
for star in stars.values():
  ra, dec, constellation, magnitude = star
  if magnitude > maglimit:
    continue
  if dec < 0:
    ax.plot(ra*np.pi/180, dec, ".w", markersize=1+maglimit-magnitude)
ax.set_rmax(90)
ax.set_rlim(bottom=-90, top=0)
ax.set_rticks([0, -30, -60, -90])
ax.set_theta_direction(1)
ax.set_theta_offset(np.pi/2.0)
ax.set_facecolor("#261945")
[i.set_color("orange") for i in ax.get_xticklabels()]
[i.set_color("orange") for i in ax.get_yticklabels()]
ax.grid(which='major', axis='both', linestyle='-', color="darkblue")
ax.set_title("Southern Hemisphere", color="orange")

fig, ax = plt.subplots(nrows=1, ncols=1)
#plot equatorial
for points in constellations:
  ax.plot([ra for ra, dec in points], [dec for ra, dec in points], "b-")
for star in stars.values():
  ra, dec, constellation, magnitude = star
  if magnitude > maglimit:
    continue
  plt.plot(ra, dec, ".w", markersize=(1+maglimit-magnitude)/2)
  if ra > 310:
    plt.plot(ra-360, dec, ".w", markersize=(1+maglimit-magnitude)/2)
  if ra < 50:
    plt.plot(ra+360, dec, ".w", markersize=(1+maglimit-magnitude)/2)

ax.set_facecolor("#261945")
fig.set_facecolor("black")
[i.set_color("orange") for i in ax.get_xticklabels()]
[i.set_color("orange") for i in ax.get_yticklabels()]
ax.grid(which='major', axis='both', linestyle='-', color="darkblue")
ax.set_title("Equatorial", color="orange")
ax.set_ylim([-60, 60])
ax.set_xlim([-45, 405])
plt.gca().invert_xaxis()
plt.show()



