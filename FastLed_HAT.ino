#include <FastLED.h>

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#if defined(__AVR_ATtiny85__)
    #define DATA_PIN        1
    #define CLK_PIN         0
    #define BTN_PIN         A1
#else
    #error NO DATA PIN
#endif
#define LED_TYPE            APA102
#define COLOR_ORDER         BGR
#define NUM_LEDS            2
#define LED1                0
#define LED2                1

#define BRIGHTNESS          0xFF
#define FRAMES_PER_SECOND   120

/** LEDs **/
CRGB leds[NUM_LEDS];

/** Patterns **/
void pattern_rainbow();
void pattern_solid();
void pattern_flash_slow();
void pattern_flash_fast();
void pattern_breathe();
void pattern_weave();
void pattern_switch();

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

/** Colors **/
typedef CRGB SimpleColorList[];
SimpleColorList gColors = {
    CRGB::Red,              // 0 = Red
    CRGB::Blue,             // 1 = Blue
    CHSV(228, 0xFF, 0xFF),  // 2 = Pink (MediumVioletRed?)
    CRGB::Yellow,           // 3 = Yellow
    CHSV(104, 170,  0xFF),  // 4 = Aquamarine
    CRGB::Purple,           // 5 = ???
    CRGB::Lime,             // 6 = ???
    CRGB::White * .6,       // 7 = White
};

uint8_t gCurrentColorNumber = 0;    // Index number of which color is current
uint8_t gCurrentPatternNumber = 0;  // Index number of which pattern is current
uint8_t gHue = 0;                   // rotating "base color" used by many of the patterns
uint8_t gLastState = 0;             // input buttons state

/******************************************************************************/
/*                                                                            */
/*                                 SETUP                                      */
/*                                                                            */
/******************************************************************************/
void setup()
{
    //delay(3000); // 3 second delay for recovery

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(UncorrectedColor);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);
}

/******************************************************************************/
/*                                                                            */
/*                                  LOOP                                      */
/*                                                                            */
/******************************************************************************/
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
        
        // read the physical buttons
        readButtons();
    }
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
static inline void nextColor()
{
    // add one to the current pattern number, and wrap around at the end
    gCurrentColorNumber = (gCurrentColorNumber + 1) % ARRAY_SIZE(gColors);
}

static inline void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

static inline void readButtons()
{
    // read the analog in value:
    uint16_t sensorValue = analogRead(BTN_PIN);

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

    if (new_state != 0xFF && new_state != gLastState)
    {
        if (new_state == 1)
        {
            nextColor();
        }
        else if (new_state == 2)
        {
            nextPattern();
        }
        gLastState = new_state;
    }
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
