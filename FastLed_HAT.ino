#include <FastLED.h>

FASTLED_USING_NAMESPACE

// to enable testing on SK6812 RGBW (unsupported officially)
// #define LED_HACK 1

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#ifdef LED_HACK
  #if defined(__AVR_ATmega2560__)
  #define DATA_PIN    6
  #elif defined(__AVR_ATtiny85__)
  #define DATA_PIN    2
  #else
  #error NO DATA PIN
  #endif
  #define LED_TYPE    SK6812
  #define COLOR_ORDER GRB
  #define NUM_LEDS    16
  #define LED1        0
  #define LED2        12
#else
  #if defined(__AVR_ATmega2560__)
  #define DATA_PIN    6
  #define CLK_PIN     7
  #elif defined(__AVR_ATtiny85__)
  #define DATA_PIN    0
  #define CLK_PIN     2
  #else
  #error NO DATA PIN
  #endif
  #define LED_TYPE    APA102
  #define COLOR_ORDER BGR
  #define NUM_LEDS    2
  #define LED1        0
  #define LED2        1
#endif
CRGB leds[NUM_LEDS];

#define BRIGHTNESS         0xFF
#define FRAMES_PER_SECOND  120

void setup()
{
  //delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
#ifdef LED_HACK
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
#else
  FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(UncorrectedColor);
#endif

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

CRGB gColors[] = {
    CRGB::Red,              // 0 = Red
    CRGB::Blue,             // 1 = Blue
    CHSV(228, 0xFF, 0xFF),  // 2 = Pink
    CRGB::Yellow,           // 3 = Yellow
    CHSV(104, 170,  0xFF),  // 4 = Aquamarine
    CRGB::Amethyst,         // 5 = ???
    CRGB::Lime,             // 6 = ???
    CRGB::White * .6,       // 7 = White
};

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {
    pattern_solid,
    pattern_breathe,
    pattern_flash_slow,
    pattern_flash_fast,
    pattern_alternate,
    pattern_switch,
    pattern_rainbow,
    pattern_rainbow
};

/* = { rainbow,
    full_red, full_blue, full_pink, full_yellow, full_aquamarine, full_white,
    breathe_red, breathe_blue, breathe_pink, 
    alternate_blue_pink, alternate_pink_yellow, alternate_red_blue, alternate_red_white, alternate_orange_aquamarine,
};*/

uint8_t gCurrentColorNumber = 0; // Index number of which color is current
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop()
{
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 10 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 10 ) { nextColor(); } // change patterns periodically
  EVERY_N_SECONDS( 45 ) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
void nextColor()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentColorNumber = (gCurrentColorNumber + 1) % ARRAY_SIZE(gColors);
}
void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void pattern_rainbow()
{
  leds[LED1] = CHSV(gHue, 0xFF, 0xFF);
  leds[LED2] = CHSV(gHue + 0xFF/4, 0xFF, 0xFF);
}

void pattern_solid()   { leds[LED1] = leds[LED2] = gColors[gCurrentColorNumber]; }
void full_red()        { leds[LED1] = leds[LED2] = CRGB::Red; }
void full_blue()       { leds[LED1] = leds[LED2] = CRGB::Blue; }
void full_pink()       { leds[LED1] = leds[LED2] = CHSV(228, 0xFF, 0xFF); }
void full_yellow()     { leds[LED1] = leds[LED2] = CRGB::Yellow; }
void full_aquamarine() { leds[LED1] = leds[LED2] = CHSV(104, 170,  0xFF);/* CRGB::Aquamarine;*/ }
void full_white()      { leds[LED1] = leds[LED2] = CRGB::White * .6; }

void pattern_flash_slow()
{
    leds[LED1] = leds[LED2] = (gHue & 0x80) ? gColors[gCurrentColorNumber] : (CRGB) 0;
}

void pattern_flash_fast()
{
    leds[LED1] = leds[LED2] = (gHue & 0x10) ? gColors[gCurrentColorNumber] : (CRGB) 0;
}

void breathe_hue(CRGB c)
{
  leds[LED1] = leds[LED2] = c;
  leds[LED1].nscale8_video(cubicwave8(gHue));
  leds[LED2].nscale8_video(cubicwave8(gHue));
}

void pattern_breathe() { breathe_hue(gColors[gCurrentColorNumber]); }
void breathe_red()  { breathe_hue(CRGB::Red); }
void breathe_blue() { breathe_hue(CRGB::Blue); }
void breathe_pink() { breathe_hue(CHSV(228, 0xFF, 0xFF)); }

void alternate_2_hues(CRGB c1, CRGB c2)
{
  leds[LED1] = c1;
  leds[LED2] = c2;
  leds[LED1].nscale8(cubicwave8(gHue));
  leds[LED2].nscale8(cubicwave8(gHue + 0xFF/2));
}

void pattern_alternate()           { alternate_2_hues(gColors[gCurrentColorNumber], gColors[(gCurrentColorNumber + 1) % ARRAY_SIZE(gColors)]); }
void alternate_blue_pink()         { alternate_2_hues(CRGB::Blue, CHSV(228, 0xFF, 0xFF)); }
void alternate_pink_yellow()       { alternate_2_hues(CHSV(228, 0xFF, 0xFF), CRGB::Yellow); }
void alternate_red_blue()          { alternate_2_hues(CRGB::Red, CRGB::Blue); }
void alternate_red_white()         { alternate_2_hues(CRGB::Red, CRGB::White); }
void alternate_orange_aquamarine() { alternate_2_hues(CRGB::Orange, CHSV(104, 170,  0xFF)); }

void pattern_switch()
{
    leds[LED1] = (gHue & 0x80) ? gColors[gCurrentColorNumber] : gColors[(gCurrentColorNumber + 1) % ARRAY_SIZE(gColors)];
    leds[LED2] = (gHue & 0x80) ? gColors[(gCurrentColorNumber + 1) % ARRAY_SIZE(gColors)] : gColors[gCurrentColorNumber];
}
