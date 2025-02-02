#ifndef __PLANETARIUM_H__
#define __PLANETARIUM_H__

#include <cstdint>

static const uint16_t width = 320;
static const uint16_t height = 240;
extern uint16_t buffer[height][width];

struct s_observer
{
  //view
  float field; //field of view in degrees 
  float alt; //altidute in degrees
  float az; //azimuth in degrees
  float smallest_magnitude; //smallest magnitude star to plot

  //location
  float latitude; //latitude in degrees
  float longitude; //longitude in degrees

  //utc time
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
};

class c_planetarium
{
  s_observer observer;
  float lst; //calculated from utc

  inline float to_radians(float x);
  inline float to_degrees(float x);
  void ra_dec_to_alt_az(float ra, float dec, float &alt, float &az);
  void calculate_view(float alt, float az, float &x, float &y);
  void calculate_pixel_coords(float &x, float &y);
  void plot_constellations();
  void plot_stars();
  void plot_constellation_names();
  float greenwich_sidereal_time();
  void local_sidereal_time();
  uint16_t star_colour(float mk, uint8_t mag);
  uint16_t colour565(uint8_t r, uint8_t g, uint8_t b);

  void set_pixel(uint16_t x, uint16_t y, uint16_t colour);
  void draw_string(uint16_t x, uint16_t y, const uint8_t *font, const char *s, uint16_t fg);
  void draw_char(uint16_t x, uint16_t y, const uint8_t *font, char c, uint16_t fg);
  void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour);

  public:

  void update(s_observer observer);
};

#endif
