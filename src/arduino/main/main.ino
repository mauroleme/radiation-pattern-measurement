/*
 * File     : main.ino
 * Author   : Mauro Leme
 * Date     : December 12, 2024
 * Purpose  : Main control loop for the robot's joint system. 
 *            Handles serial communication, processes motor rotation 
 *            requests, captures sensor data, and manages state transitions.
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

#include <Arduino.h>
#include "joint.h"


// ===================================
// Configuration Constants
// ===================================

#define        J1_STEP_PIN PD6
#define        J1_DIR_PIN  PD7
#define        J1_EN_PIN   PB0
#define        J1_HALL_PIN PC0

#define        J2_STEP_PIN PD5
#define        J2_DIR_PIN  PD4
#define        J2_EN_PIN   PB4
#define        J2_HALL_PIN PC1

#define        RF_PIN      A2


const size_t   SAMPLES  = 10;
const uint16_t BUF_SIZE = 32;


// ===================================
// Motor Control Structures
// ===================================

typedef enum { LISTEN = 0, PROCESS = 1 } mode_t;

Joint joint1 =
{
    J1_STEP_PIN,
    J1_DIR_PIN,
    J1_EN_PIN,
    J1_HALL_PIN
};

joint_t joint2 = 
{
    J2_STEP_PIN,
    J2_DIR_PIN,
    J2_EN_PIN,
    J2_HALL_PIN
};


// ===================================
// Function Prototypes
// ===================================

inline void capture_sensor_data(uint16_t *sensor_values, size_t samples);
void transmit_sensor_data(uint16_t *sensor_values, size_t samples);
inline void sleep_joints_after_timeout();
inline void log_error(const char *message);


void setup()
{
    // Setting up the PINs 
    joint1.Init();
    joint2.Init();
    pinMode(RF_PIN, INPUT);

    // Activate the motors
    joint1.EnableMotor();
    joint2.EnableMotor();

    // Setting up the serial port
    Serial.setTimeout(1000);
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Serial port initialized successfully!");

    // Set M1 to the origin
    if (joint1.HomeMotor() == false)
    {
        joint1.DisableMotor();
        log_error("Failed to detect the magnet center of MOTOR 1");
        while (true);
    }
    
    // Set M2 to the origin
    if (joint2.HomeMotor() == false)
    {
        joint2.DisableMotor();
        log_error("Failed to detect the magnet center of MOTOR 2");
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
    static int32_t target_angle_joint1;
    static int32_t target_angle_joint2;
    static char    buf[BUF_SIZE];
    
    static mode_t mode = LISTEN;
    if (mode == LISTEN)
    {
        if (Serial.available() > 0)
        {
            int32_t temp_target_angle_joint1;
            int32_t temp_target_angle_joint2;
            size_t  len = Serial.readBytesUntil('\n', buf, BUF_SIZE - 1);
            buf[len]    = '\0';

            if (sscanf(buf, "%ld,%ld", &temp_target_angle_joint1, 
                                       &temp_target_angle_joint2) == 2)
            {
                target_angle_joint1 = temp_target_angle_joint1;
                target_angle_joint2 = temp_target_angle_joint2;

                mode = PROCESS;
            }
            else
            {
                log_error("Invalid input format");
            }
        }
    }
    else if (mode == PROCESS)
    {
        joint1.RotateMotor(target_angle_joint1);
        joint2.RotateMotor(target_angle_joint2);
        
        uint16_t sensor_values[SAMPLES] = { 0 };
        capture_sensor_data(sensor_values, SAMPLES);
        transmit_sensor_data(sensor_values, SAMPLES);
            
        mode = LISTEN;
    }
    else
    {
        log_error("Unknown mode");
        
        mode = LISTEN;
    }

    sleep_joints_after_timeout();
}

inline void capture_sensor_data(uint16_t *sensor_values, size_t samples)
{
    for (size_t i = 0; i < samples; i++) 
    {
        sensor_values[i] = analogRead(RF_PIN);
        delayMicroseconds(10000);
    }
}

void transmit_sensor_data(uint16_t *sensor_values, size_t samples)
{
    /* The buffer contains the values read from the sensor separated by commas:
     *      - 5 chars for each number;
     *      - 1 char for each comma;
     *      - 1 char for null terminator.
     */
    char   buffer[samples * 6];
    size_t index = 0;
    
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

inline void sleep_joints_after_timeout()
{
    joint1.SleepMotorAfterTimeout();
    joint2.SleepMotorAfterTimeout();
}

inline void log_error(const char *message)
{
    char error[128];
    snprintf(error, sizeof(error), "Error: %s.", message);
    Serial.println(error);
}
