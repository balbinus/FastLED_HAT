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
        #define DATA_PIN    1
        #define CLK_PIN     0
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

/** Patterns **/
void pattern_rainbow();
void pattern_solid();
void pattern_flash_slow();
void pattern_flash_fast();
void pattern_breathe();
void pattern_weave();
void pattern_switch();

CRGB gColors[] = {
    CRGB::Red,              // 0 = Red
    CRGB::Blue,             // 1 = Blue
    CHSV(228, 0xFF, 0xFF),  // 2 = Pink (MediumVioletRed?)
    CRGB::Yellow,           // 3 = Yellow
    CHSV(104, 170,  0xFF),  // 4 = Aquamarine
    CRGB::Purple,           // 5 = ???
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
    pattern_weave,
    pattern_switch,
    pattern_rainbow,
    pattern_rainbow
};

uint8_t gCurrentColorNumber = 0; // Index number of which color is current
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

/******************************************************************************/
/*                                                                            */
/*                                 SETUP                                      */
/*                                                                            */
/******************************************************************************/
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

/******************************************************************************/
/*                                                                            */
/*                                  LOOP                                      */
/*                                                                            */
/******************************************************************************/
uint8_t last_state = 0;
void loop()
{
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();

    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND); 

    // do some periodic updates
    EVERY_N_MILLISECONDS(10)
    {
        // slowly cycle the "base color" through the rainbow
        gHue++;
        
        // read the analog in value:
        uint16_t sensorValue = analogRead(A1);

        uint8_t new_state = 0xFF;
        if (sensorValue > 1013)
        {
            new_state = 0;
        }
        else if (sensorValue > 502 && sensorValue < 522)
        {
            new_state = 1;
        }
        else if (sensorValue > 672 && sensorValue < 692)
        {
            new_state = 2;
        }

        if (new_state != 0xFF && new_state != last_state)
        {
            if (new_state == 1)
            {
                nextColor();
            }
            else if (new_state == 2)
            {
                nextPattern();
            }
            last_state = new_state;
        }
    }
    //~ EVERY_N_SECONDS(10)      { nextColor(); } // change patterns periodically
    //~ EVERY_N_SECONDS(45)      { nextPattern(); } // change patterns periodically
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

/******************************************************************************/
/*                                                                            */
/*                               PATTERNS                                     */
/*                                                                            */
/******************************************************************************/
void pattern_rainbow()
{
    leds[LED1] = CHSV(gHue, 0xFF, 0xFF);
    leds[LED2] = CHSV(gHue + 0xFF/4, 0xFF, 0xFF);
}

void pattern_solid()
{
    leds[LED1] = leds[LED2] = gColors[gCurrentColorNumber];
}

void pattern_flash_slow()
{
    leds[LED1] = leds[LED2] = (gHue & 0x80) ? gColors[gCurrentColorNumber] : (CRGB) 0;
}

void pattern_flash_fast()
{
    leds[LED1] = leds[LED2] = (gHue & 0x10) ? gColors[gCurrentColorNumber] : (CRGB) 0;
}

void pattern_breathe()
{
    leds[LED1] = leds[LED2] = gColors[gCurrentColorNumber];
    leds[LED1].nscale8_video(cubicwave8(gHue));
    leds[LED2].nscale8_video(cubicwave8(gHue));
}

void pattern_weave()
{
    leds[LED1] = gColors[gCurrentColorNumber];
    leds[LED2] = gColors[(gCurrentColorNumber + 1) % ARRAY_SIZE(gColors)];
    leds[LED1].nscale8(cubicwave8(gHue));
    leds[LED2].nscale8(cubicwave8(gHue + 0xFF/2));
}

void pattern_switch()
{
    leds[LED1] = leds[LED2] = (gHue & 0x80) ? gColors[gCurrentColorNumber] : gColors[(gCurrentColorNumber + 1) % ARRAY_SIZE(gColors)];
}
