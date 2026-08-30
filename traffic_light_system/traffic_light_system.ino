/*
 * 3-Way Traffic Signal Controller
 * Converted for STM32 F446RE (Nucleo-64) — Arduino IDE
 *
 * Features:
 * - Overlapping yellow handoff between signals
 * - IR sensor: car detected → green extends to 10s instantly
 * - After completion resets back to 5s
 * - LDR on A0: onboard LED dims in bright light,
 *   brightens at night
 * - Serial terminal countdown for each phase
 *
 * Board setup in Arduino IDE:
 * Board Manager  : STM32 MCU based boards (by STMicroelectronics)
 * Board           : Nucleo-64
 * Board part no.  : Nucleo F446RE
 * Upload method   : STLink
 */

// =======================================================
// PIN DEFINITIONS
// =======================================================

// D13 (PA5) is the onboard LED on Nucleo F446RE
// RED_1 remapped to A1 (PA1)

const int RED_1 = A1;
const int YEL_1 = D12;
const int GRN_1 = D11;

const int RED_2 = D10;
const int YEL_2 = D9;
const int GRN_2 = D8;

const int RED_3 = D7;
const int YEL_3 = D6;
const int GRN_3 = D5;

// IR Sensors
// LOW = detected
// HIGH = clear

const int IR_1 = D2;
const int IR_2 = D3;
const int IR_3 = D4;

// Ambient Light Sensor

const int LDR_PIN = A0;

// Onboard green LED → PA5
// PWM via TIM2/CH1

const int AMBIENT_LED = D13;


// =======================================================
// TIMING CONFIGURATION
// =======================================================

const unsigned long GREEN_BASE = 5000;     // Base green = 5s
const unsigned long GREEN_EXT = 10000;     // Extended green = 10s
const unsigned long YELLOW_PAUSE = 1000;   // Yellow = 1s


// =======================================================
// LDR CONFIGURATION
// =======================================================

// STM32 F446RE ADC is 12-bit (0–4095)
// Arduino Uno ADC is 10-bit (0–1023)
//
// Original thresholds scaled by x4:
// 800  → 3200
// 200  → 800

const int LDR_DARK = 3200;
const int LDR_BRIGHT = 800;


// =======================================================
// SETUP
// =======================================================

void setup()
{
    Serial.begin(9600);

    // USART2 → ST-Link USB virtual COM port
    // Remove this if board should run standalone
    while (!Serial);


    // ---------------------------------------------------
    // Signal output pins
    // ---------------------------------------------------

    pinMode(RED_1, OUTPUT);
    pinMode(YEL_1, OUTPUT);
    pinMode(GRN_1, OUTPUT);

    pinMode(RED_2, OUTPUT);
    pinMode(YEL_2, OUTPUT);
    pinMode(GRN_2, OUTPUT);

    pinMode(RED_3, OUTPUT);
    pinMode(YEL_3, OUTPUT);
    pinMode(GRN_3, OUTPUT);


    // ---------------------------------------------------
    // IR input pins
    // ---------------------------------------------------

    // INPUT_PULLUP is used because most IR obstacle
    // modules are active-LOW open-drain.
    //
    // This removes the need for an external pull-up
    // resistor.

    pinMode(IR_1, INPUT_PULLUP);
    pinMode(IR_2, INPUT_PULLUP);
    pinMode(IR_3, INPUT_PULLUP);


    // ---------------------------------------------------
    // Ambient LED
    // ---------------------------------------------------

    pinMode(AMBIENT_LED, OUTPUT);


    // ---------------------------------------------------
    // STM32-specific resolution settings
    // ---------------------------------------------------

    // Use full 12-bit ADC
    // Range: 0–4095

    analogReadResolution(12);

    // Keep PWM range at 0–255

    analogWriteResolution(8);


    // Set all signals to RED initially

    allRed();


    // ---------------------------------------------------
    // Serial information
    // ---------------------------------------------------

    Serial.println("=== Traffic Controller Started (STM32 F446RE) ===");

    Serial.println("  Base green     : 5s");
    Serial.println("  Car detected   : extended to 10s");
    Serial.println("  After done     : resets to 5s");
    Serial.println("  Ambient LED    : bright=dim, dark=bright");
    Serial.println("  ADC resolution : 12-bit (0-4095)");

    Serial.println("=================================================");
    Serial.println();

    delay(1000);
}


// =======================================================
// UPDATE AMBIENT LED
// =======================================================
//
// Reads the LDR and changes the brightness of the
// onboard LED.
//
// Bright room → LED becomes dim
// Dark room   → LED becomes bright
// =======================================================

void updateAmbientLED()
{
    // Read LDR value
    // Returns 0–4095 on STM32

    int ldrVal = analogRead(LDR_PIN);


    // Clamp the value within the expected range

    ldrVal = constrain(
        ldrVal,
        LDR_BRIGHT,
        LDR_DARK
    );


    // Invert brightness:
    //
    // Low LDR value  → bright room → low LED brightness
    // High LDR value → dark room  → high LED brightness

    int brightness = map(
        ldrVal,
        LDR_BRIGHT,
        LDR_DARK,
        0,
        255
    );


    // Set LED brightness
    // PWM range = 0–255

    analogWrite(
        AMBIENT_LED,
        brightness
    );
}


// =======================================================
// RUN GREEN PHASE
// =======================================================
//
// Starts with 5 seconds.
//
// If IR detects a car:
//     Green time becomes 10 seconds.
//
// Extension happens only once per green phase.
//
// Ambient LED is updated continuously.
//
// After the phase completes, the next signal starts.
// =======================================================

void runGreen(int signalNum, int irPin)
{
    unsigned long timeLeft = GREEN_BASE;

    unsigned long lastTick = millis();

    bool extended = false;

    int lastPrintedSec = -1;


    // ---------------------------------------------------
    // Start message
    // ---------------------------------------------------

    Serial.print(">> Signal ");
    Serial.print(signalNum);
    Serial.println(" GREEN  |  5s");

    Serial.println("----------------------------------");


    // ---------------------------------------------------
    // Green countdown loop
    // ---------------------------------------------------

    while (timeLeft > 0)
    {
        unsigned long now = millis();

        unsigned long delta = now - lastTick;

        lastTick = now;


        // ------------------------------------------------
        // Update remaining time
        // ------------------------------------------------

        if (delta >= timeLeft)
        {
            timeLeft = 0;
        }
        else
        {
            timeLeft -= delta;
        }


        // ------------------------------------------------
        // Update ambient LED
        // ------------------------------------------------

        updateAmbientLED();


        // ------------------------------------------------
        // IR CHECK
        // ------------------------------------------------
        //
        // Car detected → extend green time to 10 seconds
        // only once per phase.
        // ------------------------------------------------

        if (digitalRead(irPin) == LOW && !extended)
        {
            timeLeft = GREEN_EXT;

            extended = true;

            lastPrintedSec = -1;


            Serial.print("  *** S");
            Serial.print(signalNum);
            Serial.println(
                " car detected → extended to 10s ***"
            );
        }


        // ------------------------------------------------
        // Countdown print once per second
        // ------------------------------------------------

        int currentSec = (int)(timeLeft / 1000);


        if (currentSec != lastPrintedSec)
        {
            lastPrintedSec = currentSec;


            // Read LDR value

            int ldrNow = analogRead(LDR_PIN);


            // Calculate current LED brightness

            int briNow = map(
                constrain(
                    ldrNow,
                    LDR_BRIGHT,
                    LDR_DARK
                ),
                LDR_BRIGHT,
                LDR_DARK,
                0,
                255
            );


            // ------------------------------------------------
            // Serial output
            // ------------------------------------------------

            Serial.print("  S");
            Serial.print(signalNum);

            Serial.print(" GREEN ");

            Serial.print(currentSec + 1);

            Serial.print("s  |  IR: ");

            Serial.print(
                digitalRead(irPin) == LOW
                    ? "DETECTED"
                    : "clear"
            );

            Serial.print("  |  LDR: ");

            Serial.print(ldrNow);

            Serial.print("  LED brightness: ");

            Serial.println(briNow);
        }


        // Small delay to prevent excessive CPU usage

        delay(50);
    }


    // ---------------------------------------------------
    // Green phase completed
    // ---------------------------------------------------

    Serial.print("  S");
    Serial.print(signalNum);
    Serial.println(" GREEN done");

    Serial.println();
}


// =======================================================
// RUN YELLOW PHASE
// =======================================================
//
// fromSignal → signal whose green just ended
// toSignal   → next signal
//
// Both yellow signals are ON during handoff.
// =======================================================

void runYellow(int fromSignal, int toSignal)
{
    Serial.print(">> S");
    Serial.print(fromSignal);

    Serial.print(" + S");
    Serial.print(toSignal);

    Serial.println(" YELLOW");


    unsigned long start = millis();

    int lastSec = -1;


    // ---------------------------------------------------
    // Yellow countdown
    // ---------------------------------------------------

    while (millis() - start < YELLOW_PAUSE)
    {
        // Keep ambient LED responsive

        updateAmbientLED();


        // Calculate remaining time

        unsigned long remaining =
            YELLOW_PAUSE - (millis() - start);


        int sec = (int)(remaining / 1000);


        // Print countdown only when second changes

        if (sec != lastSec)
        {
            lastSec = sec;


            Serial.print("  YELLOW ");

            Serial.print(sec + 1);

            Serial.println("s");
        }


        delay(50);
    }


    Serial.println();
}


// =======================================================
// ALL RED
// =======================================================
//
// Sets all three traffic signals to RED.
// =======================================================

void allRed()
{
    // Signal 1

    digitalWrite(RED_1, HIGH);
    digitalWrite(YEL_1, LOW);
    digitalWrite(GRN_1, LOW);


    // Signal 2

    digitalWrite(RED_2, HIGH);
    digitalWrite(YEL_2, LOW);
    digitalWrite(GRN_2, LOW);


    // Signal 3

    digitalWrite(RED_3, HIGH);
    digitalWrite(YEL_3, LOW);
    digitalWrite(GRN_3, LOW);
}


// =======================================================
// MAIN LOOP
// =======================================================

void loop()
{
    // ===================================================
    // SIGNAL 1
    // ===================================================

    digitalWrite(RED_1, LOW);
    digitalWrite(GRN_1, HIGH);


    // Run S1 green phase
    // Base = 5s
    // IR detection = 10s

    runGreen(1, IR_1);


    // ---------------------------------------------------
    // S1 → S2 YELLOW HANDOFF
    // ---------------------------------------------------

    digitalWrite(GRN_1, LOW);

    digitalWrite(YEL_1, HIGH);

    digitalWrite(RED_2, LOW);

    digitalWrite(YEL_2, HIGH);


    runYellow(1, 2);


    // Finish S1 → S2 transition

    digitalWrite(YEL_1, LOW);

    digitalWrite(RED_1, HIGH);

    digitalWrite(YEL_2, LOW);


    // ===================================================
    // SIGNAL 2
    // ===================================================

    digitalWrite(GRN_2, HIGH);


    // Run S2 green phase

    runGreen(2, IR_2);


    // ---------------------------------------------------
    // S2 → S3 YELLOW HANDOFF
    // ---------------------------------------------------

    digitalWrite(GRN_2, LOW);

    digitalWrite(YEL_2, HIGH);

    digitalWrite(RED_3, LOW);

    digitalWrite(YEL_3, HIGH);


    runYellow(2, 3);


    // Finish S2 → S3 transition

    digitalWrite(YEL_2, LOW);

    digitalWrite(RED_2, HIGH);

    digitalWrite(YEL_3, LOW);


    // ===================================================
    // SIGNAL 3
    // ===================================================

    digitalWrite(GRN_3, HIGH);


    // Run S3 green phase

    runGreen(3, IR_3);


    // ---------------------------------------------------
    // S3 → S1 YELLOW HANDOFF
    // ---------------------------------------------------

    digitalWrite(GRN_3, LOW);

    digitalWrite(YEL_3, HIGH);

    digitalWrite(RED_1, LOW);

    digitalWrite(YEL_1, HIGH);


    runYellow(3, 1);


    // Finish S3 → S1 transition

    digitalWrite(YEL_3, LOW);

    digitalWrite(RED_3, HIGH);

    digitalWrite(YEL_1, LOW);
}