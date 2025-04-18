#ifndef __STAR_NAMES_H__
#define __STAR_NAMES_H__

#include <cstdint>

struct s_star_names
{
const char *name;
float x;
float y;
float z;
};

extern const uint16_t num_star_names;
extern const s_star_names star_names[];

#endif
