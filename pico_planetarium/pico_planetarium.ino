#include "planetarium.h"
#include "frame_buffer.h"
#include "images.h"

#include "ctime"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "EEPROM.h"

#include "hardware/spi.h"
#include "ili934x.h"
#include "font_8x5.h"
#include "font_16x12.h"
#include "button.h"

//CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

#define PIN_MISO 12 //not used by TFT but part of SPI bus
#define PIN_CS   13
#define PIN_SCK  14
#define PIN_MOSI 15 
#define PIN_DC   11
#define SPI_PORT spi1

//#define ROTATION R0DEG
#define ROTATION R90DEG
//#define ROTATION R180DEG
//#define ROTATION R270DEG
//#define ROTATION MIRRORED0DEG
//#define ROTATION MIRRORED90DEG
//#define ROTATION MIRRORED180DEG
//#define ROTATION MIRRORED270DEG

//#define INVERT_COLOURS false
#define INVERT_COLOURS true

#define INVERT_DISPLAY false
//#define INVERT_DISPLAY true

//#define DISPLAY_TYPE 0 //ILI934x 320x240 TFT DIsplay
//#define DISPLAY_TYPE 1 //ILI934x (driver 2) 320x240 TFT DIsplay
#define DISPLAY_TYPE 2 //ST7796 480x320 (Needs Pico2!)
//#define DISPLAY_TYPE 3 //ILI9488 480x320 (Needs Pico2!)

//On Pico 2W, use wifi to get the current time
#define USE_NTP_TIME 1 //use credentials defined in ~/credentials.h
#define USE_WIFI_MANAGER 1 //use WIFI Manager to get credentials


//END OF CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

bool use_internet_time = true;
s_observer observer =
{
  .field              = 60.0f,   //float field of view in degrees 
  .alt                = 45.0f,   //float altidute in degrees
  .az                 = 180.0f,  //float azimuth in degrees
  .smallest_magnitude = 6.0f,    //float smallest magnitude star to plot
  
  .latitude           = 51.0,//float latitude - latitude in degrees
  .longitude          = 0.0,  //float longitude - longitude in degrees
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

#if DISPLAY_TYPE == 0
  const uint16_t * splash_image = splash_320x240;
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  uint16_t image[width][height];
  e_display_type display_type = ILI9341;
#elif DISPLAY_TYPE == 1
  const uint16_t * splash_image = splash_320x240;
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  uint16_t image[width][height];
  e_display_type display_type = ILI9341_2;
#elif DISPLAY_TYPE == 2
  const uint16_t * splash_image = splash_480x320;
  const uint16_t width = 480u;
  const uint16_t height = 320u;
  uint16_t image[width][height];
  e_display_type display_type = ST7796;
#else
  const uint16_t * splash_image = splash_480x320;
  const uint16_t width = 480u;
  const uint16_t height = 320u;
  uint16_t image[width][height];
  e_display_type display_type = ILI9488;
#endif

#if USE_NTP_TIME
  #include "NTPTime.h"
  NTPTime ntpTime;
  #if USE_WIFI_MANAGER
    #include "WiFiManager.h"
  #endif
#endif


ILI934X *display;
c_frame_buffer frame_buffer((uint16_t*)image, width, height);
c_planetarium planetarium(frame_buffer, width, height);

void setup() {
  Serial.begin(115200);
  configure_display();
  display->writeImage(0, 0, width, height, splash_image);
  Serial.println("Pico Planetarium (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  #if USE_NTP_TIME
  
    #if USE_WIFI_MANAGER
      display->fillRoundedRect((width - 320)/2, (height-120)/2, 120, 320, 10, COLOUR_DARKGREY);
      display->drawString((width - (18*13))/2, height/2-16, font_16x12, "Connecting to WIFI", COLOUR_WHITE, COLOUR_DARKGREY);
      display->drawString((width - (50*6))/2, height/2+8, font_8x5, "WIFI Setup: ap=PICO_PLANETARIUM, password=password", COLOUR_WHITE, COLOUR_DARKGREY);

      String title("Pico Planetarium");
      String name("PICO_PLANETARIUM_PICO2W");
      String shortname("PICO_PLANETARIUM");
      String maker("101 Things");
      String version("0.0.1");

      WiFiManager wm("PICO_PLANETARIUM", "password");
      wm.setContentText(title, name, shortname, maker, version);
      wm.autoConnect();
    #else
    #include "~/credentials.h"
      Serial.print("Connecting to WiFi...");
      WiFi.begin(ssid, password);
    #endif
    ntpTime.begin();
  
  #else

    //start clock
    tm timeinfo;
    timeinfo.tm_year = 125;
    timeinfo.tm_mon = 1;
    timeinfo.tm_mday = 11;
    timeinfo.tm_hour = 18;
    timeinfo.tm_min = 16;
    timeinfo.tm_sec = 0;
    timeval tv = {.tv_sec = mktime(&timeinfo)};
    settimeofday(&tv, NULL);
  
  #endif

  load_settings(settings);
  load_observer(observer);
}


void loop()
{

  #if USE_NTP_TIME
  if(use_internet_time)
  {
    ntpTime.setTime();
  }
  #endif

  //get time
  time_t now;
  time(&now);
  tm *current_time = gmtime(&now); 
  observer.year  = current_time->tm_year + 1900;
  observer.month = current_time->tm_mon+1; 
  observer.day   = current_time->tm_mday;
  observer.hour  = current_time->tm_hour; 
  observer.min   = current_time->tm_min; 
  observer.sec   = current_time->tm_sec;  
  
  uint32_t start = micros();
  planetarium.update(observer, settings);
  user_interface(frame_buffer, observer, settings, use_internet_time);
  display->writeImage(0, 0, width, height, (uint16_t*)image);
  uint32_t elapsed = micros()-start;
  
  //Serial.println(spi_get_baudrate(SPI_PORT));
  //Serial.println(elapsed/1000);

}

void configure_display()
{
  spi_init(SPI_PORT, 75000000);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_init(PIN_DC);
  gpio_set_dir(PIN_DC, GPIO_OUT);
  display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, width, height);
  display->init(ROTATION, INVERT_COLOURS, INVERT_DISPLAY, display_type);
  display->powerOn(true);
  display->clear();
}

button button_up(17); 
button button_down(20); 
button button_right(21); 
button button_left(22); 

bool number_entry(uint8_t min, uint8_t max, int &value)
{
  if(button_down.is_pressed() || button_down.is_held() && value > min){value--; return true;}
  if(button_up.is_pressed() || button_up.is_held() && value < max){value++; return true;}
  return false;
}

bool bool_entry(bool &value)
{
  if(button_down.is_pressed()){value=!value; return true;}
  if(button_up.is_pressed()){value=!value; return true;}
  return false;
}

bool number_entry(float min, float max, float step, float &value)
{
  if(button_down.is_pressed()) 
  {
    value-=step;
    if(value < min) value = max;
    return true;
  }
  if(button_up.is_pressed())
  {
    value+=step;
    if(value > max) value = min;
    return true;
  } 
  if(button_down.is_held()) 
  {
    value-=(step*10);
    if(value < min) value = max;
    return true;
  }
  if(button_up.is_held())
  {
    value+=(step*10);
    if(value > max) value = min;
    return true;
  }

  return false;
}

void user_interface(c_frame_buffer &frame_buffer, s_observer &observer, s_settings &settings, bool &use_internet_time)
{
  static uint8_t menu_item = 0;
  
  const uint8_t lat = 0;
  const uint8_t lon = 1;
  const uint8_t alt = 2;
  const uint8_t az = 3;
  const uint8_t fov = 4;
  const uint8_t tmode = 5;
  const uint8_t menu = 6;
  const uint16_t year = 7;
  const uint8_t month = 8;
  const uint8_t day = 9;
  const uint8_t hour = 10;
  const uint8_t min = 11;
  const uint8_t second = 12;

  if(button_left.is_pressed() && menu_item > 0) menu_item--;
  if(button_right.is_pressed() && menu_item < second) menu_item++;

  time_t now;
  time(&now);
  tm ct = *gmtime(&now); 

  bool time_changed=false;
  static bool observer_changed = false;
  static uint8_t timeout = 40;
  switch(menu_item)
  {
    case lat: observer_changed |= number_entry(-90.0f, 90.0f, 0.1f, observer.latitude); break;
    case lon: observer_changed |= number_entry(-180.0f, 180.0f, 0.1f, observer.longitude); break;
    case alt: observer_changed |= number_entry(-90.0f, 90.0f, 1.0f, observer.alt); break;
    case az:  observer_changed |= number_entry(0.0f, 360.0f, 1.0f, observer.az); break;
    case fov: observer_changed |= number_entry(0.0f, 180.0f, 1.0f, observer.field); break;
    case tmode: bool_entry(use_internet_time); break;
    case menu: if(button_up.is_pressed()) launch_menu(frame_buffer, observer, settings, use_internet_time); break;
    
    case year:   time_changed |= number_entry(100, 150, ct.tm_year); break;
    case month:  time_changed |= number_entry(0, 11, ct.tm_mon); break;
    case day:    time_changed |= number_entry(1, 31, ct.tm_mday); break;
    case hour:   time_changed |= number_entry(0, 23, ct.tm_hour); break;
    case min:    time_changed |= number_entry(0, 59, ct.tm_min); break;
    case second: time_changed |= number_entry(0, 59, ct.tm_sec); break;
  }

  //use a timeout for observer changes to avoid writing to flash too often
  if(observer_changed)
  {
    if(timeout-- == 0)
    {
      timeout = 40;
      observer_changed = false;
      save_observer(observer);
      Serial.println("writing observer to EEPROM");
    }
  }

  if(!use_internet_time && time_changed)
  {
    timeval tv = {.tv_sec = mktime(&ct)};
    settimeofday(&tv, NULL);
  }
  char buffer[100];

  frame_buffer.fill_rect(0, height-22, width, 22, 0, 128);

  const uint16_t inactive_colour = frame_buffer.colour565(255, 255, 255);
  const uint16_t active_colour = frame_buffer.colour565(255, 0, 0);
  uint16_t colour;

  snprintf(buffer, 100, "lat: %0.1f\x7f", observer.latitude);
  colour = menu_item==lat?active_colour:inactive_colour;
  frame_buffer.draw_string(0, height-20, font_8x5, buffer, colour);

  snprintf(buffer, 100, "lon: %0.1f\x7f", observer.longitude);
  colour = menu_item==lon?active_colour:inactive_colour;
  frame_buffer.draw_string(0, height-10, font_8x5, buffer, colour);

  snprintf(buffer, 100, "alt: %0.0f\x7f", observer.alt);
  colour = menu_item==alt?active_colour:inactive_colour;
  frame_buffer.draw_string(72, height-20, font_8x5, buffer, colour);

  snprintf(buffer, 100, "az: %0.0f\x7f", observer.az);
  colour = menu_item==az?active_colour:inactive_colour;
  frame_buffer.draw_string(72, height-10, font_8x5, buffer, colour);

  if(use_internet_time){
    snprintf(buffer, 100, "NTP");
  } else {
    snprintf(buffer, 100, "Manual");
  }
  colour = menu_item==tmode?active_colour:inactive_colour;
  frame_buffer.draw_string(144, height-10, font_8x5, buffer, colour);

  snprintf(buffer, 100, "fov: %0.0f\x7f", observer.field);
  colour = menu_item==fov?active_colour:inactive_colour;
  frame_buffer.draw_string(144, height-20, font_8x5, buffer, colour);

  colour = menu_item==menu?active_colour:inactive_colour;
  frame_buffer.draw_string(216, height-20, font_8x5, "Menu", colour);
  
  uint16_t x=width-(12*6);

  snprintf(buffer, 100, "UTC %02u", (uint16_t)observer.hour);
  colour = menu_item==hour?active_colour:inactive_colour;
  frame_buffer.draw_string(x, height-20, font_8x5, buffer, colour);
  x+=6*6;
  frame_buffer.draw_string(x, height-20, font_8x5, ":", inactive_colour);
  x+=6;
  snprintf(buffer, 100, "%02u", (uint16_t)observer.min);
  colour = menu_item==min?active_colour:inactive_colour;
  frame_buffer.draw_string(x, height-20, font_8x5, buffer, colour);
  x+=2*6;
  frame_buffer.draw_string(x, height-20, font_8x5, ":", inactive_colour);
  x+=6;
  snprintf(buffer, 100, "%02u", (uint16_t)observer.sec);
  colour = menu_item==second?active_colour:inactive_colour;
  frame_buffer.draw_string(x, height-20, font_8x5, buffer, colour);

  x=width-(10*6);
  snprintf(buffer, 100, "%04u", (uint16_t)observer.year);
  colour = menu_item==year?active_colour:inactive_colour;
  frame_buffer.draw_string(x, height-10, font_8x5, buffer, colour);
  x+=4*6;
  frame_buffer.draw_string(x, height-10, font_8x5, "-", inactive_colour);
  x+=6;
  snprintf(buffer, 100, "%02u", (uint16_t)observer.month);
  colour = menu_item==month?active_colour:inactive_colour;
  frame_buffer.draw_string(x, height-10, font_8x5, buffer, colour);
  x+=2*6;
  frame_buffer.draw_string(x, height-10, font_8x5, "-", inactive_colour);
  x+=6;
  snprintf(buffer, 100, "%02u", (uint16_t)observer.day);
  colour = menu_item==day?active_colour:inactive_colour;
  frame_buffer.draw_string(x, height-10, font_8x5, buffer, colour);

}

void launch_menu(c_frame_buffer &frame_buffer, s_observer &observer, s_settings &settings, bool &use_internet_time)
{
  uint8_t menu_item = 0;
  const uint8_t num_settings = 15;
  const uint8_t num_menu_items = num_settings+2;
  const uint8_t num_items_on_screen = 8;
  uint8_t offset = 0;

  bool settings_array[num_settings] = {
    settings.constellation_lines,
    settings.constellation_names,
    settings.star_names,
    settings.deep_sky_objects,
    settings.deep_sky_object_names,
    settings.planets,
    settings.planet_names,
    settings.moon,
    settings.moon_name,
    settings.sun,
    settings.sun_name,
    settings.celestial_equator,
    settings.ecliptic,
    settings.alt_az_grid,
    settings.ra_dec_grid
  };
  const char* const menu_items[] = {
      "Constellation Lines",
      "Constellation Names",
      "Star Names",
      "Deep Sky Objects",
      "Deep Sky Object Names",
      "Planets",
      "Planet Names",
      "Moon",
      "Moon Name",
      "Sun",
      "Sun Name",
      "Celestial Equator",
      "Ecliptic",
      "ALT/AZ Grid",
      "RA/DEC Grid",
      "Accept",
      "Cancel"
  };

  while(1)
  {

    frame_buffer.fill_rect((width-320)/2, (height-240)/2-5, 320, 240, 0);
    frame_buffer.draw_string((width-(4*12))/2, (height-240)/2, font_16x12, "Menu", frame_buffer.colour565(0, 255, 128));
    frame_buffer.draw_line((width-320)/2, (height-240)/2+30, (width-320)/2+320, (height-240)/2+30, frame_buffer.colour565(0, 255, 128));

    for(uint8_t idx=0; idx < num_items_on_screen; ++idx)
    {
      const uint8_t menu_item_index = idx + offset;
      

      if(menu_item_index < num_settings)
      {
        uint16_t colour = menu_item == menu_item_index?frame_buffer.colour565(255, 0, 255):frame_buffer.colour565(128, 0, 128);
        frame_buffer.draw_string((width-320)/2+3, 40 + ((height-240)/2) + ((idx)*25), font_16x12, menu_items[menu_item_index],  colour);
        if(settings_array[menu_item_index]) frame_buffer.draw_char((width-320)/2+320-16, 40 + ((height-240)/2) + ((idx)*25), font_16x12, 'Y',  colour);
        else frame_buffer.draw_char((width-320)/2+320-16, 40 + ((height-240)/2) + ((idx)*25), font_16x12, 'N',  colour);
      }
      else
      {
        uint16_t colour = menu_item == menu_item_index?frame_buffer.colour565(0, 255, 128):frame_buffer.colour565(0, 128, 64);
        frame_buffer.draw_string((width-strlen(menu_items[menu_item_index])*12)/2, 40 + ((height-240)/2) + ((idx)*25), font_16x12, menu_items[menu_item_index],  colour);
      }    
    }

    if(button_left.is_pressed() && menu_item > 0) menu_item--;
    if(button_right.is_pressed() && menu_item < num_menu_items-1) menu_item++;
    if(button_up.is_pressed() && menu_item < num_settings) settings_array[menu_item] = !settings_array[menu_item];
    if(button_down.is_pressed() && menu_item < num_settings) settings_array[menu_item] = !settings_array[menu_item];
    
    if(menu_item < offset) offset--;
    if(menu_item > offset+num_items_on_screen-1) offset++;

    display->writeImage(0, 0, width, height, (uint16_t*)image);

    if(menu_item == num_menu_items-2 && (button_up.is_pressed()||button_down.is_pressed()))
    {
      settings.constellation_lines=settings_array[0];
      settings.constellation_names=settings_array[1];
      settings.star_names=settings_array[2];
      settings.deep_sky_objects=settings_array[3];
      settings.deep_sky_object_names=settings_array[4];
      settings.planets=settings_array[5];
      settings.planet_names=settings_array[6];
      settings.moon=settings_array[7];
      settings.moon_name=settings_array[8];
      settings.sun=settings_array[9];
      settings.sun_name=settings_array[10];
      settings.celestial_equator=settings_array[11];
      settings.ecliptic=settings_array[12];
      settings.alt_az_grid=settings_array[13];
      settings.ra_dec_grid=settings_array[14];

      save_settings(settings);

      return;//accept
    } 
    if(menu_item == num_menu_items-1 && (button_up.is_pressed()||button_down.is_pressed())) return;//cancel
  }
}

void save_settings(s_settings settings)
{
  //save settings to EEPROM
  EEPROM.put(260, settings);
  uint32_t settings_stored = 0;
  EEPROM.get(256, settings_stored);
  if(settings_stored != 123) EEPROM.put(256, 123);
  EEPROM.commit();
}

void load_settings(s_settings &settings)
{
  //read settings from EEPROM
  uint32_t settings_stored = 0;
  EEPROM.get(256, settings_stored);
  if(settings_stored == 123) EEPROM.get(260, settings);
}

void save_observer(s_observer observer)
{
  //save settings to EEPROM
  EEPROM.put(388, observer);
  uint32_t observer_stored = 0;
  EEPROM.get(384, observer_stored);
  if(observer_stored != 123) EEPROM.put(384, 123);
  EEPROM.commit();
}

void load_observer(s_observer &observer)
{
  //read settings from EEPROM
  uint32_t observer_stored = 0;
  EEPROM.get(384, observer_stored);
  if(observer_stored == 123) EEPROM.get(388, observer);
}
