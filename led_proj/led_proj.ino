#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Pin and Strip Definitions
#define LED_PIN 6
#define NUM_LED 150
#define BRIGHTNESS 255

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LED, LED_PIN, NEO_GRB + NEO_KHZ800);

// Flag Definitions
#define LT 0
#define US 1

// Preset Color Definitions
uint32_t red = strip.Color(255, 0, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);

uint32_t white = strip.Color(255, 255, 255);
uint32_t black = strip.Color(0, 0, 0);

uint32_t yellow = strip.Color(255, 255, 0);
uint32_t gold = strip.Color(255, 215, 0);
uint32_t dark_green = strip.Color(0, 100, 0);

// Profiles
#define SINGLE 1
#define SNAKE 2
#define FLAG 3
#define RAINBOW 4
#define RAINBOW_CYCLE 5


// Per Profile Color Stuff
uint8_t curr_flag = LT;
uint8_t num_flags = 2;

bool rainbow_snake = false;


// Button Pins
#define BUTTON1_PIN 2   // Profile selector 
#define BUTTON2_PIN 3   // Profile-dependent color selector

// Button Variables
uint8_t reading_profile;            // Current reading of button 1
uint8_t previous_profile = LOW;     // Previous reading of button 1
uint8_t reading_color;              // Current reading of button 2
uint8_t previous_color = LOW;       // Previous reading of button 2

// Time Variables (in ms)
long time_profile = 0;
long time_color = 0;   
long debounce = 200;

// Interrupt Variables  
bool swap_profile = false;
bool swap_color = false;
#define INTERRUPT_WAIT 300
  
// Function counter
uint8_t func_counter = 1; 

////////////////////////////////////////////////////////////////////////////

void setup() {
  // If analog input pin 0 is unconnected, random analog noise will 
  // cause randomSeed() to generate different seed numbers on each run
  randomSeed(analogRead(0));

  // Debug through serial port
  Serial.begin(9600);
  
  // Setup pin mode and initialize
  strip.begin();
  // Set brightness (max 255)
  strip.setBrightness(BRIGHTNESS);
  // Initialize all pixels to 'off'
  strip.show();
  
  // Initialize buttons
  pinMode(BUTTON1_PIN, INPUT);
  pinMode(BUTTON2_PIN, INPUT);
}

void loop() {
  // Handle profile interrupts
  if (swap_profile) {
    swap_profile = false;
    getNextProfile();
  }

  // Handle color interrupts
  if (swap_color) {
    swap_color = false;
    getNextColor();
  }

  // PROFILE SELECTOR
  reading_profile = digitalRead(BUTTON1_PIN);
  
  // Detect signal change and debounce button input
  if (reading_profile == HIGH && previous_profile == LOW && millis() - time_profile > debounce) {
    // Get next profile
    getNextProfile();

    // Save current time
    time_profile = millis();    
  }

  // Save previous state
  previous_profile = reading_profile;
  
 
  // COLOR SELECTOR
  reading_color = digitalRead(BUTTON2_PIN);
  
  // Detect signal change and debounce button input
  if (reading_color == HIGH && previous_color == LOW && millis() - time_color > debounce) {
    // Get next color preset for current profile
    getNextColor();

    // Save current time
    time_color = millis();    
  }

  // Save previous state
  previous_color = reading_color;
}

////////////////////////////////////////////////////////////////////////////

// Fills all LEDs with a single chosen color
void setStripColor(uint32_t c) {
  for (uint16_t i=0; i<NUM_LED; i++)
    strip.setPixelColor(i, c);
  strip.show();
}

// Fills all LEDs with a single random color
void setRandomColor() {
  // Generate random color
  uint32_t rand_color = strip.Color(random(256), random(256), random(256));

  // Assign to all LEDs
  setStripColor(rand_color);
  strip.show();
}

// Fill all LEDs one after the other with a color
void colorSnake(uint32_t c, uint8_t wait) {
  for (uint16_t i=0; i<NUM_LED; i++) {
    // Exit on interrupt
    if (shouldExit()) {
      delay(INTERRUPT_WAIT);
      return;
    }

    // Create random pixel color for rainbow snake color profile
    if (rainbow_snake)
      c = generateRandomColor();
      
    // Set colors accordingly
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);

    // Restart with new random color
    if (i == NUM_LED-1) {
      delay(1000);
      i = 0;
      c = generateRandomColor();
      if (rainbow_snake)
        setStripColor(black);
    }
  }
}

// Some flag representations
void flag(int type) {
  uint16_t j;

  // Lithuanian flag
  if (type == LT) {
    for (j=0; j<NUM_LED/3; j++)
      strip.setPixelColor(j, gold);
    for (   ; j<2*NUM_LED/3; j++)
      strip.setPixelColor(j, dark_green);
    for (   ; j<NUM_LED; j++)
      strip.setPixelColor(j, red);
  }
  // USA flag
  else if (type == US) {
        for (j=0; j<NUM_LED/3; j++)
      strip.setPixelColor(j, red);
    for (   ; j<2*NUM_LED/3; j++)
      strip.setPixelColor(j, white);
    for (   ; j<NUM_LED; j++)
      strip.setPixelColor(j, blue);
  }

  // Show the changes
  strip.show();
}

// Rainbow effect
void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j=0; j<256; j++) {
    // Exit on interrupt
    if (shouldExit()) {
      delay(INTERRUPT_WAIT);
      return;
    }

    // Set colors accordingly
    for (i=0; i<NUM_LED; i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);

    // Restart at end
    if (j == 255 && i == NUM_LED) {
      i = 0;
      j = 0;    
    }
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for (j=0; j<256; j++) {
    // Exit on interrupt
    if (shouldExit()) {
      delay(INTERRUPT_WAIT);
      return;
    }
      
    for (i=0; i<NUM_LED; i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / NUM_LED) + j) & 255));
    }
    strip.show();
    delay(wait);

    // Restart at end
    if (j == 255 && i == NUM_LED) {
      i = 0;
      j = 0;    
    }
  }
}

// Theatre-style crawling lights
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q<3; q++) {
      for (uint16_t i=0; i<NUM_LED; i=i+3) {
        // Turn every third pixel on
        strip.setPixelColor(i+q, c);    
      }
      
      strip.show();
      delay(wait);

      for (uint16_t i=0; i < NUM_LED; i=i+3) {
        // Turn every third pixel off
        strip.setPixelColor(i+q, 0); 
      }
    }
  }
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j<256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q<3; q++) {
      for (uint16_t i=0; i < NUM_LED; i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i<NUM_LED; i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value
// The colours are a transition r - g - b - back to r
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

////////////////////////////////////////////////////////////////////////////

void getNextProfile() {
  // Roll over
  if (func_counter == 6)
    func_counter = 1;
    
  // Cycle through different profiles
  switch (func_counter)
  {
    case SINGLE: setRandomColor(); break;
    case SNAKE: colorSnake(generateRandomColor(), 25); break;
    case FLAG: flag(curr_flag); break;
    case RAINBOW: rainbow(25); break;
    case RAINBOW_CYCLE: rainbowCycle(25); break;
  }
  
  // Increment
  func_counter++;
}

void getNextColor() {
  // Cycle through different color profiles
  switch (func_counter-1)
  {
    case SINGLE: setRandomColor(); break;
    case SNAKE:
      // Reset all LEDs
      setStripColor(black);
      // Switch profile 
      if (!rainbow_snake) {
        rainbow_snake = true;
        colorSnake(black, 25);  
      }
      else
        rainbow_snake = false;
        colorSnake(generateRandomColor(), 25);
      break;
    case FLAG: 
      // Roll over
      curr_flag++;
      if (curr_flag >= num_flags)
        (curr_flag = 0);
      // Set new color
      flag(curr_flag);
      break;
    default: break;
  }
}

bool shouldExit() {
  // Read button input
  uint8_t interrupt_profile = digitalRead(BUTTON1_PIN);
  uint8_t interrupt_color = digitalRead(BUTTON2_PIN);

  // Should exit
  if (interrupt_profile == LOW) {
    swap_profile = true;
    return true;
  }
  else if (interrupt_color == LOW && isColorInterruptable()) {
    swap_color = true;
    return true;
  }

  // Should not exit
  return false;
}

uint32_t generateRandomColor() {
  return strip.Color(random(256), random(256), random(256));  
}

bool isColorInterruptable() {
  return (func_counter == SINGLE || func_counter == SNAKE || func_counter == FLAG);
}


