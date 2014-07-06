/*

  .ino
  http://www.semifluid.com
  
  Based upon httpclient.ino from:
  https://github.com/harlequin-tech/WiFlyHQ/blob/master/examples/httpclient/httpclient.ino
  and Console.pde from:
  ================
  Universal 8bit Graphics Library, http://code.google.com/p/u8glib/
  
  Copyright (c) 2011, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
  ================
*/

#include <WiFlyHQ.h>
#include <SoftwareSerial.h>
SoftwareSerial debugSerial(2,3);

WiFly WiFly;

// External SECRET.ino file with the network details:
/* Change these to match your WiFi network */
//const char mySSID[] = "SSID";
//const char myPassword[] = "PASSWORD";
extern const char mySSID[];
extern const char myPassword[];

#include <U8glib.h>

byte ledBacklight = 100;
const byte lcdLED = 6;                   // LED Backlight
const byte lcdA0 = 7;                    // Data and command selections. L: command  H : data
const byte lcdRESET = 8;                 // Low reset
const byte lcdCS = 9;                    // SPI Chip Select (internally pulled up), active low
const byte lcdMOSI = 11;                 // SPI Data transmission
const byte lcdSCK = 13;                  // SPI Serial Clock

const byte bufferSize = 32;

// SW SPI:
//U8GLIB_MINI12864_2X u8g(lcdSCK, lcdMOSI, lcdCS, lcdA0, lcdRESET);
// HW SPI:
U8GLIB_MINI12864_2X u8g(lcdCS, lcdA0, lcdRESET);

const byte LINE_MAX = 30;          // setup input buffer
byte line_buf[LINE_MAX];
byte line_pos = 0;
const byte ROW_MAX = 12;           // setup a text screen to support scrolling
byte screen[ROW_MAX][LINE_MAX];
byte rows, cols;
const byte LINE_PIXEL_HEIGHT = 7;  // line height, which matches the selected font (5x7)

// Print character arrays to console
void u8g_print(char* theString) {
  char ch;
  byte i = 0;
  while ((ch = theString[i++]) != '\0') {
	read_line(ch);
    }
}
// Print character arrays with a new line at the end
void u8g_println(char* theString) {
  u8g_print(theString);
  read_line('\r');
}
// Print character arrays saved in program memory to console
void u8g_print_P(const prog_char *theString) {
  char ch;
  while ((ch = pgm_read_byte(theString++)) != '\0') {
	read_line(ch);
    }
}
// Print character arrays saved in program memory with a new line at the end
void u8g_println_P(const prog_char *theString) {
  u8g_print_P(theString);
  read_line('\r');
}

// Clear entire screen, called during setup
void clear_screen(void) {
  byte i, j;
  for( i = 0; i < ROW_MAX; i++ )
    for( j = 0; j < LINE_MAX; j++ )
      screen[i][j] = 0;  
}
// Append a line to the screen, scroll up
void add_line_to_screen(void) {
  byte i, j;
  for( j = 0; j < LINE_MAX; j++ )
    for( i = 0; i < rows-1; i++ )
      screen[i][j] = screen[i+1][j];
  
  for( j = 0; j < LINE_MAX; j++ )
    screen[rows-1][j] = line_buf[j];
}
// U8GLIB draw procedure: output the screen
void draw(void) {
  byte i, y;
  // graphic commands to redraw the complete screen are placed here    
  y = 0;       // reference is the top left -1 position of the string
  y--;           // correct the -1 position of the drawStr 
  for( i = 0; i < rows; i++ )
  {
    u8g.drawStr( 0, y, (char *)(screen[i]));
    y += u8g.getFontLineSpacing();
  }
}
void exec_line(void) {
  // Add the line to the screen
  add_line_to_screen();
  // U8GLIB picture loop
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
}
// Clear current input buffer
void reset_line(void) { 
      line_pos = 0;
      line_buf[line_pos] = '\0';  
}
// Add a single character to the input buffer 
void char_to_line(char c) {
      line_buf[line_pos] = c;
      line_pos++;
      line_buf[line_pos] = '\0';  
}
// Handle new console character
void read_line(char c) {
  if ( line_pos >= cols-1 ) {
    exec_line();
    reset_line();
    char_to_line(c);
  } 
  else if ( c == '\n' ) {
    // ignore '\n' 
  }
  else if ( c == '\r' ) {
    exec_line();
    reset_line();
  }
  else {
    char_to_line(c);
  }
}

// From http://playground.arduino.cc/Code/AvailableMemory
int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup()
{
    char buf[bufferSize];

    // Turn on LED backlight
    analogWrite(lcdLED, ledBacklight);

    // Setup U8glib
    u8g.begin();
    u8g.setFont(u8g_font_5x7);    // Set font for the console window
    u8g.setFontPosTop();          // Set upper left position for the string draw procedure
    // Calculate the number of rows for the display
    rows = u8g.getHeight() / u8g.getFontLineSpacing();
    if ( rows > ROW_MAX )
      rows = ROW_MAX;
    // Estimate the number of columns for the display
    cols = u8g.getWidth() / u8g.getStrWidth("m");
    if ( cols > LINE_MAX-1 )
      cols = LINE_MAX-1;
    clear_screen();               // Clear screen
    delay(1000);                  // Delay
    reset_line();                 // Clear input buffer
    
    // Print some information
    u8g_println_P(PSTR("WiFlyTimeWithConsole.ino"));
    u8g_println_P(PSTR("semifluid.com"));
    u8g_println_P(PSTR(""));
    u8g_println_P(PSTR("Starting..."));
    u8g_print_P(PSTR("Free Memory: "));
    String(freeRam(),DEC).toCharArray(buf, sizeof(buf));u8g_println(buf);
    
    Serial.begin(9600);
    
    while (!WiFly.begin(&Serial, &debugSerial)) {
        // Failed to start WiFly
        u8g_println_P(PSTR("Failed to start WiFly"));
    }
    
    /* Join wifi network if not already associated */
    if (!WiFly.isAssociated()) {
	/* Setup the WiFly to connect to a wifi network */
        // Joining network
	WiFly.setSSID(mySSID);
	WiFly.setPassphrase(myPassword);
	WiFly.enableDHCP();

	if (WiFly.join()) {
            // Joined wifi network
            u8g_println_P(PSTR("Joined wifi network"));
	} else {
            // Failed to join wifi network
            u8g_println_P(PSTR("Failed to join wifi network"));
	}
    } else {
        // Already joined network
        u8g_println_P(PSTR("Already joined network"));
    }
    
    WiFly.setDeviceID("WiFly");
    
    u8g_println_P(PSTR("SSID: "));
    WiFly.getSSID(buf, sizeof(buf));u8g_println(buf);
    u8g_print_P(PSTR("DeviceID: "));
    WiFly.getDeviceID(buf, sizeof(buf));u8g_println(buf);
    u8g_print_P(PSTR("IP: "));
    WiFly.getIP(buf, sizeof(buf));u8g_println(buf);
    u8g_print_P(PSTR("Netmask: "));
    WiFly.getNetmask(buf, sizeof(buf));u8g_println(buf);
    u8g_print_P(PSTR("Gateway: "));
    WiFly.getGateway(buf, sizeof(buf));u8g_println(buf);
    u8g_print_P(PSTR("MAC: "));
    WiFly.getMAC(buf, sizeof(buf));u8g_println(buf);

    WiFly.setTimeAddress("129.6.15.30"); // time-c.nist.gov
    WiFly.setTimePort(123);
    WiFly.setTimezone(22);  // CEST UTC + 2 hours
    WiFly.setTimeEnable(5);
    WiFly.time();
    
    if (WiFly.isConnected()) {
        // Old connection active. Closing
        u8g_println_P(PSTR("Old connection active. Closing..."));
	WiFly.close();
    }
}

void loop()
{
    char buf[bufferSize];
    
    // Get the time off the WiFly and print to GLCD
    WiFly.getTime(buf, sizeof(buf));u8g_print(buf);
    u8g_print_P(PSTR(" -- RSSI: "));
    // Get the WiFly's signal strength (RSSI) and print to GLCD
    String(WiFly.getRSSI(), DEC).toCharArray(buf, sizeof(buf));u8g_println(buf);
  
    delay(2000);
}
