from math import sin, cos, radians

def ra_dec_to_x_y_z(ra, dec):
    ra*=15#convert hours to degrees
    sin_dec = sin(radians(dec))
    cos_dec = cos(radians(dec))
    sin_ra = sin(radians(-ra))
    cos_ra = cos(radians(-ra))
    x = cos_dec*-sin_ra
    y = cos_dec*cos_ra
    z = sin_dec
    return x, y, z
  

def make_object_list():
  with open("messier_ngc_processed.csv") as input_file:
    data = [i.split(",")[:4] for i in input_file]

  object_types = set([i[1] for i in data])
  type_enum = dict([(type_.strip().replace(" ", "_"), idx) for idx, type_ in enumerate(object_types)])
  data = [(name, type_.strip().replace(" ", "_"))+ra_dec_to_x_y_z(float(ra), float(dec)) for name, type_, ra, dec in data[1:]]

  return type_enum, data

type_enum, data = make_object_list()
num_objects = len(data)

print(type_enum)

objects = ",\n".join(['{"%s", %s, %.7ff, %.7ff, %.7ff}'%(name, type_, x, y, z) for name, type_, x, y, z in data])
objects = """
#include "objects.h"
const uint16_t num_objects = %u;
const s_object objects[num_objects] = {
%s
};"""%(num_objects, objects);

with open("pico_planetarium/objects.cpp", 'w') as output_file:
  output_file.write(objects)
