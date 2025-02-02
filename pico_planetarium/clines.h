#ifndef __CLINES_H__
#define __CLINES_H__

#include <cstdint>

struct s_cline {
  float ra1;
  float dec1;
  float ra2;
  float dec2;
};

extern s_cline clines[];
extern const uint16_t num_clines;

#endif

