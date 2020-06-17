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
#define ROW_HEIGHT ( 10 )    // Pixels
#define ROW0 ( 0 * ROW_HEIGHT )
#define ROW1 ( 1 * ROW_HEIGHT )
#define ROW2 ( 2 * ROW_HEIGHT )

/*-------------------------------------------------------------------------------
Constants and Variables
-------------------------------------------------------------------------------*/
static constexpr uint32_t displayUpdateRate = 200;       // How often the display updates its data (ms)
static constexpr uint32_t isrDebouncingTime = 2;         // How long the software debounce time is for pulse events (ms)
static constexpr uint32_t mathUpdateRate    = 100;       // How often to update the math associated with this program
static constexpr uint32_t serialBaud        = 921600;    // Baud rate
static constexpr float wheelRadius          = 1.0f;         // Radius of wheel connected to axel on which encoder wheel is mounted

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
static uint32_t pulseCount;        // Tracks number of pulse events
static float pulsePerSecond;       // Tracks pulse events per update cycle
static float linearRate;        // Rate of speed (m/s)
static float distanceTraveled;   // Total linear distance traveled
static uint32_t lastDisplayUpdateTime;    // Last time the screen was updated
static uint32_t lastMathUpdateTime;       // Last time math section was run
static uint32_t previousPulseCount;       // Pulse count last time the math section was run

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
  pulsePerSecond        = 0.0f;
  linearRate            = 0.0f;
  distanceTraveled      = 0.0f;
  previousPulseCount    = 0;


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

    pulsePerSecond   = 1.0f / ( ( millis() - lastMathUpdateTime ) / 1000.0f );
    linearRate       = ( ( 2.0f * PI / 20.0f ) * pulsePerSecond ) * wheelRadius;
    distanceTraveled = distanceTraveled + linearRate * ( ( millis() - lastMathUpdateTime ) / 1000.0f );

    /*-------------------------------------------------
    1. Force null termination
    2. Format the string
    3. Write the output using the buffer pointer.
    -------------------------------------------------*/
    printBuffer.fill( 0 );
    snprintf( printBuffer.data(), printBuffer.size(), "%lu,%d\n", millis(), pulseCount );
    Serial.write( printBuffer.data() );

    lastMathUpdateTime = millis();
  }

  /*-------------------------------------------------
  Update the math portion of the code
  -------------------------------------------------*/
  if ( ( millis() - lastMathUpdateTime ) > mathUpdateRate )
  {
    // TODO: Do mathy things here
    // duration = (millis()-lastMathUpdateTime)/((1/encoderFrequency)*1000.0f);
    // linearRate     =  (( 2.0f * PI / 20.0f )/duration) * wheelRadius;
    // Jarrod, what is this 20? Generally in programming we don't like magic numbers. Probably should
    // make a #define up at the top of the file like I did with PI and give it a meaningful name.


    // previousPulseCount = pulseCount;
    // lastMathUpdateTime = millis();

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
    snprintf( printBuffer.data(), printBuffer.size(), "Distance Traveled: %2.3f", distanceTraveled );
    display.drawString( 0, ROW1, String( printBuffer.data() ) );

    /* Print out the calculated rate of speed */
    printBuffer.fill( 0 );
    snprintf( printBuffer.data(), printBuffer.size(), "Speed (in/s): %2.3f", linearRate );
    display.drawString( 0, ROW2, String( printBuffer.data() ) );

    /* Push the data to the display */
    display.display();
    lastDisplayUpdateTime = millis();
  }
}
