/********************************************************************************
 *  File Name:
 *    main.cpp
 *
 *  Description:
 *    Implements the pulse counter for Linear Speed
 *
 *  2020 | Brandon Braun, Jarrod Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* STL Includes */
#include <array>
#include <cstdint>

/* Arduino Includes */
#include "OLEDDisplayUi.h"
#include "SSD1306Spi.h"
#include <Arduino.h>

/*-------------------------------------------------------------------------------
Macros and Literals
-------------------------------------------------------------------------------*/
#define ROW_HEIGHT  ( 10 )              // Pixels
#define ROW0        ( 0 * ROW_HEIGHT )
#define ROW1        ( 1 * ROW_HEIGHT )
#define ROW2        ( 2 * ROW_HEIGHT )

/*-------------------------------------------------------------------------------
Constants and Variables
-------------------------------------------------------------------------------*/
static constexpr uint32_t displayUpdateRate = 200;       // How often the display updates its data (ms)
static constexpr uint32_t isrDebouncingTime = 15;        // How long the software debounce time is for pulse events (ms)
static constexpr uint32_t mathUpdateRate    = 100;       // How often to update the math associated with this program
static constexpr uint32_t serialBaud        = 921600;    // Baud rate

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
static constexpr uint32_t D15 = 15;    // Digital pin 15, used for RES pin
static constexpr uint32_t D2  = 2;     // Digital pin 2, used for DC pin
static constexpr uint32_t D4  = 4;     // Digital pin 4, used for CS pin

/*-------------------------------------------------
External Interrupt (Counter)
-------------------------------------------------*/
static constexpr uint32_t EXTI_D21 = 21;      // GPIO digital pin to detect pulse events
static volatile bool isrFired;                // ISR variable to track if the ISR has fired
static volatile uint32_t isrDebounceStart;    // ISR variable for capturing time when ISR fired
static volatile bool lastState;               // ISR variable for capturing the pulse input pin state
static bool saveLastState;                    // Cache: Last known state of the pulse input pin
static bool currentState;                     // Current active state of the pulse input pin (hi/lo)
static uint32_t saveISRDebounceStart;         // Cache: Time when the debouncing process started

/*-------------------------------------------------
User Variables
-------------------------------------------------*/
static uint32_t pulseCount;       // Tracks number of pulse events
static uint32_t pulsePerSecond;    // Tracks pulse events per update cycle
static uint32_t linearRate;       // Rate of speed (m/s)

static uint32_t lastDisplayUpdateTime;    // Last time the screen was updated
static uint32_t lastMathUpdateTime;       // Last time math section was run

/*-------------------------------------------------
Module Objects
-------------------------------------------------*/
static SSD1306Spi display( D15, D2, D4 );    // Screen display object
static std::array<char, 100> printBuffer;    // Output buffer for formatting data

/*-------------------------------------------------------------------------------
Functions
-------------------------------------------------------------------------------*/
/**
 *  Interrupt Service Routine (ISR) that handles pulse event triggers.
 *
 *  @return void
 */
static void pulseISR()
{
  /*-------------------------------------------------
  A new pulse event happened, so save off a few variables so that we know
  the state of the GPIO pin as well as the time when the event occurred.
  -------------------------------------------------*/
  isrFired         = true;
  lastState        = digitalRead( EXTI_D21 );
  isrDebounceStart = millis();
}

/**
 *  Sets up the system for operation. Initializes variables, configures objects.
 *
 *  @return void
 */
void setup()
{
  /*-------------------------------------------------
  Initialize module data
  -------------------------------------------------*/
  printBuffer.fill( 0 );
  lastDisplayUpdateTime = millis();
  lastMathUpdateTime    = millis();
  pulseCount            = 0;
  isrDebounceStart      = 0;
  pulsePerSecond         = 0;
  linearRate            = 0;

  /*-------------------------------------------------
  Attatch the ISR routine that counts input pulses
  -------------------------------------------------*/
  attachInterrupt( digitalPinToInterrupt( EXTI_D21 ), pulseISR, RISING );

  /*-------------------------------------------------
  Initialize the display class
  -------------------------------------------------*/
  display.init();
  display.flipScreenVertically();
  display.clear();

  /*-------------------------------------------------
  Initialize the Serial output
  -------------------------------------------------*/
  Serial.begin( serialBaud );
  Serial.write( "Hey Jarrod\n" );
}

/**
 *  Loop function that will run indefinitely. Performs main functional
 *  operation of the program.
 *
 *  @return void
 */
void loop()
{
  /*-------------------------------------------------
  Copy out the volatile ISR data. Temporarily disable
  interrupts so we can get an accurate read.
  -------------------------------------------------*/
  noInterrupts();
  saveLastState        = lastState;
  saveISRDebounceStart = isrDebounceStart;
  interrupts();

  /*-------------------------------------------------
  Perform software debouncing of the pulse signal
  -------------------------------------------------*/
  currentState = digitalRead( EXTI_D21 );

  if ( isrFired && ( saveLastState == currentState ) && ( ( millis() - saveISRDebounceStart ) > isrDebouncingTime ) )
  {
    isrFired = false;
    pulseCount++;
  }

  /*-------------------------------------------------
  Update the math portion of the code
  -------------------------------------------------*/
  if ( ( millis() - lastMathUpdateTime ) > mathUpdateRate )
  {
    // TODO: Do mathy things here
    pulsePerSecond = 0;
    linearRate     = 0;

    /*-------------------------------------------------
    TODO: Format the output data to CSV
    -------------------------------------------------*/
    // printBuffer.fill( 0 );
    // snprintf( printBuffer.data(), printBuffer.size(), "" );
  }

  /*-------------------------------------------------
  Update the display with the necessary data
  -------------------------------------------------*/
  if ( ( millis() - lastDisplayUpdateTime ) > displayUpdateRate )
  {
    display.clear();

    /* Print out the pulse count */
    printBuffer.fill( 0 );
    snprintf( printBuffer.data(), printBuffer.size(), "Pulse Count: %d", pulseCount );
    display.drawString( 0, ROW0, String( printBuffer.data() ) );

    /* Print out the pulses per second */
    printBuffer.fill( 0 );
    snprintf( printBuffer.data(), printBuffer.size(), "Pulse Per Second: %d", pulsePerSecond );
    display.drawString( 0, ROW1, String( printBuffer.data() ) );

    /* Print out the calculated rate of speed */
    printBuffer.fill( 0 );
    snprintf( printBuffer.data(), printBuffer.size(), "Speed (m/s): %d", linearRate );
    display.drawString( 0, ROW2, String( printBuffer.data() ) );

    /* Push the data to the display */
    display.display();
    lastDisplayUpdateTime = millis();
  }
}
