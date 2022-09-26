#include <FastLED.h>

FASTLED_USING_NAMESPACE
#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

/**
 * Connections on Adafruit Gemma v2:
 * 
 *  – D0 = APA102 LED CLOCK
 *  – D1 = APA102 LED DATA
 *          (+ D1 LED on board, hence the use for data i/o clock,
 *          because at startup time the pin is used for PWM, which
 *          is then interpreted as a stream of ones by the LEDs…)
 *  – D2/A1 = ADC IN for the two buttons (see below).
 *  – GND connects to the LEDs, and the buttons.
 *  – 3VOUT connects to the LEDs, and through a same-value resistor,
 *          to D2/A1.
 */
#if defined(ARDUINO_AVR_GEMMA) or defined(ARDUINO_TRINKET_M0) or defined(ARDUINO_GEMMA_M0) or defined(ARDUINO_AVR_ATTINYX5)
    #define DATA_PIN        0
    #define BTN_PIN         A1
#else
    #error NO DATA PIN
#endif
#define LED_TYPE            WS2812B
#define COLOR_ORDER         RGB

#define BRIGHTNESS          0xFF/4
#define FRAMES_PER_SECOND   120

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

/** LEDs **/
#define NUM_LEDS_RGB        31
#define NUM_LEDS_RGBW       (7+8)
#define NUM_LEDS_RGBW_DUP   (8)
#define NUM_LEDS            NUM_LEDS_RGBW

CRGB leds_rgb[NUM_LEDS_RGB];
CRGB leds[NUM_LEDS_RGBW];

/** Patterns **/
void pattern_rainbow();
void pattern_solid();
void pattern_flash_slow();
void pattern_flash_fast();
void pattern_breathe();
void pattern_weave();
void pattern_switch();
void pattern_switch_fast();
void pattern_none();

void (*gPatterns[9])() = {
    pattern_rainbow,        // 0 = rainbooowww ♥️🌈
    pattern_solid,          // 1 = solid color
    pattern_breathe,        // 2 = breathing pattern (cubic wave)
    pattern_flash_slow,     // 3 = flash, slowly
    pattern_flash_fast,     // 4 = flash, faster
    pattern_weave,          // 5 = alternate between two hues (2 x cubic wave, antiphase)
    pattern_switch,         // 6 = alternate between two hues, flashing from one to the next
    pattern_switch_fast,    // 7 = alternate between two hues, flashing from one to the next, faster.
    pattern_none            // 8 = off
};

/** Colors **/
const CRGB gColors[8] = {
    CRGB::Blue,             // 1 = Blue
    CRGB::Purple,           // 2 = Pink
    CRGB::Yellow,           // 3 = Yellow
   (CRGB) 0x1CB82A,         // 4 = Aquamarine, CHSV(104, 170, 0xFF)
   (CRGB) 0x1C439F,         // 5 = Alice Blue, CHSV(149, 170, 0xFF)
    CRGB::Lime,             // 6 = Lime
    CRGB::White,            // 7 = White
    CRGB::Red,              // 0 = Red
};

/** Current state **/
struct {
    uint8_t pattern = 0;    // index number of which pattern is current
    uint8_t color   = 0;    // index number of which color is current
    uint8_t hue     = 0;    // rotating "base color" used by many of the patterns
    uint8_t buttons = 0;    // input buttons state
} gState;

/** Convert from RGB to RGBW (8 bit version) **/
static inline void rgb_to_rgbw()
{
    uint8_t *lrgb8 = (uint8_t *) leds_rgb;
    for (uint8_t i = 1 ; i < NUM_LEDS_RGBW ; i++)
    {
        lrgb8[i*4+0] = leds[i].g;
        lrgb8[i*4+1] = leds[i].r;
        lrgb8[i*4+2] = leds[i].b;
    }
    for (uint8_t i = 0 ; i < NUM_LEDS_RGBW_DUP ; i++)
    {
        lrgb8[(NUM_LEDS_RGBW+NUM_LEDS_RGBW_DUP-i)*4+0] = leds[NUM_LEDS_RGBW-NUM_LEDS_RGBW_DUP+i].g;
        lrgb8[(NUM_LEDS_RGBW+NUM_LEDS_RGBW_DUP-i)*4+1] = leds[NUM_LEDS_RGBW-NUM_LEDS_RGBW_DUP+i].r;
        lrgb8[(NUM_LEDS_RGBW+NUM_LEDS_RGBW_DUP-i)*4+2] = leds[NUM_LEDS_RGBW-NUM_LEDS_RGBW_DUP+i].b;
    }
}

/******************************************************************************/
/*                                                                            */
/*                                 BUTTONS                                    */
/*                                                                            */
/******************************************************************************/
static inline void nextColor()
{
    // add one to the current color number, and wrap around at the end
    gState.color = (gState.color + 1) % ARRAY_SIZE(gColors);
    gState.hue = 0;
}

static inline void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gState.pattern = (gState.pattern + 1) % ARRAY_SIZE(gPatterns);
    gState.hue = 0;
}

/**
 * Reading two buttons from one ADC pin:
 * 
 *          +3V3
 *
 *            +
 *            |
 *           +-+
 *           | |
 *     100k  | |
 *           | |
 *           +-+
 *            |
 *            +-----------+  ADC IN
 *            |
 *           +-+
 *           | |
 *     100k  | |
 *           | |
 *           +-+
 *            |
 *            +-----------+
 *            |           |
 *            |          +-+
 *            |          | |
 *            |    100k  | |
 *            |          | |
 *            |          +-+
 *            |           |
 *            \           \
 *       K1    \     K2    \
 *              \           \
 *            |           |
 *            +-----------+
 *            |
 *            +
 *
 *           GND
 */
static inline void readButtons()
{
    // read the analog in value:
    uint16_t sensorValue = analogRead(BTN_PIN);

    // determine the button pressed (or lack thereof)
    uint8_t new_state = 0xFF;
    if (sensorValue > 1013) // Nominally, 0x3FF (1023)
    {
        new_state = 0;
    }
    else if (sensorValue > 502 && sensorValue < 522) // nom: 0x1FF (511)
    {
        new_state = 1;
    }
    else if (sensorValue > 672 && sensorValue < 692) // nom: 0x2AA (682)
    {
        new_state = 2;
    }

    // is it a new state? then, react accordingly.
    if (new_state != 0xFF && new_state != gState.buttons)
    {
        if (new_state == 1)
        {
            nextColor();
        }
        else if (new_state == 2)
        {
            nextPattern();
        }
        gState.buttons = new_state;
    }
}

/******************************************************************************/
/*                                                                            */
/*                               PATTERNS                                     */
/*                                                                            */
/******************************************************************************/
void pattern_rainbow()
{
    fill_rainbow(leds, NUM_LEDS, (-gState.hue) << 2, 32);
}

void pattern_solid()
{
    fill_solid(leds, NUM_LEDS, gColors[gState.color]);
}

void pattern_flash_slow()
{
    fill_solid(leds, NUM_LEDS, (gState.hue & 0x80) ? (CRGB) 0: gColors[gState.color]);
}

void pattern_flash_fast()
{
    fill_solid(leds, NUM_LEDS, (gState.hue & 0x10) ? (CRGB) 0: gColors[gState.color]);
}

void pattern_breathe()
{
    //*
    fill_solid(leds, NUM_LEDS, gColors[gState.color]);
    nscale8_video(leds, NUM_LEDS, cubicwave8(gState.hue));
    //*/
    /*
    leds[0] = gColors[gState.color];
    leds[0].nscale8_video(cubicwave8(gState.hue));   
    for (uint8_t i = 1 ; i < NUM_LEDS ; i++)
    {
        leds[i] = leds[0];
    }
    //*/
}

void pattern_weave()
{
    CRGB start = gColors[gState.color];
    CRGB end = gColors[(gState.color + 1) % ARRAY_SIZE(gColors)];
    CRGB start2 = blend(start, end, cubicwave8(gState.hue));
    CRGB end2 = blend(start, end, cubicwave8(gState.hue + 0xFF/4));
    fill_gradient_RGB(leds, NUM_LEDS, end2, start2);
}

void pattern_switch()
{
    for (uint8_t i = 0 ; i < NUM_LEDS ; i++)
    {
        leds[i] = (gState.hue & 0x80) ? gColors[(gState.color + 1) % ARRAY_SIZE(gColors)] : gColors[gState.color];
    }
}

void pattern_switch_fast()
{
    for (uint8_t i = 0 ; i < NUM_LEDS ; i++)
    {
        leds[i] = (gState.hue & 0x10) ? gColors[(gState.color + 1) % ARRAY_SIZE(gColors)] : gColors[gState.color];
    }
}

void pattern_none()
{
    fill_solid(leds, NUM_LEDS, 0);
}

/******************************************************************************/
/*                                                                            */
/*                                 SETUP                                      */
/*                                                                            */
/******************************************************************************/
void setup()
{
    //delay(3000); // 3 second delay for recovery

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds_rgb, NUM_LEDS_RGB).setCorrection(UncorrectedColor);

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
    gPatterns[gState.pattern]();

    // send the 'leds' array out to the actual LED strip
    rgb_to_rgbw();
    FastLED.show();
    
    // insert a delay to keep the framerate modest
    FastLED.delay(1000/FRAMES_PER_SECOND);

    // do some periodic updates
    EVERY_N_MILLISECONDS(10)
    {
        // slowly cycle the "base color" through the rainbow
        gState.hue++;

        // read the physical buttons
        readButtons();
    }
}
