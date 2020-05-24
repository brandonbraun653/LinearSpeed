
/* STL Includes */
#include <array>
#include <cstdint>

/* Arduino Includes */
#include <Arduino.h>
#include "SSD1306Spi.h"
#include "OLEDDisplayUi.h"

/*-------------------------------------------------
Constants
-------------------------------------------------*/
static constexpr uint32_t displayUpdateRate = 50;   // ms
static constexpr uint32_t isrDebouncingTime = 15;   // ms
static constexpr uint32_t pixelsPerRow = 8;

/*-------------------------------------------------
Screen Digital Pins:
SPI is the default for the ESP32 Dev1 Kit and doesn't
need to be initialized.

D18 -> CLK
D23 -> MOSI (DOUT)
D15 -> RES
D2  -> DC
D4  -> CS
-------------------------------------------------*/
static constexpr uint32_t D15 = 15;
static constexpr uint32_t D2 = 2;
static constexpr uint32_t D4 = 4;

/*-------------------------------------------------
External Interrupt (Counter)
-------------------------------------------------*/
static constexpr uint32_t EXTI_D21 = 21;
static volatile bool isrFired;
static volatile uint32_t isrCounter;
static uint32_t counter;
static uint32_t isrDebounceStart;

static volatile bool lastState;
static bool saveLastState;
static bool currentState;
static uint32_t saveISRDebounceStart;

/*-------------------------------------------------
Module Objects
-------------------------------------------------*/
static SSD1306Spi display(D15, D2, D4);
static uint32_t lastUpdateTime;

static std::array<char, 100> printBuffer;

/*-------------------------------------------------
Module Functions
-------------------------------------------------*/
static void pulseISR()
{
  isrFired = true;
  lastState = digitalRead( EXTI_D21 );
  isrDebounceStart = millis();
}

void setup() {
  /*-------------------------------------------------
  Initialize module data
  -------------------------------------------------*/
  printBuffer.fill(0);
  lastUpdateTime = millis();
  isrCounter = 0;
  counter = 0;
  isrDebounceStart = 0;

  /*-------------------------------------------------
  Attatch the ISR routine that counts input pulses
  -------------------------------------------------*/
  attachInterrupt( digitalPinToInterrupt(EXTI_D21), pulseISR, RISING );

  /*-------------------------------------------------
  Initialize the display class
  -------------------------------------------------*/
  display.init();
  display.flipScreenVertically();
  display.clear();
}


void loop() {
  /*-------------------------------------------------
  Copy out the volatile ISR data
  -------------------------------------------------*/
  noInterrupts();
  saveLastState = lastState;
  saveISRDebounceStart = isrDebounceStart;
  interrupts();

  /*-------------------------------------------------
  Perform software debouncing of the pulse signal
  -------------------------------------------------*/
  currentState = digitalRead(EXTI_D21);

  if( isrFired
   && ( saveLastState == currentState )
   && ( (millis() - saveISRDebounceStart) > isrDebouncingTime ))
  {
    isrFired = false;
    counter++;
  }

  /*-------------------------------------------------
  Update the display with the necessary data
  -------------------------------------------------*/
  if ((millis() - lastUpdateTime) > displayUpdateRate)
  {
    display.clear();

    /* Print out the pulse count */
    printBuffer.fill(0);
    snprintf(printBuffer.data(), printBuffer.size(), "Pulse Count: %d", counter);
    display.drawString(0, 0, String(printBuffer.data()));

    /* Print out the calculated rate of speed */
    printBuffer.fill(0);
    snprintf(printBuffer.data(), printBuffer.size(), "Speed (m/s): %d", 0);
    display.drawString(0, 10, String(printBuffer.data()));

    /* Push the data to the display */
    display.display();
    lastUpdateTime = millis();
  }

  delay(5);
}
