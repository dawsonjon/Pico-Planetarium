#ifndef __PLANETARIUM_H__
#define __PLANETARIUM_H__

struct s_observer
{
  float field; //field of view in degrees 
  float alt; //altidute in degrees
  float az; //azimuth in degrees
  float smallest_magnitude; //smallest magnitude star to plot
  float lst; //local sidereal time in degrees
  float latitude; //latitude in degrees
  float longitude; //longitude in degrees
};

inline float to_radians(float x);
inline float to_degrees(float x);
void ra_dec_to_alt_az(float ra, float dec, float lst, float latitude, float &alt, float &az);
void calculate_view(float alt, float az, float view_alt, float view_az, float &x, float &y);
void calculate_pixel_coords(float &x, float &y, uint16_t width, uint16_t height, float field);
uint16_t plot_stars(uint16_t star_x[], uint16_t star_y[], int8_t mag[], uint8_t col[], bool dirty[], uint16_t max_stars_to_plot, uint16_t screen_width, uint16_t screen_height, const s_observer &view);
uint16_t plot_constellations(uint16_t cline_x1[], uint16_t cline_y1[], uint16_t cline_x2[], uint16_t cline_y2[], bool dirty[], uint16_t max_clines_to_plot, uint16_t screen_width, 
  uint16_t screen_height, const s_observer &observer);
void plot_constellation_names(uint16_t x[], uint16_t y[], bool visible[], bool dirty[], s_observer observer, uint16_t screen_width, uint16_t screen_height);
float local_sidereal_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, float longitude);
void star_colour(float mk, uint8_t &r, uint8_t &g, uint8_t &b);

#endif
