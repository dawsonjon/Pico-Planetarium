#ifndef __STARS_H__
#define __STARS_H__

#include <cstdint>

struct s_star {
  float ra;
  float dec;
  float mag;
  uint8_t mk;
};

extern const s_star stars[];
extern const uint16_t num_stars;

#endif

