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

struct s_keplarian {
  double a;
  double e;
  double I;
  double L;
  double w;
  double O;
};

struct s_extra_terms {
  double b;
  double c;
  double s;
  double f;
};

extern const s_keplarian elements[];
extern const s_keplarian rates[];
extern const s_extra_terms extra_terms[];

class c_planetarium
{
  s_observer observer;
  float lst; //calculated from utc
  double julian_date; //calculated from utc

  inline float to_radians(float x);
  inline float to_degrees(float x);
  void ra_dec_to_alt_az(float ra, float dec, float &alt, float &az);
  void alt_az_to_ra_dec(float alt, float az, float &ra, float &dec);
  void calculate_view(float alt, float az, float &x, float &y);
  void calculate_pixel_coords(float &x, float &y);
  void plot_constellations();
  void plot_stars();
  void plot_planets();
  void plot_constellation_names();
  float greenwich_sidereal_time();
  void local_sidereal_time();
  uint16_t star_colour(float mk, uint8_t mag);
  uint16_t colour565(uint8_t r, uint8_t g, uint8_t b);

  void set_pixel(uint16_t x, uint16_t y, uint16_t colour);
  void fill_circle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour);
  void draw_circle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour);
  void draw_string(uint16_t x, uint16_t y, const uint8_t *font, const char *s, uint16_t fg);
  void draw_char(uint16_t x, uint16_t y, const uint8_t *font, char c, uint16_t fg);
  void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour);
  double solve_kepler(double M, double e, double E);
  void convert_to_ra_dec(double x, double y, double z, double &ra, double &dec);
  void compute_planet_position(double jd, s_keplarian elements, s_keplarian rates, s_extra_terms extra_terms, double &x, double &y, double &z);

  public:

  void update(s_observer observer);
};

#endif
