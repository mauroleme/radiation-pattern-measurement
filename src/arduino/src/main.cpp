/*
 * File     : main.cpp
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
const uint8_t M1_DIR_PIN        = 7;    // Motor direction
const uint8_t M1_STEP_PIN       = 6;    // Motor step
const uint8_t M1_EN_PIN         = 8;    // Motor enable
const uint8_t HALL_PIN          = A0;   // Hall sensor
const uint8_t RF_PIN            = A15;  // Radiofrequency module

// Constants definitions
const int revolution_steps      = 1600; // Number of micro-steps to reach 180
                                        // degrees with 1/16 microstepping

const size_t samples             = 10;  // Number of samples that are read for
                                        // each request

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
    set_motor_to_origin();
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
            perform_sampling(&sensor_values, samples);
            serial_write(&sensor_values, samples);
            
            mode = 0;
            break;
        default:
            Serial.println("Unknown command. Setting back to listen mode");
            
            mode = 0;
            break;
    }
}

void set_motor_to_origin()
{
    // TODO: Implement function
}

void perform_sampling(uint16_t *sensor_values, size_t samples)
{
    // TODO: Implement function
}

void serial_write(uint16_t *sensor_values, size_t samples)
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
        index += sprintf(&buffer[index], "%u", sensor_values[i]);
        buffer[index++] = ',';
    }
    
    if (index)
    {
        buffer[--index] = '\0';
    }
    
    Serial.println(buffer);
}
