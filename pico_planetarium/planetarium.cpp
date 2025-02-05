#include <cmath>
#include <cstdio>
#include "planetarium.h"
#include "stars.h"
#include "constellations.h"
#include "clines.h"
#include "font_8x5.h"
#include "Arduino.h"

uint16_t buffer[height][width] = {};
const char * const planet_names[]={"Mercury","Venus","Earth","Mars","Jupiter","Saturn","Uranus","Neptune","Pluto","Sun"};



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

void c_planetarium::fill_circle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour)
{
  for (int16_t y = -radius; y <= radius; y++)
  {
    for (int16_t x = -radius; x <= radius; x++)
    {
      if (x * x + y * y <= radius * radius)
      {
          set_pixel(xc + x, yc + y, colour);
      }
    }
  }
}

void c_planetarium :: draw_circle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour) 
{
    int16_t x = 0, y = radius;
    int16_t d = 1 - radius;
    
    while (x <= y) {
        set_pixel(xc + x, yc + y, colour);
        set_pixel(xc - x, yc + y, colour);
        set_pixel(xc + x, yc - y, colour);
        set_pixel(xc - x, yc - y, colour);
        set_pixel(xc + y, yc + x, colour);
        set_pixel(xc - y, yc + x, colour);
        set_pixel(xc + y, yc - x, colour);
        set_pixel(xc - y, yc - x, colour);
        
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

void c_planetarium :: update(s_observer o)
{
  observer = o;
  local_sidereal_time();

  fill_rect(0, 0, width, height, colour565(5, 0, 10));

  plot_constellations();
  plot_stars();
  plot_planets();
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

void c_planetarium :: alt_az_to_ra_dec(float alt, float az, float &ra, float &dec)
{
    // Convert degrees to radians
    alt = to_radians(alt);
    az = to_radians(az);
    float lat = to_radians(observer.latitude);
    
    // Compute declination
    dec = asin(sin(alt) * sin(lat) + cos(alt) * cos(lat) * cos(az));
    
    // Compute hour angle
    float ha = atan2(-sin(az) * cos(alt), cos(lat) * sin(alt) - sin(lat) * cos(alt) * cos(az));
    
    // Convert hour angle to right ascension
    ra = lst - to_degrees(ha);
    if (ra < 0) ra += 360.0;
    if (ra >= 360.0) ra -= 360.0;
    
    // Convert declination back to degrees
    dec = to_degrees(dec);

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
  x = (90.0-alt) * sin(to_radians(az-observer.az));
  y = (90.0-alt) * -cos(to_radians(az-observer.az)) + (90.0f-observer.alt);

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

  float view_ra, view_dec;
  alt_az_to_ra_dec(observer.alt, observer.az, view_ra, view_dec);

  for(uint16_t idx=0; idx < num_stars; ++idx)
  {

    if(stars[idx].mag > observer.smallest_magnitude) continue;
    const float dec_seperation = abs(view_dec - stars[idx].dec);
    if(abs(view_dec - stars[idx].dec) > observer.field) continue;

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
      fill_circle(x, y, 3, star_colour(mk, 3));
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

    float x, y;
    calculate_view(alt, az, x, y);

    //don't bother plotting stars outside field of observer
    if(x > observer.field/2) continue;
    if(x < -observer.field/2) continue;
    if(y > observer.field/2) continue;
    if(y < -observer.field/2) continue;

    //get coordinated and magnitude of x
    calculate_pixel_coords(x, y);

    const uint16_t colours[] = {
      colour565(163,161,155), //mercury
      colour565(248,248,186), //venus
      colour565(163,161,155), //earth
      colour565(255, 83, 73), //mars
      colour565(222,184,135), //jupiter
      colour565(218,165,32), //saturn
      colour565(173,216,230), //uranus
      colour565(0,191,255), //neptune
      colour565(255,255,255), //pluto`
    };

    uint16_t colour = colours[idx];
    fill_circle(x, y, 4, colour);
    draw_string(x+4, y-16, font_8x5, planet_names[idx], colour565(223, 136, 247));

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
      draw_string(x, y, font_8x5, constellation_names[idx], colour565(136, 247, 225));
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
    julian_date = floor(365.25 * (observer.year + 4716.0)) + floor(30.6001 * (observer.month + 1.0)) + observer.day + b - 1524.5 + ut / 24.0;
    
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

/*
void c_planetarium :: fill_circle(int centerX, int centerY, int radius, uint16_t) {
    // Calculate the bounding box for the circle
    uint16_t x_start = centerX - radius;
    uint16_t x_end = centerX + radius;
    uint16_t y_start = centerY - radius;
    uint16_t y_end = centerY + radius;

    for (uint16_t y = y_start; y <= y_end; y++) {
        for (uint16_t x = x_start; x <= x_end; x++) {
            // Check if the point (x, y) is inside the circle using the equation (x - cx)^2 + (y - cy)^2 <= r^2
            if (pow(x - centerX, 2) + pow(y - centerY, 2) <= pow(radius, 2)) {
                set_pixel(x, y, colour);  // Fill the pixel (or do something with the pixel)
            }
        }
    }
}
*/

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
