/*
 * File     : utils.h
 * Author   : Mauro Leme
 * Date     : December 12, 2024
 * Purpose  : Provides utility functions and macros for the joint system. 
 *            Includes error handling, pin configuration, and sensor data 
 *            transmission. Improves modularity and simplifies the main 
 *            control logic.
 *
 * License  : MIT License
 *
 * Copyright (c) 2024 mauroleme
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "joint.h"


typedef enum { LISTEN = 0, PROCESS = 1 } mode_t;

// Joint creation
joint_t joint1 =
{
    .step_pin  = PD6,
    .dir_pin   = PD7,
    .en_pin    = PB0,

    .hall_pin  = PC0
};

joint_t joint2 = 

{
    .step_pin  = PD5,
    .dir_pin   = PD4,
    .en_pin    = PB4,

    .hall_pin  = PC1
};

// Radio-frequency detector PIN 
const uint16_t RF_PIN   = A2;

// Constants definitions
const size_t   SAMPLES  = 10;
const uint16_t BUF_SIZE = 32;

// Error reporting function
static inline void throw_error(const char *message)
{
    char error[128];
    snprintf(error, sizeof(error), "Error: %s.", message);
    Serial.println(error);
}

// Function prototypes
inline void capture_sensor_data(uint16_t *sensor_values, size_t samples);
void transmit_sensor_data(uint16_t *sensor_values, size_t samples);
inline void sleep_joints_after_timeout();

#endif // UTILS_H
