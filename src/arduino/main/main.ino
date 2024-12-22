/*
 * File     : main.ino
 * Author   : Mauro Leme
 * Date     : December 12, 2024
 * Purpose  : Retrieves sensor data and sends it via serial port.
 *
 * License  : MIT License
 *
 *            Copyright (c) 2024 mauroleme
 *
 *            Permission is hereby granted, free of charge, to any person obtaining a copy
 *            of this software and associated documentation files (the "Software"), to deal
 *            in the Software without restriction, including without limitation the rights
 *            to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *            copies of the Software, and to permit persons to whom the Software is
 *            furnished to do so, subject to the following conditions:
 *
 *            The above copyright notice and this permission notice shall be included in all
 *            copies or substantial portions of the Software.
 *
 *            THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *            IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *            FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *            AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *            LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *            OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *            SOFTWARE.
 */

#include <Arduino.h>


enum motor_direction    { RIGHT     = LOW   , LEFT      = HIGH };
enum system_state       { LISTEN    = 0     , PROCESS   = 1    };

// Port definitions
#define                 M1_PORT             PORTH
#define                 M1_STEP_BIT         PH3
#define                 M1_DIR_BIT          PH4
#define                 M1_EN_BIT           PH5

// Macros for direct PIN manipulation
#define                 ENABLE_MOTOR()      M1_PORT &= ~_BV(M1_EN_BIT)
#define                 DISABLE_MOTOR()     M1_PORT |= _BV(M1_EN_BIT)
#define                 STEP()              { M1_PORT &= ~_BV(M1_STEP_BIT);     \
                                              delayMicroseconds(100);           \
                                              M1_PORT |= _BV(M1_STEP_BIT); }
#define                 SET_DIR(direction)  do                                  \
                                            {                                   \
                                                if (direction == HIGH)          \
                                                    M1_PORT |= _BV(M1_DIR_BIT); \
                                                else                            \
                                                    M1_PORT &= ~_BV(M1_DIR_BIT);\
                                            } while(0);

// PIN definitions
const uint8_t           M1_STEP_PIN         = 6;    // Motor step
const uint8_t           M1_DIR_PIN          = 7;    // Motor direction
const uint8_t           M1_EN_PIN           = 8;    // Motor enable
const uint8_t           HALL_PIN            = A13;  // Hall sensor
const uint8_t           RF_PIN              = A15;  // Radiofrequency module

// Constants definitions
const int16_t           MAX_ANGLE           = 179;
const int16_t           MIN_ANGLE           = -180;
const uint8_t           MICROSTEPS_TO_DEG   = 16;
const size_t            SAMPLES             = 10;
const motor_direction   DEFAULT_DIRECTION   = RIGHT;

// Function prototypes
bool home_motor_to_origin();
void capture_sensor_data(uint16_t *sensor_values, size_t samples);
void rotate_motor_to_next_sample();
void inline rotate_motor_step(const motor_direction direction);
void transmit_sensor_data(uint16_t *sensor_values, size_t samples);
void inline sleep_motor(const uint32_t last_active); 

void setup()
{
    // Setting up the pins
    pinMode(M1_DIR_PIN, OUTPUT);
    pinMode(M1_STEP_PIN, OUTPUT);
    pinMode(M1_EN_PIN, OUTPUT);
    pinMode(HALL_PIN, INPUT);
    pinMode(RF_PIN, INPUT);
    
    // Activate the motor and set initial direction
    ENABLE_MOTOR();
    SET_DIR(DEFAULT_DIRECTION);

    // Setting up the serial port
    Serial.setTimeout(1000);
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Serial port initialized successfully!");

    // Set motor to the origin
    if (home_motor_to_origin() == false)
    {
        digitalWrite(M1_EN_PIN, HIGH);
        Serial.println("Error: Failed to detect the magnet center.");
        while (true);
    }

    // Signal MATLAB to begin requesting sample data
    Serial.println("Ready.");
}

void loop()
{
    /* State machine to handle different modes based on serial input:
     *      - LISTEN  : Waiting for serial input. Once the input is received,
     *                  it switches to the next mode.
     *      - PROCESS : Rotates the motors and samples the sensor values. After
     *                  completing, it sends the values via serial port and 
     *                  resets to LISTEN.
     */
    static int      mode        = LISTEN;
    static uint32_t last_active = micros();
    
    if (mode == LISTEN)
    {
        if (Serial.available() > 0)
        {
            mode = Serial.parseInt();
        }
    }
    else if (mode == PROCESS)
    {
        uint16_t sensor_values[SAMPLES]  = { 0 };
        
        ENABLE_MOTOR();

        capture_sensor_data(sensor_values, SAMPLES);
        rotate_motor_to_next_sample();
        transmit_sensor_data(sensor_values, SAMPLES);
            
        last_active = micros();
        mode        = LISTEN;
    }
    else
    {
        char error[64];
        sprintf(error, "Unknown command: %d. Setting back to LISTEN mode.",
                       mode);
        Serial.println(error);
            
        mode = LISTEN;
    }

    sleep_motor(last_active);
}

bool home_motor_to_origin()
{
    Serial.println("Starting search for motor origin...");
    
    /* Homes the motor to its origin position by:
     *      - Step 1  : Rotating the motor until the Hall sensor detects a 
     *                  magnetic threshold. 
     *                    - If the sensor is already below the threshold,
     *                    rotate the motor backward until the sensor exits the
     *                    magnetic range.
     *      - Step 2  : Rotate the motor to find the start and end points of 
     *                  the range where the sensor is below the threshold.
     *      - Step 3  : Returning the motor to the center of this range.
     */

    const uint16_t  HOMING_DELAY    = 10000;    // Lowers the velocity
    const uint16_t  HALL_THRESHOLD  = 50;       
    const uint16_t  MAX_STEPS       = 3200;     // 16 * 200
    uint16_t        steps_completed = 0;
    int16_t         start_step      = -1;
    int16_t         end_step        = -1;

    // Case where the sensor is already detecting the magnet, so the motor
    // rotates backwards until it doesn't detect it anymore
    while (analogRead(HALL_PIN) < THRESHOLD) 
    { 
        rotate_motor_step((motor_direction)(!DEFAULT_DIRECTION)); 
    }

    while (steps_completed < MAX_STEPS)
    {
        uint16_t hall_value = analogRead(HALL_PIN);

        if (hall_value < HALL_THRESHOLD && start_step == -1)
        {
            start_step = steps_completed;
        }
        else if (hall_value >= HALL_THRESHOLD && start_step != -1)
        {
            end_step = steps_completed;
            break;
        }
        rotate_motor_step(DEFAULT_DIRECTION);
        delayMicroseconds(HOMING_DELAY);

        steps_completed++;
    }

    // If no center point was reached, throw an error
    if (steps_completed == MAX_STEPS)
    {
        return false;
    }

    for (uint16_t central_step = (end_step - start_step) / 2; central_step > 0;
         central_step--)
    {
        rotate_motor_step((motor_direction)(!DEFAULT_DIRECTION));
        delayMicroseconds(HOMING_DELAY);
    }

    Serial.println("Motor homed.");
    return true;
}

void capture_sensor_data(uint16_t *sensor_values, size_t samples)
{
    for (size_t i = 0; i < samples; i++) 
    {
        sensor_values[i] = analogRead(RF_PIN);
        delayMicroseconds(10000);
    }
}

void rotate_motor_to_next_sample()
{
    static int16_t          current_angle = 0;
    static motor_direction  direction     = DEFAULT_DIRECTION;
   
    // Reset motor to home if angle limits are reached
    if (current_angle == MAX_ANGLE || current_angle == MIN_ANGLE)
    {
        direction = (motor_direction)(!direction);
        // Rotate back to home
        uint16_t abs_current_angle = abs(current_angle);
        for (size_t i = 0; i < MICROSTEPS_TO_DEG * abs_current_angle; i++)
        {
            rotate_motor_step(direction, DEFAULT_VELOCITY);
        }
        current_angle = 0;
    
        // Skip the initial rotation after performing 360°
        if (direction == DEFAULT_DIRECTION)
        {
            return;
        }
    }

    // Rotate the motor by 1° in the current direction
    for (size_t i = 0; i < MICROSTEPS_TO_DEG; i++)
    {
        rotate_motor_step(direction);
    }
    current_angle += (direction == DEFAULT_DIRECTION) ? 1 : -1;
}

void inline rotate_motor_step(const motor_direction direction)
{
    static const uint32_t   MIN_STEP_INTERVAL   = 100;
    static uint32_t         last_step_time      = 0;

    // Make sure the stepper motor has stabilized
    while ((uint32_t)(micros() - last_step_time) < MIN_STEP_INTERVAL);

    SET_DIR(direction);
    STEP();

    last_step_time = micros();
}

void transmit_sensor_data(uint16_t *sensor_values, size_t samples)
{
    /* The buffer contains the values read from the sensor separated by commas:
     *      - 5 chars for each number;
     *      - 1 char for each comma;
     *      - 1 char for null terminator.
     */
    char    buffer[samples * 6];
    size_t  index = 0;
    
    for (size_t i = 0; i < samples; i++)
    {
        index += sprintf(&buffer[index], "%u,", sensor_values[i]);
    }
    
    if (index)
    {
        buffer[--index] = '\0';
    }
    
    Serial.println(buffer);
}

void inline sleep_motor(const uint32_t last_active)
{
    static const uint32_t MOTOR_SLEEP_TIMEOUT = 10000000;
    
    if ((uint32_t)(micros() - last_active) >= MOTOR_SLEEP_TIMEOUT)
    {
        DISABLE_MOTOR();
    }
}
