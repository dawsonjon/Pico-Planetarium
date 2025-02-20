#include "planetarium.h"
#include "frame_buffer.h"
#include "images.h"

#include "ctime"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "hardware/spi.h"
#include "ili934x.h"
#include "font_8x5.h"
#include "font_16x12.h"

//CONFIGURATION SECTION
///////////////////////////////////////////////////////////////////////////////

#define PIN_MISO 12 //not used by TFT but part of SPI bus
#define PIN_CS   13
#define PIN_SCK  14
#define PIN_MOSI 15 
#define PIN_DC   11
#define SPI_PORT spi1

#define ROTATION R0DEG
//#define ROTATION R90DEG
//#define ROTATION R180DEG
//#define ROTATION R270DEG
//#define ROTATION MIRRORED0DEG
//#define ROTATION MIRRORED90DEG
//#define ROTATION MIRRORED180DEG
//#define ROTATION MIRRORED270DEG

#define INVERT_COLOURS false
//#define INVERT_COLOURS true

//#define DISPLAY_TYPE 0 //ILI934x 320x240 TFT DIsplay
#define DISPLAY_TYPE 1 //ST7796 480x320 (Needs Pico2!)

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

#if DISPLAY_TYPE == 0
  uint8_t * splash_image = (uint8_t*)splash_320x240;
  const uint16_t width = 320u;
  const uint16_t height = 240u;
  uint16_t image[width][height];
#else
  uint8_t * splash_image = (uint8_t*)splash_480x320;
  const uint16_t width = 480u;
  const uint16_t height = 320u;
  uint16_t image[width][height];
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
  configure_user_interface();
  display->_writeBlock(0, 0, width-1, height-1, splash_image, width*height*2);
  Serial.println("Pico Planetarium (C) Jonathan P Dawson 2025");
  Serial.println("github: https://github.com/dawsonjon/101Things");
  Serial.println("docs: 101-things.readthedocs.io");

  #if USE_NTP_TIME
  
    #if USE_WIFI_MANAGER
      display->drawString((width - (18*13))/2, height/2-16, font_16x12, "Connecting to WIFI", COLOUR_WHITE, COLOUR_BLACK);
      display->drawString((width - (50*6))/2, height/2+8, font_8x5, "WIFI Setup: ap=PICO_PLANETARIUM, password=password", COLOUR_WHITE, COLOUR_BLACK);

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
}


void loop()
{

  #ifdef USE_NTP_TIME
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
  planetarium.update(observer);
  user_interface(frame_buffer, observer, use_internet_time);
  display->_writeBlock(0, 0, width-1, height-1, (uint8_t*)image, width*height*2);
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
  display = new ILI934X(SPI_PORT, PIN_CS, PIN_DC, width, height, R0DEG, DISPLAY_TYPE);
  display->setRotation(ROTATION, INVERT_COLOURS);
  display->init();
  display->powerOn(true);
  display->clear();
}

const uint8_t button_up = 17;
const uint8_t button_down = 20;
const uint8_t button_right = 21;
const uint8_t button_left = 22;

void configure_user_interface()
{
  pinMode(17, INPUT_PULLUP);
  pinMode(20, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  pinMode(22, INPUT_PULLUP);
}

bool number_entry(uint8_t min, uint8_t max, int &value)
{
  if(digitalRead(button_down) == 0 && value > min){value--; return true;}
  if(digitalRead(button_up) == 0 && value < max){value++; return true;}
  return false;
}

bool bool_entry(bool &value)
{
  if(digitalRead(button_down)==0){value=!value; return true;}
  if(digitalRead(button_up)==0){value=!value; return true;}
  return false;
}

bool number_entry(float min, float max, float step, float &value)
{
  if(digitalRead(button_down) == 0) 
  {
    value-=step;
    if(value < min) value = max;
    return true;
  }
  if(digitalRead(button_up) == 0)
  {
    value+=step;
    if(value > max) value = min;
    return true;
  } 
  return false;
}

void user_interface(c_frame_buffer &frame_buffer, s_observer &observer, bool &use_internet_time)
{
  static uint8_t menu_item = 0;
  
  const uint8_t lat = 0;
  const uint8_t lon = 1;
  const uint8_t alt = 2;
  const uint8_t az = 3;
  const uint8_t fov = 4;
  const uint8_t tmode = 5;
  const uint16_t year = 6;
  const uint8_t month = 7;
  const uint8_t day = 8;
  const uint8_t hour = 9;
  const uint8_t min = 10;
  const uint8_t second = 11;

  if(digitalRead(button_left) == 0 && menu_item > 0) menu_item--;
  if(digitalRead(button_right) == 0 && menu_item < second) menu_item++;

  time_t now;
  time(&now);
  tm ct = *gmtime(&now); 

  bool time_changed=false;
  switch(menu_item)
  {
    case lat: number_entry(-90.0f, 90.0f, 0.1f, observer.latitude); break;
    case lon: number_entry(0.0f, 180.0f, 0.1f, observer.longitude); break;
    case alt: number_entry(-90.0f, 90.0f, 1.0f, observer.alt); break;
    case az:  number_entry(-180.0f, 180.0f, 1.0f, observer.az); break;
    case fov: number_entry(0.0f, 180.0f, 1.0f, observer.field); break;
    case tmode: bool_entry(use_internet_time); break;
    
    case year:   time_changed = number_entry(100, 150, ct.tm_year); break;
    case month:  time_changed = number_entry(0, 11, ct.tm_mon); break;
    case day:    time_changed = number_entry(1, 31, ct.tm_mday); break;
    case hour:   time_changed = number_entry(0, 23, ct.tm_hour); break;
    case min:    time_changed = number_entry(0, 59, ct.tm_min); break;
    case second: time_changed = number_entry(0, 59, ct.tm_sec); break;
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
  
  uint16_t x=width-(8*6);

  snprintf(buffer, 100, "%02u", (uint16_t)observer.hour);
  colour = menu_item==hour?active_colour:inactive_colour;
  frame_buffer.draw_string(x, height-20, font_8x5, buffer, colour);
  x+=2*6;
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
