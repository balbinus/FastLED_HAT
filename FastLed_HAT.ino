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
 *           is then interpreted as a stream of ones by the LEDs…)
 *  – D2/A1 = ADC IN for the two buttons (see below).
 *  – GND connects to the LEDs, and the buttons.
 *  – 3VOUT connects to the LEDs, and through a same-value resistor,
 *          to D2/A1.
 */
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
    pattern_solid,          // 0 = solid color
    pattern_breathe,        // 1 = breathing pattern (cubic wave)
    pattern_flash_slow,     // 2 = flash, slowly
    pattern_flash_fast,     // 3 = flash, faster
    pattern_weave,          // 4 = alternate between two hues (2 x cubic wave, antiphase)
    pattern_switch,         // 5 = alternate between two hues, flashing from one to the next
    pattern_rainbow,        // 6 = ???
    pattern_rainbow         // 7 = rainbooowww ♥️🌈
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

/** Current state **/
struct {
    uint8_t color   = 0;    // index number of which color is current
    uint8_t pattern = 0;    // index number of which pattern is current
    uint8_t hue     = 0;    // rotating "base color" used by many of the patterns
    uint8_t buttons = 0;    // input buttons state
} gState;

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
    gPatterns[gState.pattern]();

    // send the 'leds' array out to the actual LED strip
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

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
static inline void nextColor()
{
    // add one to the current pattern number, and wrap around at the end
    gState.color = (gState.color + 1) % ARRAY_SIZE(gColors);
}

static inline void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gState.pattern = (gState.pattern + 1) % ARRAY_SIZE(gPatterns);
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
    leds[LED1] = CHSV(gState.hue, 0xFF, 0xFF);
    leds[LED2] = CHSV(gState.hue + 0xFF/4, 0xFF, 0xFF);
}

void pattern_solid()
{
    leds[LED1] = leds[LED2] = gColors[gState.color];
}

void pattern_flash_slow()
{
    leds[LED1] = leds[LED2] = (gState.hue & 0x80) ? gColors[gState.color] : (CRGB) 0;
}

void pattern_flash_fast()
{
    leds[LED1] = leds[LED2] = (gState.hue & 0x10) ? gColors[gState.color] : (CRGB) 0;
}

void pattern_breathe()
{
    leds[LED1] = gColors[gState.color];
    leds[LED1].nscale8_video(cubicwave8(gState.hue));
    leds[LED2] = leds[LED1];
}

void pattern_weave()
{
    leds[LED1] = gColors[gState.color].nscale8(cubicwave8(gState.hue));
    leds[LED2] = gColors[(gState.color + 1) % ARRAY_SIZE(gColors)].nscale8(cubicwave8(gState.hue + 0xFF/2));
}

void pattern_switch()
{
    leds[LED1] = leds[LED2] = (gState.hue & 0x80) ? gColors[gState.color] : gColors[(gState.color + 1) % ARRAY_SIZE(gColors)];
}
