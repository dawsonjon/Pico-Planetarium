from matplotlib import pyplot as plt
import numpy as np
import math
from math import tan, sin, cos, asin, atan2, radians, degrees
from datetime import datetime, timezone

maglimit = 5

def hhmmss_to_deg(hh, mm, ss):
  """convert hours, minutes and seconds to degrees"""
  return int(hh)*15 + int(mm)*15/60 + float(ss)*15/3600

def deg_to_hhmmss(deg):
  hh = deg // 15
  deg  -= hh*15
  mm = deg // (15/60)
  deg -= mm*(15/60)
  ss = deg / (15/3600)
  return hh, mm, ss
  
def ddmmss_to_deg(dd, mm, ss):
  """convert degrees, minutes and seconds to degrees"""
  return int(dd) + int(mm)/60 + float(ss)/3600

def read_stars():
  """read starts from Bright Star Catalog (BST)"""

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
      ra = hhmmss_to_deg(ra_hr, ra_min, ra_sec)
      dec = ddmmss_to_deg(dec_deg, dec_min, dec_sec)
      magnitude = float(line[102:107])
    except ValueError:
      continue

    stars[number] = (ra, dec, constellation, magnitude)

  return stars

def read_constellations(stars):
  with open("constellations") as input_file:
    data = input_file.read()
  data = data.splitlines()
  constellations = []
  for line in data:
    points = line.split()[2:]
    points = [stars[int(number)][:2] for number in points]
    #constellations.extend(fix_ra(points))
    constellations.append(points)
  return constellations

def ra_dec_to_alt_az(ra, dec, lst, latitude):
    H=(lst - ra);
    if(H<0):
      H+=360
    if(H>180):
      H=H-360
    az = degrees(atan2(sin(radians(H)), cos(radians(H))*sin(radians(latitude)) - tan(radians(dec))*cos(radians(latitude))))
    alt = degrees(asin(sin(radians(latitude))*sin(radians(dec)) + cos(radians(latitude))*cos(radians(dec))*cos(radians(H))))
    az-=180
    if(az<0):
      az+=360
    return alt, az

def rotate_stars(stars, hour_angle, latitude):
  return [ra_dec_to_alt_az(ra, dec, hour_angle, latitude) + (constellation, magnitude) for ra, dec, constellation, magnitude in stars]
  
def rotate_constellations(constellations, hour_angle, latitude):
  return [[ra_dec_to_alt_az(ra, dec, hour_angle, latitude) for ra, dec in points] for points in constellations]

def greenwich_sidereal_time(utc_datetime):
    """
    Calculate Greenwich Mean Sidereal Time (GMST) given a UTC datetime.
    """
    # Convert to Julian date
    Y, M, D = utc_datetime.year, utc_datetime.month, utc_datetime.day
    UT = utc_datetime.hour + utc_datetime.minute / 60 + utc_datetime.second / 3600
    
    if M <= 2:
        Y -= 1
        M += 12
    
    A = math.floor(Y / 100)
    B = 2 - A + math.floor(A / 4)
    JD = math.floor(365.25 * (Y + 4716)) + math.floor(30.6001 * (M + 1)) + D + B - 1524.5 + UT / 24
    
    # Julian centuries from J2000.0
    T = (JD - 2451545.0) / 36525.0
    
    # Greenwich Mean Sidereal Time in degrees
    GMST = 280.46061837 + 360.98564736629 * (JD - 2451545) + 0.000387933 * T**2 - (T**3) / 38710000
    
    return GMST % 360  # Ensure within [0, 360] degrees

def local_sidereal_time(utc, longitude):
    """
    Calculate Local Sidereal Time (LST) for a given longitude.
    """
    gmst = greenwich_sidereal_time(utc)
    
    # Convert longitude to degrees (-180 to 180) to match convention
    lst = (gmst + longitude) % 360
    
    return lst 

def plot_overhead(stars, constellations, lst, lat, lon, utc):
  #plot overhead hemisphere
  ax = plt.subplot(111, projection = 'polar')
  for points in constellations:
    ax.plot([az*np.pi/180 for alt, az in points if alt > 0], [alt for alt, az in points if alt > 0], "b-")
  for star in stars:
    alt, az, constellation, magnitude = star
    if magnitude > maglimit:
      continue
    if alt > 0:
      ax.plot(az*np.pi/180, alt, ".w", markersize=1+maglimit-magnitude)
  ax.set_rmax(90)
  ax.set_rlim(bottom=90, top=0)
  ax.set_rticks([0, 30, 60, 90])
  ax.set_theta_offset(np.pi/2.0)
  ax.set_facecolor("#261945")
  ax.figure.set_facecolor("black")
  [i.set_color("orange") for i in ax.get_xticklabels()]
  [i.set_color("orange") for i in ax.get_yticklabels()]
  ax.grid(which='major', axis='both', linestyle='-', color="darkblue")
  ax.set_title("View Overhead", color="orange")

def project(alt, az, view_alt, view_az):

  #make zenith 0 on the x and y axis 
  x = (90-alt) * sin(radians(az-view_az))
  y = (90-alt) * -cos(radians(az-view_az)) + (90-view_alt)

  return x, y
  

def plot_direction(stars, constellations,view_alt, view_az, field):

  az_min = view_az-field/2
  az_max = view_az+field/2
  alt_min = view_alt-field/2
  alt_max = view_alt+field/2

  #plot equatorial
  fig, ax = plt.subplots(nrows=1, ncols=1)
  for points in constellations:
    #duplicate points to cover negative azimuth
    #points = points + [(alt, az-360) for alt, az in points]
    #duplicate points to give altiude greater than 90
    #points = points + [(90+(90-alt), az+180 if az < 0 else az-180) for alt, az in points]
    #check in view
    #points = [(alt, az) for alt, az in points if (alt < alt_max and alt > alt_min and az > az_min and az < az_max)]
    #plot points
    points = [project(alt, az, view_alt, view_az) for alt, az in points]
    ax.plot([x for x, y in points], [y for x, y in points], "b-")
    pass

  #remove dim stars
  stars = [(alt, az, mag) for alt, az, _, mag in stars if mag<maglimit]
  #duplicate stars for negative azimuth
  #stars = stars + [(alt, az-360, mag) for alt, az, mag in stars]
  #duplicate stars for altitude greater than 90 
  #stars = stars + [(90+(90-alt), az+180 if az < 0 else az-180, mag) for alt, az, mag in stars]
  #remove everything that isn't in view
  #stars = [(alt, az, mag) for alt, az, mag in stars if (alt < alt_max and alt > alt_min and az > az_min and az < az_max)]
  for alt, az, magnitude in stars:
    x, y = project(alt, az, view_alt, view_az)
    plt.plot(x, y, ".w", markersize=(1+maglimit-magnitude)/2)

  ax.set_facecolor("#261945")
  fig.set_facecolor("black")
  [i.set_color("orange") for i in ax.get_xticklabels()]
  [i.set_color("orange") for i in ax.get_yticklabels()]
  ax.grid(which='major', axis='both', linestyle='-', color="darkblue")
  ax.set_title("Equatorial", color="orange")
  ax.set_xlim([-field/2, field/2])
  ax.set_ylim([-field/2, field/2])

latitude = 51.0
longitude = -2.2649
fixed_stars = read_stars()
fixed_constellations = read_constellations(fixed_stars)
utc = datetime.now(timezone.utc)
lst=local_sidereal_time(utc, longitude)

stars = rotate_stars(fixed_stars.values(), lst, latitude)
constellations = rotate_constellations(fixed_constellations, lst, latitude)
plot_overhead(stars, constellations, lst, latitude, longitude, utc)
plot_direction(stars, constellations, 45, 0, 90)
plot_direction(stars, constellations, 45, 90, 90)
plot_direction(stars, constellations, 45, 180, 90)
plot_direction(stars, constellations, 45, 270, 90)
plt.show()
