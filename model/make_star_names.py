from math import sin, cos, radians

def ra_dec_to_x_y_z(ra, dec):
    sin_dec = sin(radians(dec))
    cos_dec = cos(radians(dec))
    sin_ra = sin(radians(-ra))
    cos_ra = cos(radians(-ra))
    x = cos_dec*-sin_ra
    y = cos_dec*cos_ra
    z = sin_dec
    return x, y, z

def make_star_list():
  with open("../data/star_names") as input_file:
    data = [i.split(",")[:4] for i in input_file]

  data = [(name.strip(),)+ra_dec_to_x_y_z(float(ra), float(dec)) for name, _, ra, dec in data[1:]]
  return data

data = make_star_list()
num_objects = len(data)

objects = ",\n".join(['{"%s", %.7ff, %.7ff, %.7ff}'%(name, x, y, z) for name, x, y, z in data])
objects = """
#include "star_names.h"
const uint16_t num_star_names = %u;
const s_star_names star_names[num_star_names] = {
%s
};"""%(num_objects, objects);

with open("../pico_planetarium/star_names.cpp", 'w') as output_file:
  output_file.write(objects)
