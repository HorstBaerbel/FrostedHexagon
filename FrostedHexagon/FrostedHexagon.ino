//----- Helpers -----------------------------------------------------
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void dumpStateToSerial(bool dump = true);

//#define SERIAL_OUTPUT

//----- FastLED -----------------------------------------------------
#include "FastLED.h"
FASTLED_USING_NAMESPACE

#define DATA_PIN    2
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    19
CRGB leds[NUM_LEDS];
#define FRAMES_PER_SECOND  120

//----- pattern functions ----------------------------------------------
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16(13,0,NUM_LEDS);
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16(i+7,0,NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*PatternFunction)();
PatternFunction patterns[] = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
bool cyclePatterns = true; // true = cycle through patterns
uint8_t patternIndex = 0; // [0, ARRAY_SIZE(patterns) - 1]
#define PATTERNINDEX_MIN 0
#define PATTERNINDEX_MAX ARRAY_SIZE(patterns)-1

void patterns_next()
{
    patternIndex = (patternIndex + 1) % ARRAY_SIZE(patterns); // increase pattern and wrap around
}

void patterns_update()
{
    EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
    EVERY_N_SECONDS( 10 ) { if (cyclePatterns) { patterns_next(); } } // change patterns periodically
    patterns[patternIndex]();
}

uint8_t brightness = 96; // [32, 255]
#define BRIGHTNESS_MIN 32
#define BRIGHTNESS_MAX 255

uint8_t speed = 128; // [0, 255]
#define SPEED_MIN 0
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
        menuState = 0;
        lastMenuAction = millis();
    }
}

void menu_overlay()
{
    if (menuState > 0) {
        // clear first row of display for menu
        for (int i = 16; i < NUM_LEDS; ++i) { 
            leds[i] = CRGB(0, 0, 0);
        }
        // dim down second row of display for menu
        for (int i = 12; i < 16; ++i) {
            leds[i].fadeToBlackBy(192);
        }
        // make blinking work
        EVERY_N_MILLISECONDS( MENU_BLINK_INTERVAL ) { menuBlinkState = !menuBlinkState; }
        const uint8_t colorValue = menuState == 1 || menuBlinkState ? 255 : 0;
        // highlight selected menu item
        switch (menuIndex) {
            case 0: // select pattern
                leds[16] = CRGB(colorValue, 0, 0);
                break;
            case 1: // set brightness
                leds[17] = CRGB(0, colorValue, 0);
                break;
            case 2: // set speed
                leds[18] = CRGB(0, 0, colorValue);
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
#ifdef SERIAL_OUTPUT
        Serial.println("Stored to EEPROM");
#endif
    }
}

void eeprom_loadState() {
#ifdef SERIAL_OUTPUT
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
#ifdef SERIAL_OUTPUT
        Serial.println("Ok. Read:");
#endif
    }
#ifdef SERIAL_OUTPUT
    else {
        Serial.println("Error. Bad magic number!");
    }
    dumpStateToSerial();
#endif
    mustStoreSettings = false;
}

//----- main loop -----------------------------------------------------

void dumpStateToSerial(bool dump)
{
    if (dump) {
        Serial.println("-------------------");
        Serial.println("Brightness: " + String(brightness));
        Serial.println("Speed: " + String(speed));
        Serial.println("Pattern index: " + String(patternIndex));
        Serial.println("Cycle patterns: " + String(cyclePatterns));
        Serial.println("Menu state: " + String(menuState));
        Serial.println("Menu index: " + String(menuIndex));
    }
}

void setup()
{
    delay(3000); // 3 second delay for recovery
#ifdef SERIAL_OUTPUT
    Serial.begin(115200);
    Serial.println("----- Started -----");
#endif
    eeprom_loadState(); // load state from EEPROM
    encoder.write(0);
    // set up LEDs
    FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}
  
void loop()
{
    FastLED.setBrightness(brightness);
    // update LEDs through pattern
    patterns_update();
    // draw GUI on top of LED data
    menu_overlay();
    // push data to LEDs
    FastLED.show();
    // insert a delay to keep the framerate modest
    //FastLED.delay(1000/FRAMES_PER_SECOND); 
    // update GUI
    menu_checkEncoderButton();
    menu_checkEncoder();
    menu_checkTimeout();
#ifdef SERIAL_OUTPUT
    dumpStateToSerial(mustStoreSettings);
#endif
    // store state if some of the settings changed
    eeprom_storeState();
}

