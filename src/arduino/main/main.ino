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

#define J1_STEP_PIN 6
#define J1_DIR_PIN  7
#define J1_EN_PIN   8
#define J1_HALL_PIN A0

#define J2_STEP_PIN 5
#define J2_DIR_PIN  4
#define J2_EN_PIN   12
#define J2_HALL_PIN A1

#define RF_PIN      A3


const size_t   SAMPLES     = 100;
const uint16_t BUFFER_SIZE = 32;

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

Joint joint2 = 
{
    J2_STEP_PIN,
    J2_DIR_PIN,
    J2_EN_PIN,
    J2_HALL_PIN
};


// ===================================
// Function Prototypes
// ===================================

void capture_sensor_data(uint16_t *sensor_values, size_t samples);
void transmit_sensor_data(uint16_t *sensor_values, size_t samples);
inline void sleep_joints_after_timeout();
inline void log_error(const char *message);


void setup()
{
    // Setting up the serial port
    Serial.setTimeout(1000);
    Serial.begin(115200);
    while (!Serial);
    
    Serial.println("Ready?");
    
    // Wait for MATLAB command for initialization
    {
        char buffer[10] = { 0 };
        while (1)
        {
            if (Serial.available() > 0)
            {
                size_t len = Serial.readBytesUntil('\n', buffer,
                                                   sizeof(buffer));
                buffer[len] = '\0';
                if (strncmp(buffer, "Set.", 4) == 0)
                    break;
            }
        }
    }

    // Setting up the PINs 
    joint1.Init();
    joint2.Init();
    pinMode(RF_PIN, INPUT);

    // Activate the motors
    joint1.EnableMotor();
    joint2.EnableMotor();

    // Set M1 to the origin
    if (joint1.HomeMotor() == false)
    {
        joint1.DisableMotor();
        log_error("Failed to detect the magnet center of MOTOR 1");
        while (true);
    }
    
    // Set M2 to the origin
    /*
    if (joint2.HomeMotor() == false)
    {
        joint2.DisableMotor();
        log_error("Failed to detect the magnet center of MOTOR 2");
        while (true);
    }
    */

    // Signal MATLAB to begin requesting sample data
    Serial.println("Go.");
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
    static char    buffer[BUFFER_SIZE];
    
    static mode_t mode = LISTEN;
    if (mode == LISTEN)
    {
        if (Serial.available() > 0)
        {
            int32_t temp_target_angle_joint1;
            int32_t temp_target_angle_joint2;
            size_t  len = Serial.readBytesUntil('\n', buffer, BUFFER_SIZE - 1);
            buffer[len] = '\0';

            if (sscanf(buffer, "%ld,%ld", &temp_target_angle_joint1, 
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
        
        double sensor_values[SAMPLES] = { 0 };
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

void capture_sensor_data(double *sensor_values, size_t samples)
{
    const size_t WAIT_TIME          = 1000; // Time in milliseconds
    const size_t TOTAL_CAPTURE_TIME = 100;  // Time in milliseconds

    // Standby
    delay(WAIT_TIME - TOTAL_CAPTURE_TIME);
    
    size_t capture_time_per_sample = TOTAL_CAPTURE_TIME / samples;
    for (size_t i = 0; i < samples; i++) 
    {
        int    raw_rf_value = analogRead(RF_PIN);
        double power_dBm    = (double)raw_rf_value * 0.2722 - 97.115;
        
        sensor_values[i] = power_dBm;
        delay(capture_time_per_sample);
    }
}

void transmit_sensor_data(double *sensor_values, size_t samples)
{
    char   buffer[8 * samples]; 
    size_t index = 0;

    for (size_t i = 0; i < samples; i++)
    {
        char temp[10];
        dtostrf(sensor_values[i], 5, 2, temp);
        index += sprintf(&buffer[index], "%s%s", temp,
                         (i < samples - 1) ? "," : "");
    }

    Serial.println(buffer);
}

inline void sleep_joints_after_timeout()
{
    joint1.SleepMotorAfterTimeOut();
    joint2.SleepMotorAfterTimeOut();
}

inline void log_error(const char *message)
{
    char error[128];
    snprintf(error, sizeof(error), "Error: %s.", message);
    Serial.println(error);
}
