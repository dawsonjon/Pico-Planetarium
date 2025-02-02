#include <cmath>
#include <cstdio>
#include "planetarium.h"
#include "stars.h"
#include "constellations.h"
#include "clines.h"
#include "font_8x5.h"

uint16_t buffer[height][width] = {};

void c_planetarium :: set_pixel(uint16_t x, uint16_t y, uint16_t colour)
{
  if(x>=width) return;
  if(y>=height) return;
  buffer[y][x] = colour;
}

void c_planetarium::draw_string(uint16_t x, uint16_t y, const uint8_t *font, const char *s, uint16_t fg) 
{
  const uint8_t font_width  = font[1];
  const uint8_t font_space  = font[2];
  for(int32_t x_n=x; *s; x_n+=(font_width+font_space)) {
      draw_char(x_n, y, font, *(s++), fg);
  }
}

void c_planetarium::draw_char(uint16_t x, uint16_t y, const uint8_t *font, char c, uint16_t fg) 
{

  const uint8_t font_height = font[0];
  const uint8_t font_width  = font[1];
  const uint8_t font_space  = font[2];
  const uint8_t first_char  = font[3];
  const uint8_t last_char   = font[4];
  const uint16_t bytes_per_char = font_width*font_height/8;

  if(c<first_char||c>last_char) return;

  uint16_t font_index = ((c-first_char)*bytes_per_char) + 5u;
  uint8_t data = font[font_index++];
  uint8_t bits_left = 8;

  for(uint8_t xx = 0; xx<font_width; ++xx)
  {
    for(uint8_t yy = 0; yy<font_height; ++yy)
    {
      if(data & 0x01){
        set_pixel(x+xx, y+yy, fg);
      }
      data >>= 1;
      bits_left--;
      if(bits_left == 0)
      {
        data = font[font_index++];
        bits_left = 8;
      }
    }
  }
}

uint16_t c_planetarium::colour565(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t val = (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f);
    return (val >> 8) | (val << 8);
}

void c_planetarium :: update(s_observer o)
{
  observer = o;
  local_sidereal_time();

  fill_rect(0, 0, width, height, colour565(5, 0, 10));

  plot_constellations();
  plot_stars();
  plot_constellation_names();

  fill_rect(0, height-10, width, 10, colour565(0, 0, 0));
  char buffer[100];
  snprintf(buffer, 100, "lat:%0.1f lon:%0.1f %04u-%02u-%02u %02u:%02u:%02u", 
    observer.latitude,
    observer.longitude,
    observer.year,
    observer.month,
    observer.day,
    observer.hour,
    observer.min,
    observer.sec);
  draw_string(0, height-8, font_8x5, buffer, colour565(255, 255, 255));
}

void c_planetarium :: fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour)
{
  //fill buffer with background colour
  for(uint16_t xx = 0; xx < w; xx++)
  {
    for(uint16_t yy = 0; yy < h; yy++)
    {
      set_pixel(x+xx, y+yy, colour);
    }
  }
}

inline float c_planetarium :: to_radians(float x)
{
  return x * (float)M_PI/180.0f;
}

inline float c_planetarium :: to_degrees(float x)
{
  return x * 180.0f/(float)M_PI;
}

void c_planetarium :: ra_dec_to_alt_az(float ra, float dec, float &alt, float &az)
{
    float H=(lst - ra);

    if(H<0.0f)
    {
      H+=360.0f;
    }

    if(H>180.0f)
    {
      H=H-360.0f;
    }

    az = to_degrees(
      atan2(sin(to_radians(H)), 
      cos(to_radians(H))*sin(to_radians(observer.latitude)) - tan(to_radians(dec))*cos(to_radians(observer.latitude))));
    alt = to_degrees(asin(sin(to_radians(observer.latitude))*sin(to_radians(dec)) + cos(to_radians(observer.latitude))*cos(to_radians(dec))*cos(to_radians(H))));
    az-=180.0f;

    if(az<0)
    {
      az+=360.0f;
    }
}

void c_planetarium :: calculate_view(float alt, float az, float &x, float &y)
{
  //make zenith 0 on the x and y axis 
  x = (90.0f-alt) * sin(to_radians(az-observer.az));
  y = (90.0f-alt) * -cos(to_radians(az-observer.az)) + (90.0f-observer.alt);

}

void c_planetarium :: calculate_pixel_coords(float &x, float &y)
{
  x = round(width * (x/observer.field+0.5f));
  y = round(height * (1.0f-(y/observer.field + 0.5f)));
}

void c_planetarium :: plot_constellations()
{
  for(uint16_t idx=0; idx < num_clines; ++idx)
  {

    float alt1, az1;
    ra_dec_to_alt_az(clines[idx].ra1, clines[idx].dec1, alt1, az1);
    
    float alt2, az2;
    ra_dec_to_alt_az(clines[idx].ra2, clines[idx].dec2, alt2, az2);

    float x1, y1;
    calculate_view(alt1, az1, x1, y1);

    float x2, y2;
    calculate_view(alt2, az2, x2, y2);

    //don't bother plotting stars outside field of observer
    bool draw = 
    ((x1 < observer.field/2) && (x1 > -observer.field/2) && (y1 < observer.field/2) && (y1 > -observer.field/2)) ||
    ((x2 < observer.field/2) && (x2 > -observer.field/2) && (y2 < observer.field/2) && (y2 > -observer.field/2));
    if(!draw) continue;

    calculate_pixel_coords(x1, y1);
    calculate_pixel_coords(x2, y2);
    
    //draw line between 2 points
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
  
    while (1) {
        set_pixel(x1, y1, colour565(0, 0, 64));
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }

  }
}

void c_planetarium :: plot_stars()
{
  uint16_t num_plotted = 0;
  for(uint16_t idx=0; idx < num_stars; ++idx)
  {

    if(stars[idx].mag > observer.smallest_magnitude) continue;

    float alt, az;
    ra_dec_to_alt_az(stars[idx].ra, stars[idx].dec, alt, az);

    float x, y;
    calculate_view(alt, az, x, y);

    //don't bother plotting stars outside field of observer
    if(x > observer.field/2) continue;
    if(x < -observer.field/2) continue;
    if(y > observer.field/2) continue;
    if(y < -observer.field/2) continue;

    //get coordinated and magnitude of x
    calculate_pixel_coords(x, y);
    int8_t mag = stars[idx].mag;
    uint8_t mk = stars[idx].mk;

    if(mag <= 1)
    {
      uint16_t colour = star_colour(mk, 1);
      set_pixel(x, y, colour);
      set_pixel(x+1, y, colour);
      set_pixel(x, y+1, colour);
      set_pixel(x, y-1, colour);
      set_pixel(x-1, y, colour);
      set_pixel(x+1, y-1, colour);
      set_pixel(x-1, y+1, colour);
      set_pixel(x+1, y+1, colour);
      set_pixel(x-1, y-1, colour);
      set_pixel(x+2, y, colour);
      set_pixel(x, y+2, colour);
      set_pixel(x, y-2, colour);
      set_pixel(x-2, y, colour);
    }
    else if(mag <= 2)
    {
      uint16_t colour = star_colour(mk, 1);
      set_pixel(x, y, colour);
      set_pixel(x+1, y, colour);
      set_pixel(x, y+1, colour);
      set_pixel(x, y-1, colour);
      set_pixel(x-1, y, colour);
    }
    else
    {
      uint16_t colour = star_colour(mk, mag-2);
      set_pixel(x, y, colour);
    } 

  }
}

void c_planetarium :: plot_constellation_names()
{
  for(uint16_t idx=0; idx < num_constellations; ++idx)
  {

    float alt, az;
    ra_dec_to_alt_az(constellation_centres[idx].ra*15, constellation_centres[idx].dec, alt, az);

    float x, y;
    calculate_view(alt, az, x, y);

    if((x < observer.field/2) && (x > -observer.field/2) && (y < observer.field/2) && (y > -observer.field/2))
    {
      calculate_pixel_coords(x, y);
      draw_string(x, y, font_8x5, constellation_names[idx], colour565(255, 255, 255));
    }
    
  }
}

float c_planetarium :: greenwich_sidereal_time()
{
    //Calculate Greenwich Mean Sidereal Time (GMST) given a UTC datetime.

    // Convert to Julian date
    double ut = observer.hour + observer.min / 60.0f + observer.sec / 3600.0f;
    
    if(observer.month <= 2)
    {
        observer.year -= 1;
        observer.month += 12;
    }
    
    double a = floor(observer.year / 100.0);
    double b = 2.0 - a + floor(a / 4.0);
    double julian_date = floor(365.25 * (observer.year + 4716.0)) + floor(30.6001 * (observer.month + 1.0)) + observer.day + b - 1524.5 + ut / 24.0;
    
    // Julian centuries from J2000.0
    double centuries = (julian_date - 2451545.0) / 36525.0;
    
    // Greenwich Mean Sidereal Time in degrees
    double gmst = 280.46061837 + 360.98564736629 * (julian_date - 2451545.0) + 0.000387933 * pow(centuries,2.0) - pow(centuries, 3.0) / 38710000.0;
    
    return fmod(gmst, 360.0);  // Ensure within [0, 360] degrees
}

void c_planetarium :: local_sidereal_time()
{
    //Calculate Local Sidereal Time (LST) for a given longitude.
    float gmst = greenwich_sidereal_time();
    
    //Convert longitude to degrees (-180 to 180) to match convention
    lst = fmod(gmst + observer.longitude, 360.0f);
    
}

uint16_t c_planetarium :: star_colour(float mk, uint8_t mag)
{
  uint8_t r, g, b;

  //O -> A (0-29) blue->white
  if (mk < 29)
  {
    b = 255;
    r = 255 * mk/30;
    g = 255 * mk/30;
  }
  //F -> G (30-49) white->yellow
  if (mk < 49)
  {
    b = 255 - (255*(mk-30)/20);
    r = 255;
    g = 255;
  }
  //K -> M (50-59) yellow->red
  else
  {
    b = 0;
    r = 255;
    g = 255 - (255*(mk-50)/20);
  }

  return colour565(r>>mag, g>>mag, b>>mag);
}
