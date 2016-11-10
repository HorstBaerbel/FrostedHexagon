//----- Helpers -----------------------------------------------------
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

// define this to output debug messages through the serial port
//#define DEBUG_OUTPUT
#ifdef DEBUG_OUTPUT
    #ifndef SERIAL_OUTPUT
        #define SERIAL_OUTPUT
    #endif
    void dumpStateToSerial(bool dumpOverride = false);
#endif

//----- FastLED -----------------------------------------------------
#include "FastLED.h"
FASTLED_USING_NAMESPACE

#define DATA_PIN 2
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define LEDS_HEIGHT 5
#define NUM_LEDS 19
CRGB leds0[NUM_LEDS];
CRGB leds1[NUM_LEDS];
#define SIZE_LEDS sizeof(leds0)
CRGB *frontBuffer = (CRGB*)&leds0;
CRGB *backBuffer = (CRGB*)&leds1;
#define FRAMES_PER_SECOND 60

// define this to output LED data to processing via the serial port
//#define ENABLE_LED_EMULATION
#ifdef ENABLE_LED_EMULATION
    #ifndef SERIAL_OUTPUT
        #define SERIAL_OUTPUT
    #endif
    #define PARAMETER_CHAR_PATTERN 'p'
    #define PARAMETER_CHAR_CYCLE 'c'
    #define PARAMETER_CHAR_BRIGHTNESS 'b'
    #define PARAMETER_CHAR_SPEED 's'
    void dumpFrameBufferToSerial(const CRGB * buffer, uint16_t size = SIZE_LEDS);
    void readParametersFromSerial();
#endif

const uint8_t pixelsPerLine[5] = {3, 4, 5, 4, 3};

const int8_t pixelMap[5][5] = {
    { 16, 17, 18, -1, -1},
    { 15, 14, 13, 12, -1},
    {  7,  8,  9, 10, 11},
    {  6,  5,  4,  3, -1},
    {  0,  1,  2, -1, -1}
}; // maps virtual screen pixels to the display. if a pixel index is -1 it does not exist

struct vec2 { int8_t x; int8_t y; };

const vec2 shiftMap0[5][5] = {
    {{-1,  0}, {-1,  0}, {-1,  0}, { 0,  0}, { 0,  0}},
    {{ 1,  0}, { 1,  0}, { 1,  0}, { 1,  0}, { 0,  0}},
    {{-1,  0}, {-1,  0}, {-1,  0}, {-1,  0}, {-1,  0}},
    {{ 1,  0}, { 1,  0}, { 1,  0}, { 1,  0}, { 0,  0}},
    {{-1,  0}, {-1,  0}, {-1,  0}, { 0,  0}, { 0,  0}}
};

//----- pattern functions ----------------------------------------------
// all these pattern functions must be time-based. that is they must base their
// visuals solely on a uint16_t input parameter that increases from 0 to 65536
// in one animation cycle.
#define FRACT_TO_NUMBER(a) ((a) >> 16)

CRGB glitter[NUM_LEDS];
#define GLITTER_ADD_INTERVAL ((uint32_t)(65536/128)) // add glitter every 15ms
#define GLITTER_FADE_INTERVAL ((uint32_t)(65536/32)) // fully fade glitter to black in 60ms

#define FUNCTION_BY_INTERVAL_VALUE(fractPassed, interval, function) ({ \
    static uint32_t accumulate = 0; \
    accumulate += fractPassed; \
    uint8_t value = ((uint32_t)255 * accumulate) / (uint32_t)(interval); \
    if (value > 0) { \
        function \
        accumulate -= ((uint32_t)value * (uint32_t)(interval)) / (uint32_t)255; \
    }})

#define FUNCTION_BY_INTERVAL_LOOP(fractPassed, interval, function) ({ \
    static uint32_t accumulate = 0; \
    accumulate += fractPassed; \
    while (accumulate > interval) { \
        function \
        accumulate -= interval; \
    }})

void addGlitter(uint16_t cycleFract, uint16_t fractPassed, fract8 chanceOfGlitter) 
{
    // fade existing glitter
    FUNCTION_BY_INTERVAL_VALUE(fractPassed, GLITTER_FADE_INTERVAL, fadeToBlackBy(glitter, NUM_LEDS, value););
    // check if we need to add new glitter
    FUNCTION_BY_INTERVAL_LOOP(fractPassed, GLITTER_ADD_INTERVAL, if (random8() < chanceOfGlitter) { glitter[random16(NUM_LEDS)] = CRGB::White; });
    // add glitter to LED data
    for (uint8_t j = 0; j < NUM_LEDS; ++j) {
        frontBuffer[j] += glitter[j];
    }
}

void rainbow(uint16_t cycleFract, uint16_t /*fractPassed*/)
{
    uint8_t hue = FRACT_TO_NUMBER((uint32_t)255 * (uint32_t)cycleFract);
    fill_rainbow(frontBuffer, NUM_LEDS, hue, 7);
}

void rainbowWithGlitter(uint16_t cycleFract, uint16_t fractPassed) 
{
    rainbow(cycleFract, fractPassed);
    addGlitter(cycleFract, fractPassed, 80);
}

#define CONFETTI_ADD_INTERVAL ((uint32_t)(65536/128)) // add confetti every 15ms
#define CONFETTI_FADE_INTERVAL ((uint32_t)(65536/4)) // fully fade confetti to black in 250ms

void confetti(uint16_t cycleFract, uint16_t fractPassed)
{
    uint8_t hue = FRACT_TO_NUMBER((uint32_t)255 * (uint32_t)cycleFract);
    // fade existing confetty
    FUNCTION_BY_INTERVAL_VALUE(fractPassed, CONFETTI_FADE_INTERVAL, fadeToBlackBy(frontBuffer, NUM_LEDS, value););
    // check if we need to add new confetti
    FUNCTION_BY_INTERVAL_LOOP(fractPassed, CONFETTI_FADE_INTERVAL, frontBuffer[random16(NUM_LEDS)] += CHSV(hue + random8(64), 200, 191 + random8(64)););
}

#define SINELON_HUE_INTERVAL ((uint32_t)(65536/3))
#define SINELON_ADD_INTERVAL ((uint32_t)(65536/256))
#define SINELON_FADE_INTERVAL ((uint32_t)(65536/4))

void sinelon(uint16_t cycleFract, uint16_t fractPassed)
{
    uint8_t hue = ((uint32_t)255 * (uint32_t)cycleFract) / SINELON_HUE_INTERVAL;
    // fade existing snake
    FUNCTION_BY_INTERVAL_VALUE(fractPassed, SINELON_FADE_INTERVAL, fadeToBlackBy(frontBuffer, NUM_LEDS, value););
    // add snake head
    uint32_t addValue = ((uint32_t)255 * (uint32_t)fractPassed) / SINELON_ADD_INTERVAL;
    // calculate snake head position
    uint32_t pos = (((uint32_t)sin16(cycleFract) + 32767) * (NUM_LEDS - 1));
    // calculate factors for both head pixels involved. we interpret pos as a 16.16 fixed point number
    uint32_t posFract = pos & 65535;
    uint8_t valueValue = (addValue * posFract) >> 16;
    CRGB color1 = CHSV(hue, 200, 255);
    CRGB color2 = color1;
    color1 = color1.fadeToBlackBy(valueValue);
    color2 = color2.fadeToBlackBy(255 - valueValue);
    frontBuffer[pos >> 16] += color1;
    if ((pos >> 16) + 1 < NUM_LEDS) {
        frontBuffer[(pos >> 16) + 1] += color2;
    }
    FUNCTION_BY_INTERVAL_LOOP(fractPassed, GLITTER_ADD_INTERVAL, frontBuffer[random16(NUM_LEDS)] += CHSV(hue + random8(64), 200, 191 + random8(64)););
}

#define APPLYMAP_HUE_INTERVAL ((uint32_t)(65536/3))
#define APPLYMAP_SHIFT_INTERVAL ((uint32_t)(65536/16))
#define APPLYMAP_ADD_INTERVAL ((uint32_t)(65536/4))

void applyMap0(uint16_t cycleFract, uint16_t fractPassed)
{
    applyMap(cycleFract, fractPassed, &shiftMap0);
}

void applyMap(uint16_t cycleFract, uint16_t fractPassed, const vec2 (*shiftMap)[5][5])
{
    FUNCTION_BY_INTERVAL_LOOP(fractPassed, APPLYMAP_SHIFT_INTERVAL, 
        //memset(backBuffer, 0, SIZE_LEDS);
        memcpy(backBuffer, frontBuffer, SIZE_LEDS);
        for (int8_t y = 0; y < LEDS_HEIGHT; ++y) {
            int8_t w = pixelsPerLine[y];
            for (int8_t x = 0; x < w; ++x) {
                // get shift value from map
                vec2 shift = (*shiftMap)[y][x];
                // calculate source pixel and clamp to display
                int8_t dstY = y + shift.y;
                if (dstY >= 0 && dstY < LEDS_HEIGHT) {
                    int8_t dstX = x + shift.x;
                    if (dstX >= 0 && dstX < pixelsPerLine[dstY]) {
                        // copy color from source to backbuffer
                        CRGB color0 = frontBuffer[pixelMap[dstY][dstX]] / (uint8_t)2;
                        CRGB color1 = backBuffer[pixelMap[y][x]] / (uint8_t)2;
                        frontBuffer[pixelMap[dstY][dstX]] = color0 + color1;
                    }
                }
            }
        }
        // copy back buffer to front buffer
        /*for (uint8_t i = 0; i < NUM_LEDS; ++i) {
            frontBuffer[i] += backBuffer[i] / (uint8_t)16;
        }*/
    );
    // add some pixels from time to time
    uint8_t hue = ((uint32_t)255 * (uint32_t)cycleFract) / APPLYMAP_HUE_INTERVAL;
    FUNCTION_BY_INTERVAL_LOOP(fractPassed, APPLYMAP_ADD_INTERVAL, frontBuffer[random16(NUM_LEDS)] += CHSV(hue + random8(64), 200, 191 + random8(64)););
}

#define SIFT_MOVE_INTERVAL ((uint32_t)(65536/16))
#define SIFT_HUE_INTERVAL ((uint32_t)(65536/3))

void sift(uint16_t cycleFract, uint16_t fractPassed)
{
    static const int8_t lineAdjustDestX[5] = {0, 0, 0, -1, -1};
    uint8_t hue = ((uint32_t)255 * (uint32_t)cycleFract) / SIFT_HUE_INTERVAL;
    FUNCTION_BY_INTERVAL_LOOP(fractPassed, SIFT_MOVE_INTERVAL, 
        // clear bottom row first
        for (int8_t x = 0; x < pixelsPerLine[4]; ++x) {
            frontBuffer[pixelMap[4][x]] = CRGB::Black;
        }
        // loop from bottom to top and shift pixels down a line. pixels can either go left or right
        for (int8_t y = 3; y >= 0; --y) {
            int8_t adjustX = lineAdjustDestX[y + 1];
            for (int8_t x = 0; x < pixelsPerLine[y]; ++x) {
                int8_t destX = x + random8(1) + adjustX;
                if (destX >= 0 || destX < pixelsPerLine[y + 1]) {
                    frontBuffer[pixelMap[y + 1][destX]] = frontBuffer[pixelMap[y][x]];
                    frontBuffer[pixelMap[y][x]] = CRGB::Black;
                }
            }
        }
        // now add some pixels at the top from time to time
        frontBuffer[pixelMap[0][random16(3)]] += CHSV(hue + random8(64), 200, 191 + random8(64));
    );
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*PatternFunction)(uint16_t, uint16_t);
PatternFunction patterns[] = { rainbow, rainbowWithGlitter, confetti, sinelon/*, applyMap0*/ };
bool cyclePatterns = true; // true = cycle through patterns
uint8_t patternIndex = 0; // [0, ARRAY_SIZE(patterns) - 1]
#define PATTERNINDEX_MIN 0
#define PATTERNINDEX_MAX ARRAY_SIZE(patterns)-1
uint16_t patternCycleFract = 0; // a full cycle for an effect goes from 0 to 1.0 aka 65536
uint16_t lastPatternCallFract = 0; // the difference between this and the last pattern function call
#define FRACT_TIME_DIFF(a,b) ((a >= b) ? (a - b) : ((65535 - b) + a))

void patterns_next()
{
    patternIndex = (patternIndex + 1) % ARRAY_SIZE(patterns); // increase pattern and wrap around
}

void patterns_update(uint8_t factor)
{	
    EVERY_N_SECONDS(60) { if (cyclePatterns) { patterns_next(); } } // change patterns periodically
	EVERY_N_MILLISECONDS(50) {
        // calculate fractional pattern cycle increase
    	patternCycleFract += ((((uint32_t)65535 * 50) * factor) / 1024) / 512;
        // calculate time passed from last call to this call
        uint16_t patternCallFracPassed = FRACT_TIME_DIFF(patternCycleFract, lastPatternCallFract);
        lastPatternCallFract = patternCycleFract;
        patterns[patternIndex](patternCycleFract, patternCallFracPassed);
	}
}

uint8_t brightness = 96; // [32, 255]
#define BRIGHTNESS_MIN 32
#define BRIGHTNESS_MAX 255

uint8_t speed = 128; // [1, 255]
#define SPEED_MIN 1
#define SPEED_MAX 255

//----- rotary encoder ----------------------------------------------
//#define ENCODER_DO_NOT_USE_INTERRUPTS // define this if you have problems with other libraries using interrupts
//#define ENCODER_OPTIMIZE_INTERRUPTS // define this to use optimized interrupt routines
#include <Encoder.h>
Encoder encoder(9, 8);

//----- encoder button ----------------------------------------------
#include <Pushbutton.h>
Pushbutton encoderButton(7);

//----- menu state --------------------------------------------------
#define MENU_TIMEOUT 10000 // menu will be hidden after 10s
long lastMenuAction = 0; // the last time the user did something
uint8_t menuIndex = 0; // 0 = animation selection, 1 = brightness, 2 = speed
uint8_t menuState = 0; // 0 = menu not visible, 1 = menu point selected, 2 = adjusting value
uint8_t menuPatternIndex = patternIndex; // internal pattern index value;
bool mustStoreSettings = true; // if true, settings will be stored on next eeprom_storeState() call
#define MENU_BLINK_INTERVAL 250 // interval in ms at which GUI blinks when activated
bool menuBlinkState = true; // state in which menu blinking is in: on or off
#define MENUINDEX_MIN 0
#define MENUINDEX_MAX 2

void menu_checkEncoderButton()
{
    // check if button was pressed
    if (encoderButton.getSingleDebouncedPress()) {
        lastMenuAction = millis(); // yes. update last menu usage time
        switch (menuState) {
            case 0:
                menuState = 1; // we're in "off" state, activate menu selection
                break;
            case 1:
                menuState = 2; // we're in menu selection, activate adjustment of parameter
                if (menuIndex == 0) {
                    menuPatternIndex = cyclePatterns ? 0 : patternIndex + 1; // if cycling is on, value is 0, else value is patternIndex + 1
                }
                break;
            case 2:
                menuState = 1; // we're in parameter adjustment mode, return to menu selection
                break;
            default:
                menuState = 0; // not sure what happened, hide menu
        }
        encoder.write(0);
    }
}

void menu_checkEncoder()
{
    // the encoder always shows us two steps, so halve it
    int value = encoder.read() / 2;
    // the encoder is always reset to 0, so check if the value changed
    if (value != 0) {
        lastMenuAction = millis(); // yes. update last menu usage time
        if (menuState == 1) {
            // we're in menu selection mode, change the menu item
            int newIndex = constrain(menuIndex + value, 0, 2);
            if (menuIndex != newIndex) {
                menuIndex = newIndex;
                mustStoreSettings = true;
            }
        }
        else if (menuState == 2) {
            // we're in parameter change mode, so check what parameter to change
            switch (menuIndex) {
                case 0: {
                    int newIndex = constrain(menuPatternIndex + value, 0, PATTERNINDEX_MAX + 1);
                    if (menuPatternIndex != newIndex) { 
                        menuPatternIndex = newIndex;
                        cyclePatterns = menuPatternIndex <= 0 ? true : false;
                        patternIndex = menuPatternIndex > 0 ? menuPatternIndex - 1 : patternIndex;
                        mustStoreSettings = true;
                    }
                }
                break;
                case 1: {
                    int newBrightness = constrain(brightness + value, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
                    if (brightness != newBrightness) {
                        brightness = newBrightness;
                        mustStoreSettings = true;
                    }
                }
                break;
                case 2:
                    int newSpeed = constrain(speed + value, SPEED_MIN, SPEED_MAX);
                    if (speed != newSpeed) {
                        speed = newSpeed;
                        mustStoreSettings = true;
                    }
                    break;
            }
        }
        encoder.write(0);
    }
}

void menu_checkTimeout()
{
    if ((millis() - lastMenuAction) >= MENU_TIMEOUT) {
        // clear menu state to show full animation
        menuState = 0;
        lastMenuAction = millis();
        // store state if some of the settings changed
        eeprom_storeState();
    }
}

void menu_overlay()
{
    if (menuState > 0) {
        // clear first row of display for menu
        for (int i = 12; i < NUM_LEDS; ++i) { 
            frontBuffer[i] = CRGB(0, 0, 0);
        }
        // make blinking work
        EVERY_N_MILLISECONDS( MENU_BLINK_INTERVAL ) { menuBlinkState = !menuBlinkState; }
        const uint8_t colorValue = menuState == 1 || menuBlinkState ? 255 : 0;
        // highlight selected menu item
        switch (menuIndex) {
            case 0: // select pattern
                frontBuffer[16] = CRGB(colorValue, 0, 0);
                break;
            case 1: // set brightness
                frontBuffer[17] = CRGB(0, colorValue, 0);
                break;
            case 2: // set speed
                frontBuffer[18] = CRGB(0, 0, colorValue);
                break;
        }
    }
}

//----- EEPROM -------------------------------------------------------
#include <EEPROM.h>
#define EEPROM_START_ADDRESS 0;
#define EEPROM_MAGIC "FHEX"

void eeprom_storeState() {
    if (mustStoreSettings) {
        mustStoreSettings = false;
        int address = EEPROM_START_ADDRESS;
        EEPROM.update(address++, EEPROM_MAGIC[0]); // store magic header bytes
        EEPROM.update(address++, EEPROM_MAGIC[1]);
        EEPROM.update(address++, EEPROM_MAGIC[2]);
        EEPROM.update(address++, EEPROM_MAGIC[3]);
        EEPROM.update(address++, brightness);
        EEPROM.update(address++, speed);
        EEPROM.update(address++, patternIndex);
        EEPROM.update(address++, cyclePatterns);
        EEPROM.update(address++, menuIndex);
#ifdef DEBUG_OUTPUT
        Serial.println("Stored to EEPROM");
#endif
    }
}

void eeprom_loadState() {
#ifdef DEBUG_OUTPUT
    Serial.print("Loading from EEPROM - ");
#endif
    int address = EEPROM_START_ADDRESS;
    // check if we can find the magic header
    if (EEPROM.read(address++) == EEPROM_MAGIC[0] &&
        EEPROM.read(address++) == EEPROM_MAGIC[1] &&
        EEPROM.read(address++) == EEPROM_MAGIC[2] &&
        EEPROM.read(address++) == EEPROM_MAGIC[3])
    {
        // read the settings. note that constrain is a macro and thus it can not be used like:
        // uint8_t value = constrain(EEPROM.read(address++), a, b), because it will get expanded to
        // uint8_t value = EEPROM.read(address++) < a ? a : EEPROM.read(address++) > b ? b : EEPROM.read(address++);
        // and obviously give bogus results...
        uint8_t tempBrightness = EEPROM.read(address++);
        brightness = constrain(tempBrightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
        uint8_t tempSpeed = EEPROM.read(address++);
        speed = constrain(tempSpeed, SPEED_MIN, SPEED_MAX);
        uint8_t tempPatternIndex = EEPROM.read(address++);
        patternIndex = constrain(tempPatternIndex, PATTERNINDEX_MIN, PATTERNINDEX_MAX);
        uint8_t tempCyclePatterns = EEPROM.read(address++);
        cyclePatterns = constrain(tempCyclePatterns, false, true);
        uint8_t tempMenuIndex = EEPROM.read(address++);
        menuIndex = constrain(tempMenuIndex, MENUINDEX_MIN, MENUINDEX_MAX);
#ifdef DEBUG_OUTPUT
        Serial.println("Ok. Read:");
#endif
    }
#ifdef DEBUG_OUTPUT
    else {
        Serial.println("Error. Bad magic number!");
    }
    dumpStateToSerial(true);
#endif
    mustStoreSettings = false;
}

//----- main loop -----------------------------------------------------

#ifdef DEBUG_OUTPUT
uint8_t lastBrightness = brightness;
uint8_t lastSpeed = speed;
uint8_t lastPatternIndex = patternIndex;
uint8_t lastCyclePatterns = cyclePatterns;
uint8_t lastMenuState = menuState;
uint8_t lastMenuIndex = menuIndex;

void dumpStateToSerial(bool dumpOverride)
{
    if (dumpOverride) {
        Serial.println("Brightness: " + String(brightness));
        Serial.println("Speed: " + String(speed));
        Serial.println("Pattern index: " + String(patternIndex));
        Serial.println("Cycle patterns: " + String(cyclePatterns));
        Serial.println("Menu state: " + String(menuState));
        Serial.println("Menu index: " + String(menuIndex));
    }
    else {
        if (lastBrightness != brightness) {
            lastBrightness = brightness;
            Serial.println("Brightness: " + String(brightness));
        }
        if (lastSpeed != speed) {
            lastSpeed = speed;
            Serial.println("Speed: " + String(speed));
        }
        if (lastPatternIndex != patternIndex) {
            lastPatternIndex = patternIndex;
            Serial.println("Pattern index: " + String(patternIndex));
        }
        if (lastCyclePatterns != cyclePatterns) {
            lastCyclePatterns = cyclePatterns;
            Serial.println("Cycle patterns: " + String(cyclePatterns));
        }
        if (lastMenuState != menuState) {
            lastMenuState = menuState;
            Serial.println("Menu state: " + String(menuState));
        }
        if (lastMenuIndex != menuIndex) {
            lastMenuIndex = menuIndex;
            Serial.println("Menu index: " + String(menuIndex));
        }
    }
}
#endif

#ifdef ENABLE_LED_EMULATION
void dumpFrameBufferToSerial(const CRGB * buffer, uint16_t size)
{
    Serial.print("LEDS,");
    uint16_t sizeBuffer = size;
    Serial.write((byte *)&sizeBuffer, 2);
    Serial.print(",");
    Serial.write((const byte *)buffer, size);
}

void readParametersFromSerial()
{
     if (Serial.available() >= 2) {
        const uint8_t command = Serial.read();
        const uint8_t value = Serial.read();
        switch (command) {
            case PARAMETER_CHAR_PATTERN:
                patternIndex = constrain(value, PATTERNINDEX_MIN, PATTERNINDEX_MAX);
                break;
            case PARAMETER_CHAR_CYCLE:
                cyclePatterns = constrain(value, false, true);
                break;
            case PARAMETER_CHAR_BRIGHTNESS:
                brightness = constrain(value, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
                break;
            case PARAMETER_CHAR_SPEED:
                speed = constrain(value, SPEED_MIN, SPEED_MAX);
                break;
            default:
                break;
        }
     }
}
#endif

void setup()
{
    delay(3000); // 3 second delay for recovery
#ifdef SERIAL_OUTPUT
    Serial.begin(115200);
    Serial.setTimeout(10);
    Serial.println("----- Started -----");
#endif
    eeprom_loadState(); // load state from EEPROM
    encoder.write(0);
    // set up LEDs
    FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(frontBuffer, NUM_LEDS).setCorrection(TypicalLEDStrip);
}
  
void loop()
{
    // update display at 60fps
    EVERY_N_MILLISECONDS(1000 / FRAMES_PER_SECOND)
    {
        FastLED.setBrightness(brightness);
        // update LEDs through pattern
        patterns_update(speed);
        // draw GUI on top of LED data
        menu_overlay();
        // push data to LEDs
        FastLED.show();
    }
    // update menu and parameters
    menu_checkEncoderButton();
    menu_checkEncoder();
    menu_checkTimeout();
#ifdef DEBUG_OUTPUT
    dumpStateToSerial();
#endif
#ifdef ENABLE_LED_EMULATION
    dumpFrameBufferToSerial(frontBuffer);
    readParametersFromSerial();
#endif
}

