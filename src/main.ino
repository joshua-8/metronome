/*
 * A small, silent, blue metronome for my grandma.
 * Uses a 128x32 OLED display and a Trinket M0.
 * Wire two buttons between ground and pin 3 and pin 4.
 * v1.0 2025-01-02 by joshua-8
 */

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FastLED.h>
#include <FlashAsEEPROM.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <SPI.h>
#include <Wire.h>

CHSV color = CHSV(160, 255, 255);

const unsigned long lockTimeMillis = 4000; // this many milliseconds after a frequency change, the frequency is saved to EEPROM
unsigned long blinkDurationMicros = 50000; // the led turns on for this many microseconds
const unsigned long buttonTimeout1 = 50;
const unsigned long buttonTimeout2 = 500;
const unsigned long buttonTimeout3 = 75;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

CRGB led = CRGB(255, 0, 0);

#define NUM_FREQS 39

const int frequencies[39] = { 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 63, 66, 69, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108, 112, 116, 120, 126, 132, 138, 144, 152, 160, 168, 176, 184, 192, 200, 208 };

int frequency = 100;

int setFrequencyIndex = 21;

int savedFrequencyIndex = 0;

#define frequency_eeprom_address 0

unsigned long frequencyChangedMillis = 0;

const byte pButtonPin = 3;
const byte nButtonPin = 4;

boolean lastpButtonState = HIGH;
boolean lastnButtonState = HIGH;

unsigned long pButtonPressedMillis = 0;
unsigned long nButtonPressedMillis = 0;

unsigned long blinkStartMicros = 0;

int displayedFrequency = -1;

unsigned long pButtonTimeout = buttonTimeout1;
unsigned long nButtonTimeout = buttonTimeout1;

void setup()
{
    Serial.begin(9600);
    pinMode(pButtonPin, INPUT_PULLUP);
    pinMode(nButtonPin, INPUT_PULLUP);
    FastLED.addLeds<APA102, PIN_DOTSTAR_DATA, PIN_DOTSTAR_CLK, BGR>(&led, 1);
    delay(10);
    FastLED.show();

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    if (EEPROM.isValid() == false || EEPROM.read(frequency_eeprom_address) >= NUM_FREQS) {
        led = CRGB::Yellow;
        FastLED.show();
        delay(1000);
        EEPROM.write(frequency_eeprom_address, setFrequencyIndex);
        EEPROM.commit();
    }
    if (EEPROM.isValid() == false) {
        Serial.println("eeprom failed");
        for (;;)
            ;
    }

    setFrequencyIndex = EEPROM.read(frequency_eeprom_address);
    savedFrequencyIndex = setFrequencyIndex;

    display.setRotation(2);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.clearDisplay();
    display.display();

    led = CRGB::Black;
    FastLED.show();
}

void loop()
{
    if (digitalRead(pButtonPin) == LOW) {
        if (lastpButtonState == HIGH) {
            pButtonPressedMillis = millis();
        }
        lastpButtonState = LOW;
        if (millis() - pButtonPressedMillis > pButtonTimeout) {
            if (pButtonTimeout == buttonTimeout2) {
                pButtonTimeout = buttonTimeout3;
            }
            if (pButtonTimeout == buttonTimeout1) {
                pButtonTimeout = buttonTimeout2;
            }
            setFrequencyIndex++;
            pButtonPressedMillis = millis();
        }
    } else {
        lastpButtonState = HIGH;
        pButtonTimeout = buttonTimeout1;
    }
    if (digitalRead(nButtonPin) == LOW) {
        if (lastnButtonState == HIGH) {
            nButtonPressedMillis = millis();
        }
        lastnButtonState = LOW;
        if (millis() - nButtonPressedMillis > nButtonTimeout) {
            if (nButtonTimeout == buttonTimeout2) {
                nButtonTimeout = buttonTimeout3;
            }
            if (nButtonTimeout == buttonTimeout1) {
                nButtonTimeout = buttonTimeout2;
            }
            setFrequencyIndex--;
            nButtonPressedMillis = millis();
        }
    } else {
        lastnButtonState = HIGH;
        nButtonTimeout = buttonTimeout1;
    }

    setFrequencyIndex = constrain(setFrequencyIndex, 0, NUM_FREQS - 1);

    frequency = frequencies[setFrequencyIndex];

    if (frequency != displayedFrequency) {
        displayedFrequency = frequency;
        display.clearDisplay();
        display.setFont(&FreeMonoBold24pt7b);
        display.setCursor(2, 29);
        display.printf("%3d", frequency);
        display.display();
    }

    if (setFrequencyIndex == savedFrequencyIndex) {
        frequencyChangedMillis = millis();
        digitalWrite(13, LOW);
    } else {
        digitalWrite(13, HIGH);
    }
    if (savedFrequencyIndex != setFrequencyIndex && (millis() - frequencyChangedMillis) > lockTimeMillis) {
        savedFrequencyIndex = setFrequencyIndex;
        EEPROM.write(frequency_eeprom_address, setFrequencyIndex);
        EEPROM.commit();
    }

    unsigned long blinkIntervalMicros = 60000000 / frequency;

    unsigned long currentMicros = micros();
    if (currentMicros - blinkStartMicros >= blinkIntervalMicros) {
        blinkStartMicros += blinkIntervalMicros;
    }
    if (currentMicros - blinkStartMicros < blinkDurationMicros) {
        if (led != color) {
            led = color;
            FastLED.show();
        }
    } else {
        if (led != CRGB::Black) {
            led = CRGB::Black;
            FastLED.show();
        }
    }
}
