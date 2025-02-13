#include <cmath>
#include <cstdio>
#include "planetarium.h"
#include "stars.h"
#include "constellations.h"
#include "clines.h"
#include "skyline.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "images.h"
#include "Arduino.h"

#include "pico/stdlib.h"

uint16_t buffer[height][width] = {};
const char * const planet_names[]={"Mercury","Venus","Earth","Mars","Jupiter","Saturn","Uranus","Neptune","Pluto","Sun"};



void c_planetarium :: set_pixel(uint16_t x, uint16_t y, uint16_t colour, uint16_t alpha)
{
  if(x>=width) return;
  if(y>=height) return;
  uint16_t old_colour = buffer[y][x];
  buffer[y][x] = alpha_blend(old_colour, colour, alpha);
}

void c_planetarium :: draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t colour, uint16_t alpha)
{
    bool one_point_in_view = 
      (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height) || 
      (x2 >= 0 && x2 < width && y2 >= 0 && y2 < height);
    if(!one_point_in_view) return;

    //draw line between 2 points
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
  
    while (1) {
        set_pixel(x1, y1, colour, alpha);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void c_planetarium::draw_string(uint16_t x, uint16_t y, const uint8_t *font, const char *s, uint16_t fg, uint16_t alpha) 
{
  const uint8_t font_width  = font[1];
  const uint8_t font_space  = font[2];
  for(int32_t x_n=x; *s; x_n+=(font_width+font_space)) {
      draw_char(x_n, y, font, *(s++), fg, alpha);
  }
}

void c_planetarium::draw_char(uint16_t x, uint16_t y, const uint8_t *font, char c, uint16_t fg, uint16_t alpha) 
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
        set_pixel(x+xx, y+yy, fg, alpha);
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

void c_planetarium::fill_circle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour, uint16_t alpha)
{
  for (int16_t y = -radius; y <= radius; y++)
  {
    for (int16_t x = -radius; x <= radius; x++)
    {
      if (x * x + y * y <= radius * radius)
      {
          set_pixel(xc + x, yc + y, colour, alpha);
      }
    }
  }
}

void c_planetarium :: draw_circle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour, uint16_t alpha) 
{
    int16_t x = 0, y = radius;
    int16_t d = 1 - radius;
    
    while (x <= y) {
        set_pixel(xc + x, yc + y, colour, alpha);
        set_pixel(xc - x, yc + y, colour, alpha);
        set_pixel(xc + x, yc - y, colour, alpha);
        set_pixel(xc - x, yc - y, colour, alpha);
        set_pixel(xc + y, yc + x, colour, alpha);
        set_pixel(xc - y, yc + x, colour, alpha);
        set_pixel(xc + y, yc - x, colour, alpha);
        set_pixel(xc - y, yc - x, colour, alpha);
        
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
    }
}

uint16_t c_planetarium::colour565(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t val = (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f);
    return (val >> 8) | (val << 8);
}

void c_planetarium::colour_rgb(uint16_t colour_565, uint8_t &r, uint8_t &g, uint8_t &b)
{
    colour_565 = (colour_565 >> 8) | (colour_565 << 8);
    r = ((colour_565 >> 11) & 0x1f) << 3;
    g = ((colour_565 >> 5) & 0x3f) << 2;
    b = (colour_565 & 0x1f) << 3;
}

uint16_t c_planetarium::colour_scale(uint16_t colour, uint16_t alpha)
{
    uint8_t r, g, b;
    colour_rgb(colour, r, g, b);
    r=std::min((r*alpha)>>8, 255);
    g=std::min((g*alpha)>>8, 255);
    b=std::min((b*alpha)>>8, 255);
    return colour565(r, g, b);
}

uint16_t c_planetarium::alpha_blend(uint16_t old_colour, uint16_t colour, uint16_t alpha)
{
    if(alpha == 256) return colour;

    uint8_t old_r, old_g, old_b;
    colour_rgb(old_colour, old_r, old_g, old_b);
    old_r=((uint16_t)old_r*(256-alpha))>>8;
    old_g=((uint16_t)old_g*(256-alpha))>>8;
    old_b=((uint16_t)old_b*(256-alpha))>>8;
    uint8_t r, g, b;
    colour_rgb(colour, r, g, b);
    uint16_t blended_r = r; uint16_t blended_g = g; uint16_t blended_b = b;
    blended_r *= alpha; blended_g *= alpha; blended_b *= alpha;
    blended_r >>= 8; blended_g >>= 8; blended_b >>= 8;
    blended_r += old_r; blended_g += old_g; blended_b += old_b;
    blended_r = std::min(blended_r, (uint16_t)255);
    blended_g = std::min(blended_g, (uint16_t)255);
    blended_b = std::min(blended_b, (uint16_t)255);
    return colour565(blended_r, blended_g, blended_b);
}


void c_planetarium :: update(s_observer o)
{
  observer = o;
  local_sidereal_time();
  fill_rect(0, 0, width, height, colour565(5, 0, 50));

  //calculate constants related to observer view
  sin_lat = sin(to_radians(observer.latitude));
  cos_lat = cos(to_radians(observer.latitude));
  view_scale = 1.0f/(2.0f*sin(to_radians(observer.field/2.0f)));
  const float theta = -(90-observer.alt);
  sin_theta = sin(to_radians(theta));
  cos_theta = cos(to_radians(theta));

  plot_milky_way(); //66ms
  plot_alt_az_grid(colour565(255, 255, 255)); //107ms
  plot_ra_dec_grid(colour565(255, 0, 128)); //243ms
  plot_planes(); //38ms
  plot_constellations(); //133ms
  plot_stars(); //452ms
  plot_planets(); //3ms
  plot_moon();
  plot_constellation_names(); //9ms

  //obscure the area bellow the horizon
  uint16_t view_major_radius = height/(2*sin(to_radians(observer.field/2)));
  uint16_t view_minor_radius = view_major_radius * sin(to_radians(observer.alt));
  const int a = view_major_radius;
  const int b = view_minor_radius;


  //draw skyline
  int max_y = observer.alt>89.0f?height/2:0;
  for (int y = -height/2; y <= max_y; y++) {
      for (int x = -width/2; x <= width/2; x++) {
          if (((float)x * x) * ((float)b * b) + ((float)y * y) * ((float)a * a) > ((float)a * a) * ((float)b * b)) {
              set_pixel(width/2 + x, height/2 - y, colour565(0, 0, 0), 200);
          }
      }
  }

  int min_y = observer.alt>89.0f?-height/2:0;
  for (int y = min_y; y <= height/2; y++) {
    for (int x = -width/2; x < width/2; x++) {
      uint16_t from_x = 240.0f + (x*240.0f/view_major_radius);
      uint16_t from_y = 240.0f + (y*240.0f/view_minor_radius);
      if(from_x < 480 && from_y < 480 && skyline[from_x * 480 + from_y] != 0xffff)
      {
        set_pixel(width/2 + x, height/2 + y, colour565(0, 0, 0), 200);
      }
    }
  }

  plot_cardinal_points();

  char buffer[100];
  snprintf(buffer, 100, "%0.1f\x7f,%0.1f\x7f", observer.latitude, observer.longitude);
  draw_string(0, height-8, font_8x5, buffer, colour565(255, 255, 255));
  snprintf(buffer, 100, "%04u-%02u-%02u %02u:%02u:%02u",
    observer.year,
    (uint16_t)observer.month,
    (uint16_t)observer.day,
    (uint16_t)observer.hour,
    (uint16_t)observer.min,
    (uint16_t)observer.sec);
  draw_string(width-(19*6), height-8, font_8x5, buffer, colour565(255, 255, 255));
}

void c_planetarium :: fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour, uint16_t alpha)
{
  //fill buffer with background colour
  for(uint16_t xx = 0; xx < w; xx++)
  {
    for(uint16_t yy = 0; yy < h; yy++)
    {
      set_pixel(x+xx, y+yy, colour, alpha);
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

void c_planetarium :: alt_az_to_ra_dec(float alt, float az, float &ra, float &dec)
{
    // Convert degrees to radians
    const float sin_alt = sinf(to_radians(alt));
    const float cos_alt = cosf(to_radians(alt));
    const float sin_az = sinf(to_radians(az));
    const float cos_az = cosf(to_radians(az));
    
    // Compute declination
    dec = asinf(sin_alt * sin_lat + cos_alt * cos_lat * cos_az);
    
    // Compute hour angle
    float ha = atan2f(-sin_az * cos_alt, cos_lat * sin_alt - sin_lat * cos_alt * cos_az);
    
    // Convert hour angle to right ascension
    ra = lst - to_degrees(ha);
    if (ra < 0) ra += 360.0f;
    if (ra >= 360.0f) ra -= 360.0f;
    
    // Convert declination back to degrees
    dec = to_degrees(dec);

}

void inline c_planetarium :: ra_dec_to_alt_az(float ra, float dec, float &alt, float &az)
{
    float H=(lst - ra);

    if(H<0.0f) H+=360.0f;
    if(H>180.0f) H=H-360.0f;
    
    const float sin_H = sin(to_radians(H));
    const float cos_H = cos(to_radians(H));
    const float tan_dec = tan(to_radians(dec));
    const float sin_dec = sin(to_radians(dec));
    const float cos_dec = cos(to_radians(dec));

    az = to_degrees(atan2(sin_H, cos_H*sin_lat - tan_dec*cos_lat));
    alt = to_degrees(asin(sin_lat*sin_dec + cos_lat*cos_dec*cos_H));
    az-=180.0f;
    if(az<0.0f) az+=360.0f;
}

void c_planetarium :: calculate_view(float alt, float az, float &x, float &y, float &z)
{
  //rotate so that observer az is at bottom of screen
  az -= observer.az;

  //make zenith 0 on the x and y axis
  const float sin_alt = sin(to_radians(alt));
  const float cos_alt = cos(to_radians(alt));
  const float sin_az = sin(to_radians(az));
  const float cos_az = cos(to_radians(az));
  x = cos_alt*sin_az;
  y = cos_alt*-cos_az;
  z = sin_alt;

  //rotate around equinox
  const float new_y = y*cos_theta-z*sin_theta;
  const float new_z = y*sin_theta+z*cos_theta;
  z = new_z;
  y = new_y;

  //0 is centre of view, +0.5 is at the top, -0.5 is at the bottom
  x *= view_scale;
  y *= view_scale; 
  z *= view_scale;

}

void inline c_planetarium :: calculate_view_ra_dec(float ra, float dec, float &x, float &y, float &z)
{
  float alt, az;
  ra_dec_to_alt_az(ra, dec, alt, az);
  calculate_view(alt, az, x, y, z);
}

void c_planetarium :: calculate_pixel_coords(float &x, float &y)
{
  x = round((width-height)/2 + (height * (x+0.5f)));
  y = round(height * (1.0f-(y + 0.5f)));
}

void c_planetarium :: plot_constellations()
{
  for(uint16_t idx=0; idx < num_clines; ++idx)
  {

    float alt1, az1;
    ra_dec_to_alt_az(clines[idx].ra1, clines[idx].dec1, alt1, az1);
    
    float alt2, az2;
    ra_dec_to_alt_az(clines[idx].ra2, clines[idx].dec2, alt2, az2);

    float x1, y1, z1;
    calculate_view(alt1, az1, x1, y1, z1);

    float x2, y2, z2;
    calculate_view(alt2, az2, x2, y2, z2);

    if(z1 < 0.0f || z2 < 0.0f) continue;
    calculate_pixel_coords(x1, y1);
    calculate_pixel_coords(x2, y2);
    draw_line(x1, y1, x2, y2, colour565(0, 0, 255), 200); 

  }
}

void c_planetarium :: plot_plane(float pole_alt, float pole_az, uint16_t colour)
{
  float plane_elevation = 90-pole_alt; //maximum height of plane above horizon
  float plane_direction = pole_az+180; //azimuth direction of highest point

  uint16_t az1 = 0;
  float x1, y1, z1;
  float alt = to_degrees(atan(cos(to_radians(az1-plane_direction))*sin(to_radians(plane_elevation))));
  calculate_view(alt, az1, x1, y1, z1);
  calculate_pixel_coords(x1, y1);

  for(uint16_t az=1; az <= 360; az += 1)
  {
    float x2, y2, z2;
    float alt = to_degrees(atan(cos(to_radians(az-plane_direction))*tan(to_radians(plane_elevation))));

    calculate_view(alt, az, x2, y2, z2);
    calculate_pixel_coords(x2, y2);
    
    //don't bother plotting line outside field of observer
    bool draw = (z1 >= 0.0f) && (z2 >= 0.0f);
    
    //draw line between 2 points
    if(draw) draw_line(x1, y1, x2, y2, colour, 128);

    x1 = x2;
    y1 = y2;
    z1 = z2;
    az1 = az;

  }

}

void c_planetarium :: plot_ra_dec_grid(uint16_t colour)
{

  //plot lines of constant dec
  for(int16_t dec = -90; dec < 90; dec += 10)
  {
  
    float x1, y1, z1, alt, az1;
    ra_dec_to_alt_az(0.0f, dec, alt, az1);
    calculate_view(alt, az1, x1, y1, z1);
    calculate_pixel_coords(x1, y1);

    for(uint16_t ra=5; ra <= 360; ra += 5)
    {
      float x2, y2, z2, az;
      ra_dec_to_alt_az(ra, dec, alt, az);
      calculate_view(alt, az, x2, y2, z2);
      calculate_pixel_coords(x2, y2);
  
      //don't bother plotting line outside field of observer
      bool draw = (z1 >= 0.0f) && (z2 >= 0.0f);
      
      //draw line between 2 points
      if(draw) draw_line(x1, y1, x2, y2, colour, 48);

      x1 = x2;
      y1 = y2;
      z1 = z2;
      az1 = az;

    }
  }

  //plot lines of constant ra
  for(uint16_t ra = 0; ra < 360; ra += 10)
  {

    float x1, y1, z1, alt1, az;
    ra_dec_to_alt_az(ra, -90, alt1, az);
    calculate_view(alt1, az, x1, y1, z1);
    calculate_pixel_coords(x1, y1);

    for(int16_t dec=-85; dec <= 90; dec += 5)
    {
      float x2, y2, z2, alt;
      ra_dec_to_alt_az(ra, dec, alt, az);
      if(dec == 90u) az = 0;
      calculate_view(alt, az, x2, y2, z2);
      calculate_pixel_coords(x2, y2);
  
      //don't bother plotting line outside field of observer
      bool draw = (z1 > 0.0f) && (z2 > 0.0f);
      
      //draw line between 2 points
      if(draw) draw_line(x1, y1, x2, y2, colour, 48);

      x1 = x2;
      y1 = y2;
      z1 = z2;
      alt1 = alt;

    }
  }

}

void c_planetarium :: plot_alt_az_grid(uint16_t colour)
{

  //plot lines of constant altitude
  for(int16_t alt = -90; alt <= 90; alt += 10)
  {
    int16_t az1 = 0;
    float x1, y1, z1;
    calculate_view(alt, az1, x1, y1, z1);
    calculate_pixel_coords(x1, y1);

    for(uint16_t az=5; az <= 360; az += 5)
    {
      float x2, y2, z2;
      calculate_view(alt, az, x2, y2, z2);
      calculate_pixel_coords(x2, y2);
  
      //don't bother plotting line outside field of observer
      bool draw = (z1 >= 0.0f) && (z2 >= 0.0f);
      
      //draw line between 2 points
      if(draw) draw_line(x1, y1, x2, y2, colour, 16);

      x1 = x2;
      y1 = y2;
      z1 = z2;
      az1 = az;

    }
  }

  //plot lines of constant azimuth
  for(uint16_t az = 0; az < 360; az += 10)
  {
    int16_t alt1 = -90;
    float x1, y1, z1;
    calculate_view(alt1, az, x1, y1, z1);
    calculate_pixel_coords(x1, y1);

    for(int16_t alt=-85; alt <= 90; alt += 5)
    {
      float x2, y2, z2;
      calculate_view(alt, az, x2, y2, z2);
      calculate_pixel_coords(x2, y2);
  
      //don't bother plotting line outside field of observer
      bool draw = (z1 >= 0.0f) && (z2 >= 0.0f);
      
      //draw line between 2 points
      if(draw) draw_line(x1, y1, x2, y2, colour, 16);

      x1 = x2;
      y1 = y2;
      z1 = z2;
      alt1 = alt;

    }
  }
}

void c_planetarium :: plot_planes()
{
  //plot celestial equator
  float az, alt;
  ra_dec_to_alt_az(90, 90, alt, az);
  plot_plane(observer.latitude, 0, colour565(0, 100, 255));

  //plot ecliptic
  const float orbital_noth_pole_dec = 66.56;
  const float orbital_noth_pole_ra = 270;
  ra_dec_to_alt_az(orbital_noth_pole_ra, orbital_noth_pole_dec, alt, az);
  plot_plane(alt, az, colour565(255, 0, 64));

}

int rand_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void c_planetarium :: plot_cardinal_points()
{
  
  for(uint16_t az=0; az < 4*360; az += 45)
  {
    float x, y, z;
    calculate_view(0, az/4.0f, x, y, z);
    calculate_pixel_coords(x, y);

    if(x >= 0 && x < width && y >= 0 && y < height && z > -0.01f)
    {
      switch(az){
        case 4*0: draw_char(x-6, y+5, font_16x12, 'N',    colour565(255, 128, 0)); break;
        case 4*45: draw_string(x-3, y+5, font_8x5, "NE",  colour565(255, 128, 0)); break;
        case 4*90: draw_char(x-6, y+5, font_16x12,'E',    colour565(255, 128, 0)); break;
        case 4*135: draw_string(x-3, y+5, font_8x5, "SE", colour565(255, 128, 0)); break;
        case 4*180: draw_char(x-6, y+5, font_16x12,'S',   colour565(255, 128, 0)); break;
        case 4*225: draw_string(x-3, y+5, font_8x5, "SW", colour565(255, 128, 0)); break;
        case 4*270: draw_char(x-6, y+5, font_16x12,'W',   colour565(255, 128, 0)); break;
        case 4*315: draw_string(x-3, y+5, font_8x5, "NW", colour565(255, 128, 0)); break;
      };
      fill_circle(x, y, 3, colour565(255, 128, 0), 200);
    }
  }
}


void c_planetarium :: plot_milky_way()
{
  
  //plot galactic plane
  const float north_galactic_pole_ra = 192.25;
  const float north_galactic_pole_dec = 27.23;
  float pole_alt, pole_az;
  ra_dec_to_alt_az(north_galactic_pole_ra, north_galactic_pole_dec, pole_alt, pole_az);

  float plane_elevation = 90-pole_alt; //maximum height of plane above horizon
  float plane_direction = pole_az+180; //azimuth direction of highest point

  int16_t x1 = width-1;
  int16_t x2 = 0;
  int16_t y1 = height-1;
  int16_t y2 = 0;
  srand(1234);
  int16_t last_x = 0;
  int16_t last_y = 0;
  for(float az=0; az < 360; az += 1)
  {

    float x, y, z;
    float alt = to_degrees(atan(cos(to_radians(az-plane_direction))*tan(to_radians(plane_elevation))));

    calculate_view(alt, az, x, y, z);
    calculate_pixel_coords(x, y);

    if((x-last_x)*(x-last_x) + (y-last_y)*(y-last_y) < 100) continue;
    last_x = x; last_y = y;

    if(x >= 0 && x < width && y >= 0 && y < height && z > -0.01f)
    {
      //draw line between 2 points
      uint8_t n = rand_range(15, 30);
      for(uint8_t i=0; i<rand_range(100, 200); i++)
      {
        fill_circle(x+rand_range(-n, n), y+rand_range(-n, n), rand_range(1, 5), colour565(255, 255, 255), 8);
      }
    }
  }

}

void c_planetarium :: plot_stars()
{

  float view_ra, view_dec;
  alt_az_to_ra_dec(observer.alt, observer.az, view_ra, view_dec);

  for(uint16_t idx=0; idx < num_stars; ++idx)
  {

    if(stars[idx].mag > observer.smallest_magnitude) continue;
    const float dec_seperation = abs(view_dec - stars[idx].dec);
    if(abs(view_dec - stars[idx].dec) > observer.field) continue;

    //uint32_t t0 = micros();

    float x, y, z;
    calculate_view_ra_dec(stars[idx].ra, stars[idx].dec, x, y, z);
    calculate_pixel_coords(x, y);

    //Serial.print("calculating2: ");
    //Serial.println(micros()-t0);

    //don't bother plotting stars outside field of observer
    if(x>width) continue;
    if(y>height) continue;
    if(x<0) continue;
    if(y<0) continue;
    if(z < 0) continue;


    int8_t mag = stars[idx].mag;
    uint8_t mk = stars[idx].mk;

    //t0 = micros();
    if(mag <= 1)
    {
      fill_circle(x, y, 6, star_colour(mk, 1), 9);
      fill_circle(x, y, 3, star_colour(mk, 3), 128);
      fill_circle(x, y, 2, star_colour(mk, 1));
    }
    else if(mag <= 2)
    {
      fill_circle(x, y, 2, star_colour(mk, 3));
      fill_circle(x, y, 1, star_colour(mk, 1));
    }
    else if(mag <= 3)
    {
      fill_circle(x, y, 1, star_colour(mk, 2));
      set_pixel(x, y, star_colour(mk, 1));
    }
    else
    {
      set_pixel(x, y, star_colour(mk, mag-3));
    }
    //Serial.print("plotting: ");
    //Serial.println(micros()-t0);

  }
}

void c_planetarium :: plot_planets()
{
  double earth_x, earth_y, earth_z;
  compute_planet_position(julian_date, elements[2], rates[2], extra_terms[2], earth_x, earth_y, earth_z);

  for(uint16_t idx=0; idx < 9; ++idx)
  {
    if(idx == 2) continue;

    double planet_x, planet_y, planet_z;
    compute_planet_position(julian_date, elements[idx], rates[idx], extra_terms[idx], planet_x, planet_y, planet_z);

    double ra, dec;
    convert_to_ra_dec(planet_x-earth_x, planet_y-earth_y, planet_z-earth_z, ra, dec);

    float alt, az;
    ra_dec_to_alt_az(ra, dec, alt, az);

    float x, y, z;
    calculate_view(alt, az, x, y, z);

    //don't bother plotting stars outside field of observer
    if(z < 0.0f) continue;
    if(x*x + y*y > 0.5) continue;

    //get coordinated and magnitude of x
    calculate_pixel_coords(x, y);

    switch(idx)
    {
      case 0: draw_object(x, y, 4, (uint16_t*)mercury); break;
      case 1: draw_object(x, y, 4, (uint16_t*)venus); break;
      case 3: draw_object(x, y, 4, (uint16_t*)mars); break;
      case 4: draw_object(x, y, 4, (uint16_t*)jupiter); break;
      case 5: 
        draw_object(x, y, 4, (uint16_t*)saturn); 
        draw_line(x-6, y-6, x+6, y+6, colour565(0x49, 0x42, 0x3b));
        break;
      case 6: draw_object(x, y, 4, (uint16_t*)uranus); break;
      case 7: draw_object(x, y, 4, (uint16_t*)neptune); break;
    };
    draw_string(x+4, y-16, font_8x5, planet_names[idx], colour565(223, 136, 247));

  }

  double ra, dec;
  convert_to_ra_dec(-earth_x, -earth_y, -earth_z, ra, dec);

  float alt, az;
  ra_dec_to_alt_az(ra, dec, alt, az);

  float x, y, z;
  calculate_view(alt, az, x, y, z);

  //don't bother plotting stars outside field of observer
  if(z < 0.0f) return;
  if(x*x + y*y > 0.5) return;

  //get coordinated and magnitude of x
  calculate_pixel_coords(x, y);
  draw_object(x, y, 10, (uint16_t*)sun);
  draw_string(x+4, y-16, font_8x5, "Sun", colour565(223, 136, 247));
}

void c_planetarium :: plot_constellation_names()
{
  for(uint16_t idx=0; idx < num_constellations; ++idx)
  {

    float alt, az;
    ra_dec_to_alt_az(constellation_centres[idx].ra*15.0f, constellation_centres[idx].dec, alt, az);

    float x, y, z;
    calculate_view(alt, az, x, y, z);

    if(abs(x) > 0.5f || abs(y) > 0.5f) continue;
    if(z < 0) continue;
    calculate_pixel_coords(x, y);
    draw_string(x, y, font_8x5, constellation_names[idx], colour565(136, 247, 225));
    
  }
}

float c_planetarium :: greenwich_sidereal_time()
{
    //Calculate Greenwich Mean Sidereal Time (GMST) given a UTC datetime.

    // Convert to Julian date
    double ut = observer.hour + observer.min / 60.0f + observer.sec / 3600.0f;
    
    uint8_t month = observer.month;
    uint16_t year = observer.year;
    if(month <= 2)
    {
        year -= 1;
        month += 12;
    }
    
    double a = floor(year / 100.0);
    double b = 2.0 - a + floor(a / 4.0);
    julian_date = floor(365.25 * (year + 4716.0)) + floor(30.6001 * (month + 1.0)) + observer.day + b - 1524.5 + ut / 24.0;
    
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

//code to compute planet positions adapted from:
//https://www.celestialprogramming.com/planets_with_keplers_equation.html
//by Greg Miller gmiller@gregmiller.net 2021
const s_keplarian elements[] = { //https://ssd.jpl.nasa.gov/txt/p_elem_t2.txt
//Mercury  
        { 0.38709843,      0.20563661,      7.00559432,      252.25166724,     77.45771895,     48.33961819},
//Venus    
        { 0.72332102,      0.00676399,      3.39777545,      181.97970850,    131.76755713,     76.67261496},
//EM Bary  
        { 1.00000018,      0.01673163,     -0.00054346,      100.46691572,    102.93005885,     -5.11260389},
//Mars     
        { 1.52371243,      0.09336511,      1.85181869,       -4.56813164,    -23.91744784,     49.71320984},
//Jupiter  
        { 5.20248019,      0.04853590,      1.29861416,       34.33479152,     14.27495244,    100.29282654},
//Saturn   
        { 9.54149883,      0.05550825,      2.49424102,       50.07571329,     92.86136063,    113.63998702},
//Uranus   
        {19.18797948,      0.04685740,      0.77298127,      314.20276625,    172.43404441,     73.96250215},
//Neptune  
        {30.06952752,      0.00895439,      1.77005520,      304.22289287,     46.68158724,    131.78635853},
//Pluto    
        {39.48686035,      0.24885238,     17.14104260,      238.96535011,    224.09702598,    110.30167986},
};
const s_keplarian rates[] = { //https://ssd.jpl.nasa.gov/txt/p_elem_t2.txt
//Mercury  
         { 0.00000000,      0.00002123,     -0.00590158,   149472.67486623,      0.15940013,     -0.12214182},
//Venus    
         {-0.00000026,     -0.00005107,      0.00043494,    58517.81560260,      0.05679648,     -0.27274174},
//EM Bary  
         {-0.00000003,     -0.00003661,     -0.01337178,    35999.37306329,      0.31795260,     -0.24123856},
//Mars     
         { 0.00000097,      0.00009149,     -0.00724757,    19140.29934243,      0.45223625,     -0.26852431},
//Jupiter  
         {-0.00002864,      0.00018026,     -0.00322699,     3034.90371757,      0.18199196,      0.13024619},
//Saturn   
         {-0.00003065,     -0.00032044,      0.00451969,     1222.11494724,      0.54179478,     -0.25015002},
//Uranus   
         {-0.00020455,     -0.00001550,     -0.00180155,      428.49512595,      0.09266985,      0.05739699},
//Neptune  
         { 0.00006447,      0.00000818,      0.00022400,      218.46515314,      0.01009938,     -0.00606302},
//Pluto    
         { 0.00449751,      0.00006016,      0.00000501,      145.18042903,     -0.00968827,     -0.00809981},
};



const s_extra_terms extra_terms[] = {
   {0.0,            0.0,           0.0,          0.0        },
   {0.0,            0.0,           0.0,          0.0        },
   {0.0,            0.0,           0.0,          0.0        },
   {0.0,            0.0,           0.0,          0.0        },
   {-0.00012452,    0.06064060,   -0.35635438,   38.35125000}, //Jupiter
   { 0.00025899,   -0.13434469,    0.87320147,   38.35125000}, //Saturn
   { 0.00058331,   -0.97731848,    0.17689245,    7.67025000}, //Uranus
   {-0.00041348,    0.68346318,   -0.10162547,    7.67025000}, //Neptune
   {-0.01262724,    0,             0,             0}           //Pluto
};


// Solving Kepler's equation iteratively for the Eccentric Anomaly
double c_planetarium :: solve_kepler(double M, double e, double E)
{
    double dM=M - (E-to_degrees(e)*sin(to_radians(E)));
    //double dM=M - (E-e/to_radians(sin(to_radians(E))));
    double dE=dM/(1-e*cos(to_radians(E)));
    return dE;
}

// Convert heliocentric coordinates to RA and Dec
void c_planetarium :: convert_to_ra_dec(double x, double y, double z, double &ra, double &dec) 
{
    //Convert from Cartesian to polar coordinates 
    const double r=sqrt(x*x+y*y+z*z);
    ra=atan2(y,x);
    dec=acos(z/r);

    //Make sure RA is positive, and Dec is in range +/-90
    if(ra<0){ra+=2*M_PI;}
    dec=0.5*M_PI-dec;

    ra = to_degrees(ra);
    dec = to_degrees(dec);
}

void c_planetarium :: compute_planet_position(double jd, s_keplarian elements, s_keplarian rates, s_extra_terms extra_terms, double &x, double &y, double&z){

    //Algorithm from Explanatory Supplement to the Astronomical Almanac ch8 P340
    //Step 1:
    const double T=(jd-2451545.0)/36525.0;
    double a=elements.a+rates.a*T;
    double e=elements.e+rates.e*T;
    double I=elements.I+rates.I*T;
    double L=elements.L+rates.L*T;
    double w=elements.w+rates.w*T;
    double O=elements.O+rates.O*T;

    //Step 2:
    double ww=w-O;
    const double b=extra_terms.b;
    const double c=extra_terms.c;
    const double s=extra_terms.s;
    const double f=extra_terms.f;
    double M=L - w + b*T*T + c*cos(to_radians(f*T)) + s*sin(to_radians(f*T));

    while(M>180.0){M-=360.0;}

    double E=M+57.29578*e*sin(to_radians(M));
    double dE=1.0;
    uint8_t n=0;
    while(abs(dE)>1e-7 && n<10)
    {
        dE=solve_kepler(M,e,E);
        E+=dE;
        n++;
    }

    //Step 4:
    const double xp=a*(cos(to_radians(E))-e);
    const double yp=a*sqrt(1-e*e)*sin(to_radians(E));
    const double zp=0;

    //Step 5:
    a=to_radians(a); e=to_radians(e); I=to_radians(I); L=to_radians(L); ww=to_radians(ww); O=to_radians(O);
    const double xecl=(cos(ww)*cos(O)-sin(ww)*sin(O)*cos(I))*xp + (-sin(ww)*cos(O)-cos(ww)*sin(O)*cos(I))*yp;
    const double yecl=(cos(ww)*sin(O)+sin(ww)*cos(O)*cos(I))*xp + (-sin(ww)*sin(O)+cos(ww)*cos(O)*cos(I))*yp;
    const double zecl=(sin(ww)*sin(I))*xp + (cos(ww)*sin(I))*yp;

    //Step 6:
    const double eps=to_radians(23.43928);

    x=xecl;
    y=cos(eps)*yecl - sin(eps)*zecl;
    z=sin(eps)*yecl + cos(eps)*zecl;

}

//code adapted from https://www.celestialprogramming.com/lowprecisionmoonposition.html
//Low precision geocentric moon position (RA,DEC) from Astronomical Almanac page D22 (2017 ed)
float c_planetarium :: sind(float r){return sin(to_radians(r));}
float c_planetarium :: cosd(float r){return cos(to_radians(r));}
void c_planetarium :: plot_moon()
{

	float T = (julian_date-2451545)/36525;
	float L = 218.32 + 481267.881*T + 
    6.29*sind(135.0 + 477198.87*T) - 
    1.27*sind(259.3 - 413335.36*T) + 
    0.66*sind(235.7 + 890534.22*T) + 
    0.21*sind(269.9 + 954397.74*T) - 
    0.19*sind(357.5 + 35999.05*T) - 
    0.11*sind(186.5 + 966404.03*T);

	float B = 5.13*sind( 93.3 + 483202.02*T) + 
    0.28*sind(228.2 + 960400.89*T) - 
    0.28*sind(318.3 + 6003.15*T) - 
    0.17*sind(217.6 - 407332.21*T);

	float P = 0.9508 + 0.0518*cosd(135.0 + 477198.87*T) + 
    0.0095*cosd(259.3 - 413335.36*T) + 
    0.0078*cosd(235.7 + 890534.22*T) + 
    0.0028*cosd(269.9 + 954397.74*T);

	float SD=0.2724*P;
	float r=1/sind(P);

	float l = cosd(B) * cosd(L);
	float m = 0.9175*cosd(B)*sind(L) - 0.3978*sind(B);
	float n = 0.3978*cosd(B)*sind(L) + 0.9175*sind(B);

	float ra=to_degrees(atan2(m,l));
	if(ra<0)ra+=360;
	float dec=to_degrees(asin(n));

	float alt, az;
  ra_dec_to_alt_az(ra, dec, alt, az);

  float x, y, z;
  calculate_view(alt, az, x, y, z);

  if(abs(x) > 0.5f || abs(y) > 0.5f) return;
  if(z < 0) return;
  calculate_pixel_coords(x, y);
  
  draw_object(x, y, 10, (uint16_t*)moon);
  draw_string(x+4, y-16, font_8x5, "Moon", colour565(223, 136, 247));
}

void c_planetarium :: draw_object(uint16_t x, uint16_t y, uint16_t r, uint16_t* image)
{
  for(int16_t xx = -r; xx<r; xx++)
  {
    for(int16_t yy = -r; yy<r; yy++)
    {
      if((xx*xx+yy*yy)>(r*r)) continue;
      set_pixel(x+xx, y+yy, image[((xx+r)*2*r)+yy+r]);
    }
  }
}