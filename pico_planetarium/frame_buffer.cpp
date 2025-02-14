#include <cmath>
#include "frame_buffer.h"

void c_frame_buffer :: set_pixel(uint16_t x, uint16_t y, uint16_t colour, uint16_t alpha)
{
  if(x>=m_width) return;
  if(y>=m_height) return;
  uint16_t old_colour = m_buffer[y*m_width + x];
  m_buffer[y*m_width + x] = alpha_blend(old_colour, colour, alpha);
}

void c_frame_buffer :: draw_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t colour, uint16_t alpha)
{
    bool one_point_in_view = 
      (x1 >= 0 && x1 < m_width && y1 >= 0 && y1 < m_height) || 
      (x2 >= 0 && x2 < m_width && y2 >= 0 && y2 < m_height);
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

void c_frame_buffer::draw_string(uint16_t x, uint16_t y, const uint8_t *font, const char *s, uint16_t fg, uint16_t alpha) 
{
  const uint8_t font_width  = font[1];
  const uint8_t font_space  = font[2];
  for(int32_t x_n=x; *s; x_n+=(font_width+font_space)) {
      draw_char(x_n, y, font, *(s++), fg, alpha);
  }
}

void c_frame_buffer::draw_char(uint16_t x, uint16_t y, const uint8_t *font, char c, uint16_t fg, uint16_t alpha) 
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

void c_frame_buffer::fill_circle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour, uint16_t alpha)
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

void c_frame_buffer :: draw_circle(uint16_t xc, uint16_t yc, uint16_t radius, uint16_t colour, uint16_t alpha) 
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

uint16_t c_frame_buffer::colour565(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t val = (((r >> 3) & 0x1f) << 11) | (((g >> 2) & 0x3f) << 5) | ((b >> 3) & 0x1f);
    return (val >> 8) | (val << 8);
}

void c_frame_buffer::colour_rgb(uint16_t colour_565, uint8_t &r, uint8_t &g, uint8_t &b)
{
    colour_565 = (colour_565 >> 8) | (colour_565 << 8);
    r = ((colour_565 >> 11) & 0x1f) << 3;
    g = ((colour_565 >> 5) & 0x3f) << 2;
    b = (colour_565 & 0x1f) << 3;
}

uint16_t c_frame_buffer::colour_scale(uint16_t colour, uint16_t alpha)
{
    uint8_t r, g, b;
    colour_rgb(colour, r, g, b);
    r=std::min((r*alpha)>>8, 255);
    g=std::min((g*alpha)>>8, 255);
    b=std::min((b*alpha)>>8, 255);
    return colour565(r, g, b);
}

uint16_t c_frame_buffer::alpha_blend(uint16_t old_colour, uint16_t colour, uint16_t alpha)
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

void c_frame_buffer :: fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t colour, uint16_t alpha)
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

void c_frame_buffer :: draw_object(uint16_t x, uint16_t y, uint16_t r, uint16_t* image)
{
  for(int16_t xx = -r; xx<r; xx++)
  {
    for(int16_t yy = -r; yy<r; yy++)
    {
      if((xx*xx+yy*yy)>(r*r)) continue;
      set_pixel(x+xx, y+yy, image[((yy+r)*2*r)+xx+r]);
    }
  }
}
