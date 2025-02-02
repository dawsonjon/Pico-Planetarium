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

  clines = []
  for line in data:
    points = [stars[int(number)][:2] for number in line.split()[2:]]
    last_point = points[0]
    for point in points[1:]:
      clines.append(last_point+point) 
      last_point = point

  return clines

stars = read_stars()
clines = read_constellations(stars)

num_clines = len(clines)
clines = ",\n".join(["{%.7ff, %.7ff, %.7ff, %.7ff}"%(i) for i in clines])
clines = """
#include "clines.h"
uint16_t num_clines = %u;
s_cline clines[num_clines] = {
%s
};"""%(num_clines, clines);

with open("pico_planetarium/clines.cpp", 'w') as output_file:
  output_file.write(clines)
