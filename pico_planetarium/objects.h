#ifndef __OBJECTS_H__
#define __OBJECTS_H__

#include <cstdint>

enum e_object_type {
open_cluster,
star_cloud,
emission_nebula,
globular_cluster,
emission_nebula_open_cluster,
spiral_galaxy
};

struct s_object
{
const char *name;
e_object_type object_type;
float x;
float y;
float z;
};

extern const uint16_t num_objects;
extern const s_object objects[];

#endif
