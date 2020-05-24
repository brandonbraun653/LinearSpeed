
/* STL Includes */
#include <array>
#include <cstdint>

/* Arduino Includes */
#include <Arduino.h>
#include "SSD1306Spi.h"
#include "OLEDDisplayUi.h"

/*-------------------------------------------------
Screen Digital Pins:
SPI is the default for the ESP32 Dev1 Kit.

D5 -> CLK
D7 -> MOSI (DOUT)
D0 -> RES
D2 -> DC
D8 -> CS
-------------------------------------------------*/
// CLK pin is D18
// MOSI pin is D23
static constexpr uint32_t D0 = 15;
static constexpr uint32_t D2 = 2;
static constexpr uint32_t D8 = 4;

/*-------------------------------------------------
External Interrupt (Counter)
-------------------------------------------------*/
static constexpr uint32_t EXTI_D21 = 21;

/*-------------------------------------------------
Module Objects
-------------------------------------------------*/
static SSD1306Spi display(D0, D2, D8);
static volatile uint32_t isrCounter;
static uint32_t counter;

/*-------------------------------------------------
Module Functions
-------------------------------------------------*/
static void pulseISR()
{
  isrCounter++;
}

void setup() {
  /*-------------------------------------------------
  Initialize module data
  -------------------------------------------------*/
  isrCounter = 0;
  counter = 0;

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
  Copy out the volatile data
  -------------------------------------------------*/
  noInterrupts();
  counter = isrCounter;
  interrupts();

  /*-------------------------------------------------
  Update the display with the necessary data
  -------------------------------------------------*/
  display.clear();
  display.drawString(0, 0, String(counter));
  display.display();

  delay( 50 );
}
