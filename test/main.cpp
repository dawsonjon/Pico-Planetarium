#include "bmp_lib.h"
#include <cstdio>
#include <cstdlib>

class c_bmp_writer_stdio : public c_bmp_writer
{
    bool file_open(const char* filename)
    {
        f = fopen(filename, "wb");
        return f != 0;
    }

    void file_close()
    {
        fclose(f);
    }

    void file_write(const void* data, uint32_t element_size, uint32_t num_elements)
    {
        fwrite(data, element_size, num_elements, f);
    }

    FILE* f;
};

#include "../pico_planetarium/planetarium.h"
#include "../pico_planetarium/frame_buffer.h"

int main()
{

  const uint16_t width = 1280;
  const uint16_t height = 720;
  uint16_t image[height][width];
  c_bmp_writer_stdio output_file;

  s_observer observer =
  {
    .field              = 90.0f,   //float field of view in degrees 
    .alt                = 45.0f,   //float altidute in degrees
    .az                 = 180.0f,  //float azimuth in degrees
    .smallest_magnitude = 8.0f,    //float smallest magnitude star to plot
    
    .latitude           = 51.0,//float latitude - latitude in degrees
    .longitude          = 0.0,  //float longitude - longitude in degrees

    .year  = 2025,
    .month = 4, 
    .day   = 21,
    .hour  = 0, 
    .min   = 0, 
    .sec   = 0,  
  };

  s_settings settings =
  {
    .constellation_lines = true,
    .constellation_names = true,
    .star_names = true,
    .deep_sky_objects = true,
    .deep_sky_object_names = true,
    .planets = true,
    .planet_names = true,
    .moon = true,
    .moon_name = true,
    .sun = true,
    .sun_name = true,
    .celestial_equator = true,
    .ecliptic = true,
    .alt_az_grid = true,
    .ra_dec_grid = true,
  };

  c_frame_buffer frame_buffer((uint16_t*)image, width, height);
  c_planetarium planetarium(frame_buffer, width, height);

  uint16_t count = 0;
  for(uint8_t hour = 0; hour < 24; hour ++)
  {
    for(uint8_t minute = 0; minute < 59; minute +=4)
    {

      observer.hour = hour;
      observer.min = minute;

      char filename[20];
      snprintf(filename, 20, "frame_%03u.bmp", count++);
      output_file.open(filename, width, height);

      planetarium.update(observer, settings);

      for(uint16_t y=0; y<height; y++)
      {
        uint16_t row[width];
        for(uint16_t x=0; x<width; x++)
        {
          uint16_t pixel = image[y][x];
          pixel = ((pixel & 0xff) << 8) | ((pixel & 0xff00) >> 8); 
          row[x] = pixel;
        }
        output_file.write_row_rgb565(row);
      }
      output_file.close();

      printf("%u %u %u\n", count, hour, minute);
      if(hour == 23 && minute == 52) return 0;
    }
  }

  return 0;

}

