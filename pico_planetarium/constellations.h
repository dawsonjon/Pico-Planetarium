#ifndef __CONSTELLATIONS_H__
#define __CONSTELLATIONS_H__

#include <cstdint>

struct s_ra_dec
{
float ra;
float dec;
};

static const uint8_t num_constellations = 88;
extern const char* const constellation_short_names[];
extern const char* const constellation_names[];
extern const s_ra_dec constellation_centres[];

#endif
