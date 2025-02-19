#include <cmath>
#include <cstdio>
#include "planetarium.h"
#include "stars.h"
#include "constellations.h"
#include "clines.h"
#include "objects.h"
#include "skyline.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "images.h"

#include "Arduino.h"

//#include "pico/stdlib.h"

const char * const planet_names[]={"Mercury","Venus","Earth","Mars","Jupiter","Saturn","Uranus","Neptune","Pluto","Sun"};

void c_planetarium :: update(s_observer o)
{
  observer = o;
  local_sidereal_time();
  frame_buffer.fill_rect(0, 0, width, height, frame_buffer.colour565(5, 0, 50));

  //calculate constants related to observer view
  sin_lat = sin(to_radians(observer.latitude));
  cos_lat = cos(to_radians(observer.latitude));
  view_scale = 1.0f/(2.0f*sin(to_radians(observer.field/2.0f)));
  const float theta = -(90-observer.alt);
  sin_theta = sin(to_radians(theta));
  cos_theta = cos(to_radians(theta));
  build_rotation_matrix();

  //plot_milky_way(); //155ms 
  plot_alt_az_grid(frame_buffer.colour565(54, 50, 90)); //9ms 
  plot_ra_dec_grid(frame_buffer.colour565(54, 0, 65)); //9ms 
  plot_planes(); //4ms 
  plot_constellations(); //8ms 
  plot_stars(); //25ms 
  plot_planets(); //3ms
  plot_moon(); //2ms 
  plot_constellation_names(); //5ms
  plot_objects();

  //obscure the area bellow the horizon
  //uint16_t view_major_radius = height/(2*sin(to_radians(observer.field/2)));
  //uint16_t view_minor_radius = view_major_radius * sin(to_radians(observer.alt));
  //const int a = view_major_radius;
  //const int b = view_minor_radius;


  //11ms
  //draw skyline
  //int max_y = observer.alt>89.0f?height/2:0;
  //for (int y = -height/2; y <= max_y; y++) {
  //  for (int x = 0; x <= width/2; x++) {
  //    if (((int64_t)x * x) * ((int64_t)b * b) + ((int64_t)y * y) * ((int64_t)a * a) > ((int64_t)a * a) * ((int64_t)b * b)) {
  //        frame_buffer.set_pixel(width/2 + x, height/2 - y, 0, 200);
  //        frame_buffer.set_pixel(width/2 - x, height/2 - y, 0, 200);
  //    }
  //  }
  //}

  //90ms
  //int min_y = observer.alt>89.0f?-height/2:0;
  //for (int y = min_y; y <= height/2; y++) {
  //  for (int x = -width/2; x < width/2; x++) {
  //    uint16_t from_x = 240 + (x*240/view_major_radius);
  //    uint16_t from_y = 240 + (y*240/view_minor_radius);
  //    if (((int64_t)x * x) * ((int64_t)b * b) + ((int64_t)y * y) * ((int64_t)a * a) < ((int64_t)a * a) * ((int64_t)b * b)) {
        //if(from_x < 480 && from_y < 480 && skyline[from_x * 480 + from_y] != 0xffff)
        //{
  //        frame_buffer.set_pixel(width/2 + x, height/2 + y, 0, 200);
        //}
  //    }
  //  }
  //}

  plot_cardinal_points();

  char buffer[100];
  snprintf(buffer, 100, "%0.1f\x7f,%0.1f\x7f", observer.latitude, observer.longitude);
  frame_buffer.draw_string(0, height-8, font_8x5, buffer, frame_buffer.colour565(255, 255, 255));
  snprintf(buffer, 100, "%04u-%02u-%02u %02u:%02u:%02u",
    observer.year,
    (uint16_t)observer.month,
    (uint16_t)observer.day,
    (uint16_t)observer.hour,
    (uint16_t)observer.min,
    (uint16_t)observer.sec);
  frame_buffer.draw_string(width-(19*6), height-8, font_8x5, buffer, frame_buffer.colour565(255, 255, 255));
}

inline float c_planetarium :: to_radians(float x)
{
  return x * (float)M_PI/180.0f;
}

inline float c_planetarium :: to_degrees(float x)
{
  return x * 180.0f/(float)M_PI;
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

void c_planetarium :: calculate_view_alt_az(float alt, float az, float &x, float &y, float &z)
{
  //convert to x, y, z
  const float sin_alt = sin(to_radians(alt));
  const float cos_alt = cos(to_radians(alt));
  const float sin_az = sin(to_radians(180+az));
  const float cos_az = cos(to_radians(180+az));
  x = cos_alt*-sin_az;
  y = cos_alt*cos_az;
  z = sin_alt;

  calculate_view_horizontal_x_y_z(x, y, z);

}

void c_planetarium :: calculate_view_equatorial_x_y_z(float &x, float &y, float &z)
{

  float new_x = rotation_matrix[0][0]*x+rotation_matrix[0][1]*y+rotation_matrix[0][2]*z;
  float new_y = rotation_matrix[1][0]*x+rotation_matrix[1][1]*y+rotation_matrix[1][2]*z;
  float new_z = rotation_matrix[2][0]*x+rotation_matrix[2][1]*y+rotation_matrix[2][2]*z;

  //0 is centre of view, +0.5 is at the top, -0.5 is at the bottom
  x = new_x * view_scale;
  y = new_y * view_scale; 
  z = new_z * view_scale;

}

void c_planetarium :: calculate_view_horizontal_x_y_z(float &x, float &y, float &z)
{

  float new_x = view_rotation_matrix[0][0]*x+view_rotation_matrix[0][1]*y+view_rotation_matrix[0][2]*z;
  float new_y = view_rotation_matrix[1][0]*x+view_rotation_matrix[1][1]*y+view_rotation_matrix[1][2]*z;
  float new_z = view_rotation_matrix[2][0]*x+view_rotation_matrix[2][1]*y+view_rotation_matrix[2][2]*z;

  //0 is centre of view, +0.5 is at the top, -0.5 is at the bottom
  x = new_x * view_scale;
  y = new_y * view_scale; 
  z = new_z * view_scale;

}

void inline c_planetarium :: calculate_view_ra_dec(float ra, float dec, float &x, float &y, float &z)
{

  //convert to x, y, z
  const float sin_dec = sin(to_radians(dec));
  const float cos_dec = cos(to_radians(dec));
  const float sin_ra = sin(to_radians(-ra));
  const float cos_ra = cos(to_radians(-ra));
  x = cos_dec*-sin_ra;
  y = cos_dec*cos_ra;
  z = sin_dec;

  calculate_view_equatorial_x_y_z(x, y, z);

}

void c_planetarium :: calculate_pixel_coords(float &x, float &y)
{
  x = round((width-height)/2 + (height * (x+0.5f)));
  y = round(height * (1.0f-(y + 0.5f)));
}

void c_planetarium :: plot_constellations()
{
  uint16_t colour = frame_buffer.colour565(68, 123, 127);
  for(uint16_t idx=0; idx < num_clines; ++idx)
  {
    float x1 = clines[idx].x1;
    float y1 = clines[idx].y1;
    float z1 = clines[idx].z1;
    
    calculate_view_equatorial_x_y_z(x1, y1, z1);
    
    float x2 = clines[idx].x2;
    float y2 = clines[idx].y2;
    float z2 = clines[idx].z2;
    calculate_view_equatorial_x_y_z(x2, y2, z2);

    if(z1 < 0.0f || z2 < 0.0f) continue;
    calculate_pixel_coords(x1, y1);
    calculate_pixel_coords(x2, y2);
    frame_buffer.draw_line(x1, y1, x2, y2, colour, 200); 

  }
}

void c_planetarium :: plot_plane(float pole_alt, float pole_az, uint16_t colour)
{
  float plane_elevation = 90-pole_alt; //maximum height of plane above horizon
  float plane_direction = pole_az+180; //azimuth direction of highest point

  uint16_t az1 = 0;
  float x1, y1, z1;
  float alt = to_degrees(atan(cos(to_radians(az1-plane_direction))*sin(to_radians(plane_elevation))));
  calculate_view_alt_az(alt, az1, x1, y1, z1);
  calculate_pixel_coords(x1, y1);

  for(uint16_t az=1; az <= 360; az += 1)
  {
    float x2, y2, z2;
    float alt = to_degrees(atan(cos(to_radians(az-plane_direction))*tan(to_radians(plane_elevation))));

    calculate_view_alt_az(alt, az, x2, y2, z2);
    calculate_pixel_coords(x2, y2);
    
    //don't bother plotting line outside field of observer
    bool draw = (z1 >= 0.0f) && (z2 >= 0.0f);
    
    //draw line between 2 points
    if(draw) frame_buffer.draw_line(x1, y1, x2, y2, colour);

    x1 = x2;
    y1 = y2;
    z1 = z2;
    az1 = az;

  }

}

void c_planetarium :: plot_ra_dec_grid(uint16_t colour)
{

  //plot lines of conastant right declination
  for(int dec = 0; dec<90; dec+=10)
  {
    float r = cos(to_radians(dec));
    float z = sin(to_radians(dec));
    float x, y;
    float x_octants[8], y_octants[8], z_octants[8];
    float last_x_octants[8], last_y_octants[8], last_z_octants[8];
    bool first_iteration = true;
    
    for(x=0.0f; x<1.0f; x+=0.177*r)
    {
      y = sqrt((r*r)-(x*x));
      if(isnan(y)) y=0.0f;

      x_octants[0] = x;
      x_octants[1] = x;
      x_octants[2] = -x;
      x_octants[3] = -x;
      x_octants[4] = y;
      x_octants[5] = y;
      x_octants[6] = -y;
      x_octants[7] = -y;

      y_octants[0] = y;
      y_octants[1] = -y;
      y_octants[2] = y;
      y_octants[3] = -y;
      y_octants[4] = x;
      y_octants[5] = -x;
      y_octants[6] = x;
      y_octants[7] = -x;

      z_octants[0] = z;
      z_octants[1] = z;
      z_octants[2] = z;
      z_octants[3] = z;
      z_octants[4] = z;
      z_octants[5] = z;
      z_octants[6] = z;
      z_octants[7] = z;

      for(uint8_t octant = 0; octant<8; octant++)
      {
        calculate_view_equatorial_x_y_z(x_octants[octant], y_octants[octant], z_octants[octant]);
        calculate_pixel_coords(x_octants[octant], y_octants[octant]);
      }

      if(first_iteration)
      {
        first_iteration = false;
      }
      else
      {
        for(uint8_t octant = 0; octant<8; octant++)
        {
          if(last_z_octants[octant] > 0.0f && z_octants[octant] > 0.0f)
          {
            frame_buffer.draw_line(last_x_octants[octant], last_y_octants[octant], x_octants[octant], y_octants[octant], colour);
          }
        }
      }

      for(uint8_t octant = 0; octant<8; octant++)
      {
        last_x_octants[octant] = x_octants[octant];
        last_y_octants[octant] = y_octants[octant];
        last_z_octants[octant] = z_octants[octant];
      }

      if(x > y) break;
    }
  }

  //plot lines of conastant right ascension
  for(int ra = 0; ra<=170; ra+=10)
  {
    float sin_ra = sin(to_radians(ra));
    float cos_ra = cos(to_radians(ra));
    float y, z;
    float x_octants[8], y_octants[8], z_octants[8];
    float last_x_octants[8], last_y_octants[8], last_z_octants[8];
    bool first_iteration = true;
    for(y=0.0f; y<1.0f; y+=0.177)
    {
      z = sqrt(1.0f-(y*y));

      x_octants[0] = 0;
      x_octants[1] = 0;
      x_octants[2] = 0;
      x_octants[3] = 0;
      x_octants[4] = 0;
      x_octants[5] = 0;
      x_octants[6] = 0;
      x_octants[7] = 0;

      y_octants[0] = y;
      y_octants[1] = -y;
      y_octants[2] = y;
      y_octants[3] = -y;
      y_octants[4] = z;
      y_octants[5] = -z;
      y_octants[6] = z;
      y_octants[7] = -z;

      z_octants[0] = z;
      z_octants[1] = z;
      z_octants[2] = -z;
      z_octants[3] = -z;
      z_octants[4] = y;
      z_octants[5] = y;
      z_octants[6] = -y;
      z_octants[7] = -y;

      for(uint8_t octant = 0; octant<8; octant++)
      {
        float temp_y = y_octants[octant]*cos_ra;
        float temp_x = y_octants[octant]*sin_ra;
        y_octants[octant] = temp_y;
        x_octants[octant] = temp_x;
        calculate_view_equatorial_x_y_z(x_octants[octant], y_octants[octant], z_octants[octant]);
        calculate_pixel_coords(x_octants[octant], y_octants[octant]);
      }

      if(first_iteration)
      {
        first_iteration = false;
      }
      else
      {
        for(uint8_t octant = 0; octant<8; octant++)
        {
          if(last_z_octants[octant] > 0.0f && z_octants[octant] > 0.0f)
          {
            frame_buffer.draw_line(last_x_octants[octant], last_y_octants[octant], x_octants[octant], y_octants[octant], colour);
          }
        }
      }

      for(uint8_t octant = 0; octant<8; octant++)
      {
        last_x_octants[octant] = x_octants[octant];
        last_y_octants[octant] = y_octants[octant];
        last_z_octants[octant] = z_octants[octant];
      }

      if(y > z) break;
    }
  }
}

void c_planetarium :: plot_alt_az_grid(uint16_t colour)
{
  //plot lines of conastant right declination
  for(int alt = 0; alt<90; alt+=10)
  {
    float r = cos(to_radians(alt));
    float z = sin(to_radians(alt));
    float x, y;
    float x_octants[8], y_octants[8], z_octants[8];
    float last_x_octants[8], last_y_octants[8], last_z_octants[8];
    bool first_iteration = true;
    
    for(x=0.0f; x<1.0f; x+=0.177*r)
    {
      y = sqrt((r*r)-(x*x));
      if(isnan(y)) y=0.0f;

      x_octants[0] = x;
      x_octants[1] = x;
      x_octants[2] = -x;
      x_octants[3] = -x;
      x_octants[4] = y;
      x_octants[5] = y;
      x_octants[6] = -y;
      x_octants[7] = -y;

      y_octants[0] = y;
      y_octants[1] = -y;
      y_octants[2] = y;
      y_octants[3] = -y;
      y_octants[4] = x;
      y_octants[5] = -x;
      y_octants[6] = x;
      y_octants[7] = -x;

      z_octants[0] = z;
      z_octants[1] = z;
      z_octants[2] = z;
      z_octants[3] = z;
      z_octants[4] = z;
      z_octants[5] = z;
      z_octants[6] = z;
      z_octants[7] = z;

      for(uint8_t octant = 0; octant<8; octant++)
      {
        calculate_view_horizontal_x_y_z(x_octants[octant], y_octants[octant], z_octants[octant]);
        calculate_pixel_coords(x_octants[octant], y_octants[octant]);
      }

      if(first_iteration)
      {
        first_iteration = false;
      }
      else
      {
        for(uint8_t octant = 0; octant<8; octant++)
        {
          if(last_z_octants[octant] > 0.0f && z_octants[octant] > 0.0f)
          {
            frame_buffer.draw_line(last_x_octants[octant], last_y_octants[octant], x_octants[octant], y_octants[octant], colour);
          }
        }
      }

      for(uint8_t octant = 0; octant<8; octant++)
      {
        last_x_octants[octant] = x_octants[octant];
        last_y_octants[octant] = y_octants[octant];
        last_z_octants[octant] = z_octants[octant];
      }

      if(x > y) break;
    }
  }

  //plot lines of conastant right ascension
  for(int dec = 0; dec<=170; dec+=10)
  {
    float sin_ra = sin(to_radians(dec));
    float cos_ra = cos(to_radians(dec));
    float y, z;
    float x_octants[8], y_octants[8], z_octants[8];
    float last_x_octants[8], last_y_octants[8], last_z_octants[8];
    bool first_iteration = true;
    for(y=0.0f; y<1.0f; y+=0.177)
    {
      z = sqrt(1.0f-(y*y));

      x_octants[0] = 0;
      x_octants[1] = 0;
      x_octants[2] = 0;
      x_octants[3] = 0;
      x_octants[4] = 0;
      x_octants[5] = 0;
      x_octants[6] = 0;
      x_octants[7] = 0;

      y_octants[0] = y;
      y_octants[1] = -y;
      y_octants[2] = y;
      y_octants[3] = -y;
      y_octants[4] = z;
      y_octants[5] = -z;
      y_octants[6] = z;
      y_octants[7] = -z;

      z_octants[0] = z;
      z_octants[1] = z;
      z_octants[2] = -z;
      z_octants[3] = -z;
      z_octants[4] = y;
      z_octants[5] = y;
      z_octants[6] = -y;
      z_octants[7] = -y;

      for(uint8_t octant = 0; octant<8; octant++)
      {
        float temp_y = y_octants[octant]*cos_ra;
        float temp_x = y_octants[octant]*sin_ra;
        y_octants[octant] = temp_y;
        x_octants[octant] = temp_x;
        calculate_view_horizontal_x_y_z(x_octants[octant], y_octants[octant], z_octants[octant]);
        calculate_pixel_coords(x_octants[octant], y_octants[octant]);
      }

      if(first_iteration)
      {
        first_iteration = false;
      }
      else
      {
        for(uint8_t octant = 0; octant<8; octant++)
        {
          if(last_z_octants[octant] > 0.0f && z_octants[octant] > 0.0f)
          {
            frame_buffer.draw_line(last_x_octants[octant], last_y_octants[octant], x_octants[octant], y_octants[octant], colour);
          }
        }
      }

      for(uint8_t octant = 0; octant<8; octant++)
      {
        last_x_octants[octant] = x_octants[octant];
        last_y_octants[octant] = y_octants[octant];
        last_z_octants[octant] = z_octants[octant];
      }

      if(y > z) break;
    }
  }
  
}

void c_planetarium :: plot_planes()
{
  //plot celestial equator
  float az, alt;
  ra_dec_to_alt_az(90, 90, alt, az);
  plot_plane(observer.latitude, 0, frame_buffer.colour565(3, 50, 153));

  //plot ecliptic
  const float orbital_noth_pole_dec = 66.56;
  const float orbital_noth_pole_ra = 270;
  ra_dec_to_alt_az(orbital_noth_pole_ra, orbital_noth_pole_dec, alt, az);
  plot_plane(alt, az, frame_buffer.colour565(135, 0, 57));

}

int rand_range(int min, int max) {
    return min + rand() % (max - min + 1);
}

void c_planetarium :: plot_cardinal_points()
{
  uint16_t colour = frame_buffer.colour565(255, 128, 0);
  for(uint16_t az=0; az < 4*360; az += 45)
  {
    float x, y, z;
    calculate_view_alt_az(0, az/4.0f, x, y, z);
    calculate_pixel_coords(x, y);

    if(x >= 0 && x < width && y >= 0 && y < height && z > -0.01f)
    {
      switch(az){
        case 4*0: frame_buffer.draw_char(x-6, y+5, font_16x12, 'N',    colour); break;
        case 4*45: frame_buffer.draw_string(x-3, y+5, font_8x5, "NE",  colour); break;
        case 4*90: frame_buffer.draw_char(x-6, y+5, font_16x12,'E',    colour); break;
        case 4*135: frame_buffer.draw_string(x-3, y+5, font_8x5, "SE", colour); break;
        case 4*180: frame_buffer.draw_char(x-6, y+5, font_16x12,'S',   colour); break;
        case 4*225: frame_buffer.draw_string(x-3, y+5, font_8x5, "SW", colour); break;
        case 4*270: frame_buffer.draw_char(x-6, y+5, font_16x12,'W',   colour); break;
        case 4*315: frame_buffer.draw_string(x-3, y+5, font_8x5, "NW", colour); break;
      };
      frame_buffer.fill_circle(x, y, 3, colour, 200);
    }
  }
}


void c_planetarium :: plot_milky_way()
{
  
  uint16_t colour = frame_buffer.colour565(255, 255, 255);

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

    calculate_view_alt_az(alt, az, x, y, z);
    calculate_pixel_coords(x, y);

    if((x-last_x)*(x-last_x) + (y-last_y)*(y-last_y) < 100) continue;
    last_x = x; last_y = y;

    if(x >= 0 && x < width && y >= 0 && y < height && z > -0.01f)
    {
      //draw line between 2 points
      uint8_t n = rand_range(15, 30);
      for(uint8_t i=0; i<rand_range(100, 200); i++)
      {
        frame_buffer.fill_circle(x+rand_range(-n, n), y+rand_range(-n, n), rand_range(1, 5), colour, 8);
      }
    }
  }

}

void c_planetarium :: plot_stars()
{

  for(uint16_t idx=0; idx < num_stars; ++idx)
  {

    if(stars[idx].mag > observer.smallest_magnitude) continue;

    float x, y, z;
    x = stars[idx].x; y = stars[idx].y; z = stars[idx].z;
    calculate_view_equatorial_x_y_z(x, y, z);
    calculate_pixel_coords(x, y);

    //don't bother plotting stars outside field of observer
    if(x>width) continue;
    if(y>height) continue;
    if(x<0) continue;
    if(y<0) continue;
    if(z<0) continue;


    int8_t mag = stars[idx].mag;
    uint8_t mk = stars[idx].mk;

    //t0 = micros();
    if(mag <= 1)
    {
      frame_buffer.fill_circle(x, y, 3, star_colour(mk));
    }
    else if(mag <= 2)
    {
      frame_buffer.fill_circle(x, y, 2, star_colour(mk));
    }
    else if(mag <= 3)
    {
      frame_buffer.fill_circle(x, y, 1, star_colour(mk));
    }
    else
    {
      frame_buffer.set_pixel(x, y, star_colour(mk), (256 >> (mag-3)));
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
    float x, y, z;
    calculate_view_ra_dec(ra, dec, x, y, z);

    //don't bother plotting stars outside field of observer
    if(z < 0.0f) continue;

    //get coordinated and magnitude of x
    calculate_pixel_coords(x, y);
    if(x < 0 || x > width || y < 0 || y > height) continue;

    switch(idx)
    {
      case 0: frame_buffer.draw_object(x, y, 4, (uint16_t*)mercury); break;
      case 1: frame_buffer.draw_object(x, y, 4, (uint16_t*)venus); break;
      case 3: frame_buffer.draw_object(x, y, 4, (uint16_t*)mars); break;
      case 4: frame_buffer.draw_object(x, y, 6, (uint16_t*)jupiter); break;
      case 5: 
        frame_buffer.draw_object(x, y, 4, (uint16_t*)saturn); 
        frame_buffer.draw_line(x-6, y-6, x+6, y+6, frame_buffer.colour565(0x49, 0x42, 0x3b));
        break;
      case 6: frame_buffer.draw_object(x, y, 4, (uint16_t*)uranus); break;
      case 7: frame_buffer.draw_object(x, y, 4, (uint16_t*)neptune); break;
    };
    frame_buffer.draw_string(x+4, y-16, font_8x5, planet_names[idx], frame_buffer.colour565(223, 136, 247));

  }

  double ra, dec;
  convert_to_ra_dec(-earth_x, -earth_y, -earth_z, ra, dec);
  float x, y, z;
  calculate_view_ra_dec(ra, dec, x, y, z);

  //don't bother plotting stars outside field of observer
  if(z < 0.0f) return;
  if(x*x + y*y > 0.5) return;

  //get coordinated and magnitude of x
  calculate_pixel_coords(x, y);
  frame_buffer.draw_object(x, y, 10, (uint16_t*)sun);
  frame_buffer.draw_string(x+4, y-16, font_8x5, "Sun", frame_buffer.colour565(223, 136, 247));
}

void c_planetarium :: plot_constellation_names()
{
  uint16_t colour = frame_buffer.colour565(136, 247, 225);
  for(uint16_t idx=0; idx < num_constellations; ++idx)
  {

    float x, y, z;
    calculate_view_ra_dec(constellation_centres[idx].ra*15.0f, constellation_centres[idx].dec, x, y, z);
    calculate_pixel_coords(x, y);

    if(x > width || x < 0 || y > height || y < 0 || z < 0) continue;

    frame_buffer.draw_string(x, y, font_8x5, constellation_names[idx], colour);
    
  }
}

void c_planetarium :: plot_objects()
{
  uint16_t colour = frame_buffer.colour565(0, 175, 201);
  uint16_t text_colour = frame_buffer.colour565(101, 73, 100);
  for(uint16_t idx=0; idx < num_objects; ++idx)
  {

    float x = objects[idx].x;
    float y = objects[idx].y;
    float z = objects[idx].z;
    calculate_view_equatorial_x_y_z(x, y, z);
    calculate_pixel_coords(x, y);

    if(x > width || x < 0 || y > height || y < 0 || z < 0) continue;

    frame_buffer.draw_circle(x, y, 2, colour);
    frame_buffer.draw_string(x, y, font_8x5, objects[idx].name, text_colour);
    
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

uint16_t c_planetarium :: star_colour(float mk)
{
  uint8_t r, g, b;

  //O -> A (0-29) blue->white
  if (mk < 15)
  {
    b = 250;
    r = 180;
    g = 180;
  }
  else if (mk < 29)
  {
    b = 242;
    r = 198;
    g = 204;
  }
  //F -> G (30-49) white->yellow
  else if (mk < 49)
  {
    b = 231;
    r = 251;
    g = 238;
  }
  //K -> M (50-59) yellow->red
  else
  {
    b = 184;
    r = 255;
    g = 200;
  }

  return frame_buffer.colour565(r, g, b);
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

  float x, y, z;
  calculate_view_ra_dec(ra, dec, x, y, z);

  if(abs(x) > 0.5f || abs(y) > 0.5f) return;
  if(z < 0) return;
  calculate_pixel_coords(x, y);
  
  frame_buffer.draw_object(x, y, 10, (uint16_t*)moon);
  frame_buffer.draw_string(x+4, y-16, font_8x5, "Moon", frame_buffer.colour565(223, 136, 247));
}

void c_planetarium :: matrix_multiply(float first_matrix[3][3], float second_matrix[3][3], float result_matrix[3][3]) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result_matrix[i][j] = 0;
            for (int k = 0; k < 3; k++) {
                result_matrix[i][j] += first_matrix[i][k] * second_matrix[k][j];
            }
        }
    }
}

void c_planetarium :: rotate_x_axis(float matrix[3][3], float theta)
{
  matrix[0][0] = 1.0f;
  matrix[0][1] = 0.0f;
  matrix[0][2] = 0.0f;
  matrix[1][0] = 0.0f;
  matrix[1][1] = cos(to_radians(theta));
  matrix[1][2] = -sin(to_radians(theta));
  matrix[2][0] = 0.0f;
  matrix[2][1] = sin(to_radians(theta));
  matrix[2][2] = cos(to_radians(theta));

}

void c_planetarium :: rotate_z_axis(float matrix[3][3], float theta)
{
  matrix[0][0] = cos(to_radians(theta));
  matrix[0][1] = -sin(to_radians(theta));
  matrix[0][2] = 0.0f;
  matrix[1][0] = sin(to_radians(theta));
  matrix[1][1] = cos(to_radians(theta));
  matrix[1][2] = 0.0f;
  matrix[2][0] = 0.0f;
  matrix[2][1] = 0.0f;
  matrix[2][2] = 1.0f;
}

void c_planetarium :: build_rotation_matrix()
{
  //converting from alt-to az coordinates to screen
  //coordinates involes a series of rotations in 2 planes

  //Once the coordinates of the objects are in x, y, z format
  //the screen coordinates can be calculated using a 3x3 rotation
  //matrix. Building the matrix requires trig functions, but only
  //needs to be done once for each update.

  //Applying the rotation matrix needs to be done tens of thousands
  //of times for each update, but only needs multiplies and adds.

  //rotate celestial sphere around Earth's axis (z) depending on the
  //local celestial time.
  float lst_rotation[3][3];
  rotate_z_axis(lst_rotation, lst);

  //rotate around x (W-E) axis depending on the latitude so that
  //the pole appears at the right altitude.
  float lat_rotation[3][3];
  rotate_x_axis(lat_rotation, 90-observer.latitude);

  //rotate around zenith (z) axis depending on the observers view azimuth direction
  float az_rotation[3][3];
  rotate_z_axis(az_rotation, -observer.az);

  //rotate around x (W-E) axis depending on the observers view altitude direction
  float alt_rotation[3][3];
  rotate_x_axis(alt_rotation, -(90-observer.alt));

  //combine all the rotations into a single matrix
  view_rotation_matrix[3][3];
  matrix_multiply(alt_rotation, az_rotation, view_rotation_matrix);
  float lat_rotation_matrix[3][3];
  matrix_multiply(view_rotation_matrix, lat_rotation, lat_rotation_matrix);
  matrix_multiply(lat_rotation_matrix, lst_rotation, rotation_matrix);

}

