from matplotlib import pyplot as plt
import numpy as np
import math
from math import tan, sin, cos, asin, atan2, radians, degrees
from datetime import datetime, timezone

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
    mk = line[129:131]

    try:
      number = int(number)
      ra = hhmmss_to_deg(ra_hr, ra_min, ra_sec)
      dec = ddmmss_to_deg(dec_deg, dec_min, dec_sec)
      magnitude = float(line[102:107])
    except ValueError:
      continue

    sin_dec = sin(radians(dec))
    cos_dec = cos(radians(dec))
    sin_ra = sin(radians(-ra))
    cos_ra = cos(radians(-ra))
    x = cos_dec*-sin_ra
    y = cos_dec*cos_ra
    z = sin_dec

    stars[number] = (x, y, z, constellation, magnitude, mk)

  return stars

scale = {
"O" : 0,
"B" : 10,
"A" : 20,
"F" : 30,
"G" : 40,
"K" : 50,
"M" : 60
}
def scale_colour(mk):
  """scale mk spectral class to fit in one byte"""
  x = scale.get(mk[0], 30)#by default assume a medium colour
  if len(mk) > 1:
    try:
      x += int(mk[1])
    except ValueError:
      pass
  return x; 
  

stars = read_stars().values()
num_stars = len(stars)
stars = ",\n".join(["{%.7ff, %.7ff, %.7ff, %.7ff, %u}"%(x, y, z, magnitude, scale_colour(col)) for x, y, z, _, magnitude, col in stars])
stars = """
#include "stars.h"
const uint16_t num_stars = %u;
s_star stars[num_stars] = {
%s
};"""%(num_stars, stars);

with open("pico_planetarium/stars.cpp", 'w') as output_file:
  output_file.write(stars)
