#ifndef __PLANETARIUM_H__
#define __PLANETARIUM_H__

#include <cstdint>
#include "frame_buffer.h"

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
  float sin_lat, cos_lat;
  float view_scale;
  float cos_theta, sin_theta;

  inline float to_radians(float x);
  inline float to_degrees(float x);
  void ra_dec_to_alt_az(float ra, float dec, float &alt, float &az);
  void alt_az_to_ra_dec(float alt, float az, float &ra, float &dec);
  void calculate_view_ra_dec(float ra, float dec, float &x, float &y, float &z);
  void calculate_view(float alt, float az, float &x, float &y, float &z);
  void calculate_pixel_coords(float &x, float &y);
  void plot_constellations();
  void plot_planes();
  void plot_stars();
  void plot_planets();
  void plot_moon();
  void plot_constellation_names();
  void plot_cardinal_points();
  void plot_plane(float pole_alt, float pole_az, uint16_t colour);
  void plot_alt_az_grid(uint16_t colour);
  void plot_ra_dec_grid(uint16_t colour);
  void plot_milky_way();
  float greenwich_sidereal_time();
  void local_sidereal_time();
  uint16_t star_colour(float mk, uint8_t mag);
  float sind(float r);
  float cosd(float r);

  double solve_kepler(double M, double e, double E);
  void convert_to_ra_dec(double x, double y, double z, double &ra, double &dec);
  void compute_planet_position(double jd, s_keplarian elements, s_keplarian rates, s_extra_terms extra_terms, double &x, double &y, double &z);

  c_frame_buffer &frame_buffer;
  uint16_t width, height;

  public:

  c_planetarium(c_frame_buffer & frame_buffer, uint16_t width, uint16_t height):frame_buffer(frame_buffer), width(width), height(height){} 

  void update(s_observer observer);
};

#endif
