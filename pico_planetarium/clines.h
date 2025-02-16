#ifndef __CLINES_H__
#define __CLINES_H__

#include <cstdint>

struct s_cline {
  float x1;
  float y1;
  float z1;
  float x2;
  float y2;
  float z2;
};

extern const s_cline clines[];
extern const uint16_t num_clines;

#endif

