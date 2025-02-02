#include <cmath>
#include <Arduino.h>
#include "planetarium.h"
#include "stars.h"
#include "constellations.h"
#include "clines.h"


inline float to_radians(float x)
{
  return x * (float)M_PI/180.0f;
}

inline float to_degrees(float x)
{
  return x * 180.0f/(float)M_PI;
}

void ra_dec_to_alt_az(float ra, float dec, float lst, float latitude, float &alt, float &az)
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
      cos(to_radians(H))*sin(to_radians(latitude)) - tan(to_radians(dec))*cos(to_radians(latitude))));
    alt = to_degrees(asin(sin(to_radians(latitude))*sin(to_radians(dec)) + cos(to_radians(latitude))*cos(to_radians(dec))*cos(to_radians(H))));
    az-=180.0f;

    if(az<0)
    {
      az+=360.0f;
    }
}

void calculate_view(float alt, float az, float observer_alt, float observer_az, float &x, float &y)
{
  //make zenith 0 on the x and y axis 
  x = (90.0f-alt) * sin(to_radians(az-observer_az));
  y = (90.0f-alt) * -cos(to_radians(az-observer_az)) + (90.0f-observer_alt);

}

void calculate_pixel_coords(float &x, float &y, uint16_t width, uint16_t height, float field)
{
  x = round(width * (x/field+0.5f));
  y = round(height * (1.0f-(y/field + 0.5f)));
}

uint16_t plot_constellations(uint16_t cline_x1[], uint16_t cline_y1[],
  uint16_t cline_x2[], uint16_t cline_y2[], bool dirty[],
  uint16_t max_clines_to_plot, uint16_t screen_width, 
  uint16_t screen_height, const s_observer &observer)
{
  uint16_t num_plotted = 0;
  for(uint16_t idx=0; idx < num_clines; ++idx)
  {

    float alt1, az1;
    ra_dec_to_alt_az(clines[idx].ra1, clines[idx].dec1, observer.lst, observer.latitude, alt1, az1);
    
    float alt2, az2;
    ra_dec_to_alt_az(clines[idx].ra2, clines[idx].dec2, observer.lst, observer.latitude, alt2, az2);

    float x1, y1;
    calculate_view(alt1, az1, observer.alt, observer.az, x1, y1);

    float x2, y2;
    calculate_view(alt2, az2, observer.alt, observer.az, x2, y2);

    //don't bother plotting stars outside field of observer
    bool draw = 
    ((x1 < observer.field/2) && (x1 > -observer.field/2) && (y1 < observer.field/2) && (y1 > -observer.field/2)) ||
    ((x2 < observer.field/2) && (x2 > -observer.field/2) && (y2 < observer.field/2) && (y2 > -observer.field/2));
    if(!draw) continue;

    calculate_pixel_coords(x1, y1, screen_width, screen_height, observer.field);
    calculate_pixel_coords(x2, y2, screen_width, screen_height, observer.field);
    
    if(num_plotted < max_clines_to_plot)
    {
      //if line has changed mark the y region to be redrawn
      //if(cline_x1[num_plotted] != (uint16_t)x1 || cline_y1[num_plotted] != (uint16_t)y1 || cline_x2[num_plotted] != (uint16_t)x2 || cline_y2[num_plotted] != (uint16_t)y2)
      //{
      //  for(uint16_t yy=std::min(y1, y2); yy<std::max(y1, y2); ++yy) dirty[yy] = true;
      //  for(uint16_t yy=std::min(cline_y1[num_plotted], cline_y2[num_plotted]); yy<std::max(cline_y1[num_plotted], cline_y2[num_plotted]); ++yy) dirty[yy] = true;
      //}
      cline_x1[num_plotted] = x1;
      cline_y1[num_plotted] = y1;
      cline_x2[num_plotted] = x2;
      cline_y2[num_plotted] = y2;
      num_plotted++;
    }
    else
    {
      return num_plotted;
    }

  }
  return num_plotted;
}

//plot stars
uint16_t plot_stars(uint16_t star_x[], uint16_t star_y[], int8_t mag[], uint8_t col[], bool dirty[],
  uint16_t max_stars_to_plot, uint16_t screen_width,
  uint16_t screen_height, const s_observer &observer)
{
  uint16_t num_plotted = 0;
  for(uint16_t idx=0; idx < num_stars; ++idx)
  {

    if(stars[idx].mag > observer.smallest_magnitude) continue;

    float alt, az;
    ra_dec_to_alt_az(stars[idx].ra, stars[idx].dec, observer.lst, observer.latitude, alt, az);

    float x, y;
    calculate_view(alt, az, observer.alt, observer.az, x, y);

    //don't bother plotting stars outside field of observer
    if(x > observer.field/2) continue;
    if(x < -observer.field/2) continue;
    if(y > observer.field/2) continue;
    if(y < -observer.field/2) continue;

    calculate_pixel_coords(x, y, screen_width, screen_height, observer.field);
    if(num_plotted < max_stars_to_plot)
    {
      //if line has changed mark the y region to be redrawn
      Serial.print(num_plotted);
      Serial.print(" ");
      Serial.print(star_x[num_plotted]);
      Serial.print(" ");
      Serial.print((uint16_t)x);
      Serial.print(" ");
      Serial.print(star_y[num_plotted]);
      Serial.print(" ");
      Serial.print((uint16_t)y);
      Serial.println("");
      if(star_x[num_plotted] != (uint16_t)x || star_y[num_plotted] != (uint16_t)y)
      {
        for(uint16_t yy=y-2; yy<y+2; ++yy) dirty[yy] = true;
        for(uint16_t yy=star_y[num_plotted]-2; yy<star_y[num_plotted]+2; ++yy) dirty[yy] = true;
      }

      star_x[num_plotted] = x;
      star_y[num_plotted] = y;
      mag[num_plotted] = stars[idx].mag;
      col[num_plotted] = stars[idx].mk;
      num_plotted++;
    }
    else
    {
      return num_plotted++;
    }
  }
  return num_plotted;
}

void plot_constellation_names(uint16_t constellation_x[], uint16_t constellation_y[], bool visible[], bool dirty[],
  s_observer observer, uint16_t screen_width, uint16_t screen_height)
{
  for(uint16_t idx=0; idx < num_constellations; ++idx)
  {

    float alt, az;
    ra_dec_to_alt_az(constellation_centres[idx].ra*15, constellation_centres[idx].dec, observer.lst, observer.latitude, alt, az);

    float x, y;
    calculate_view(alt, az, observer.alt, observer.az, x, y);

    visible[idx] = (x < observer.field/2) && (x > -observer.field/2) && (y < observer.field/2) && (y > -observer.field/2);
    calculate_pixel_coords(x, y, screen_width, screen_height, observer.field);
    
    //if position has changed mark the y region to be redrawn
    //if(constellation_x[idx] != (uint16_t)x || constellation_y[idx] != (uint16_t)y)
    //{
    //  dirty[std::min(constellation_y[idx], (uint16_t)239)] = true;
    //  dirty[std::min((uint16_t)y, (uint16_t)239)] = true;
    //}

    constellation_x[idx] = x;
    constellation_y[idx] = y;
    
  }
}

float greenwich_sidereal_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    //Calculate Greenwich Mean Sidereal Time (GMST) given a UTC datetime.

    // Convert to Julian date
    double ut = hour + minute / 60.0f + second / 3600.0f;
    
    if(month <= 2)
    {
        year -= 1;
        month += 12;
    }
    
    double a = floor(year / 100.0);
    double b = 2.0 - a + floor(a / 4.0);
    double julian_date = floor(365.25 * (year + 4716.0)) + floor(30.6001 * (month + 1.0)) + day + b - 1524.5 + ut / 24.0;
    
    // Julian centuries from J2000.0
    double centuries = (julian_date - 2451545.0) / 36525.0;
    
    // Greenwich Mean Sidereal Time in degrees
    double gmst = 280.46061837 + 360.98564736629 * (julian_date - 2451545.0) + 0.000387933 * pow(centuries,2.0) - pow(centuries, 3.0) / 38710000.0;
    
    return fmod(gmst, 360.0);  // Ensure within [0, 360] degrees
}

float local_sidereal_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, float longitude)
{
    //Calculate Local Sidereal Time (LST) for a given longitude.
    float gmst = greenwich_sidereal_time(year, month, day, hour, minute, second);
    
    //Convert longitude to degrees (-180 to 180) to match convention
    float lst = fmod(gmst + longitude, 360.0f);
    
    return lst;
}

void star_colour(float mk, uint8_t &r, uint8_t &g, uint8_t &b)
{
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
}