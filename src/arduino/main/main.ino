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

// PIN definitions
const uint8_t M1_DIR_PIN    = 7;    // Motor direction
const uint8_t M1_STEP_PIN   = 6;    // Motor step
const uint8_t M1_EN_PIN     = 8;    // Motor enable
const uint8_t HALL_PIN      = A0;   // Hall sensor
const uint8_t RF_PIN        = A15;  // Radiofrequency module

// Constants definitions
const size_t samples        = 10;   // Number of samples that are read for
                                        // each request
enum motor_direction { RIGHT = LOW, LEFT = HIGH };

// Function prototypes
void home_motor_to_origin();
void rotate_motor_to_sample();
void rotate_motor_step(motor_direction dir);
void capture_sensor_data(uint16_t *sensor_values, size_t samples);
void transmit_sensor_data(uint16_t *sensor_values, size_t samples);
motor_direction flip_direction(const motor_direction direction);

void setup()
{
    // Setting up the pins
    pinMode(M1_DIR_PIN, OUTPUT);
    pinMode(M1_STEP_PIN, OUTPUT);
    pinMode(M1_EN_PIN, OUTPUT);
    pinMode(HALL_PIN, INPUT);
    pinMode(RF_PIN, INPUT);
    
    // Activate the motor and set initial direction
    digitalWrite(M1_EN_PIN, LOW);
    digitalWrite(M1_DIR_PIN, LOW);

    // Setting up the serial port
    Serial.setTimeout(1000);
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("Serial port initialized successfully!");

    // Set motor to the origin
    home_motor_to_origin();
}

void loop()
{
    /* State machine to handle different modes based on serial input
     *      - mode 0: Waiting for serial input. Once the input is received, it
     *                switches to the next mode.
     *      - mode 1: Rotates the motors and samples the sensor values. After
     *                completing, it sends the values via serial port and 
     *                resets to mode 0.
     */
    static int mode = 0;
    switch (mode)
    {
        case 0:
            if (Serial.available() > 0)
            {
                mode = Serial.parseInt();
            }
            break;
        case 1:
            uint16_t sensor_values[samples] = {0};
            rotate_motor_to_sample();
            capture_sensor_data(sensor_values, samples);
            transmit_sensor_data(sensor_values, samples);
            
            mode = 0;
            break;
        default:
            char error[64];
            sprintf(error, "Unknown command: %d. Setting back to listen mode",
                            mode);
            Serial.println(error);
            
            mode = 0;
            break;
    }
}

void home_motor_to_origin()
{
    Serial.println("Starting search for motor origin...");

    /* Homes the motor to its origin position by:
     *      1. Rotating the motor until the Hall sensor detects a magnetic
     *         threshold. If the sensor already detects a value bellow the
     *         magnetic threshold, rotate the motor backwards until it doesn't
     *         detect it anymore.
     *      2. Finding the center point of the detected magnetic range.
     *      3. Returning the motor to this center position.
     */

    motor_direction default_direction = RIGHT;
    const uint16_t  THRESHOLD         = 50;
    const uint16_t  MAX_STEPS         = 3200; // 16 * 200
    
    uint16_t        steps_completed   = 0;
    int16_t         start_step        = -1;
    int16_t         end_step          = -1;

    // Special case where the sensor might already be detecting the magnet,
    // so the motor should rotate backwards until it doesn't detect it anymore
    while (analogRead(HALL_PIN) < THRESHOLD) 
    { 
        rotate_motor_step(flip_direction(default_direction)); 
    }

    for (; steps_completed < MAX_STEPS; steps_completed++)
    {
        uint16_t sensor_value = analogRead(HALL_PIN);

        if (sensor_value < THRESHOLD && start_step == -1)
        {
            start_step = steps_completed;
        }
        else if (sensor_value >= THRESHOLD && start_step != -1)
        {
            end_step = steps_completed;
            break;
        }
        rotate_motor_step(default_direction);
    }

    uint16_t central_step = (start_step + end_step) / 2;
    for (; central_step >= 0; central_step--)
    {
        rotate_motor_step(flip_direction(default_direction));
    }

    Serial.println("Motor homed.");
}

void rotate_motor_to_sample()
{
    static const uint8_t    MICROSTEPS_TO_DEG   = 16;
    static const uint8_t    ANGLE_BOUND         = 180;
    static int16_t          current_angle       = 0;
    static motor_direction  direction           = RIGHT;
   
    // Set the motor back to home if it reached the limits
    if (current_angle == ANGLE_BOUND || current_angle == -ANGLE_BOUND)
    {
        bool resetting_from_min_angle = (current_angle == -ANGLE_BOUD);

        direction = flip_direction(direction);
        for (size_t i = 0; i < MICROSTEPS_TO_DEG * current_angle; i++)
        {
            rotate_motor_step(direction);
        }
        current_angle = 0;
    
        // Don't rotate the motor by 1º once it returns from the -180º limit,
        // forcing it to start sampling back from 0º
        if (resetting_from_min_angle)
        {
            return;
        }
    }

    // Rotate the motor by 1º in the current direction
    for (size_t i = 0; i < MICROSTEPS_TO_DEG; i++)
    {
        rotate_motor_step(direction);
    }
    current_angle += (direction == RIGHT) ? 1 : -1;
}

void rotate_motor_step(const motor_direction dir)
{
    digitalWrite(M1_DIR_PIN, dir);

    digitalWrite(M1_STEP_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(M1_STEP_PIN, LOW);
    delayMicroseconds(1000);
}

void capture_sensor_data(uint16_t *sensor_values, size_t samples)
{
    for (size_t i = 0; i < samples; i++) 
    {
        sensor_values[i] = analogRead(RF_PIN);
        delay(10);
    }
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

motor_direction flip_direction(const motor_direction direction)
{
  if (direction == RIGHT)
  {
    return LEFT;
  }
  else
  {
    return RIGHT;
  }
}
