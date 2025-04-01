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

#include "utils.h"


void setup()
{
    // Setting up the pins
    joint_init(&joint1);
    joint_init(&joint2);
    pinMode(RF_PIN, INPUT);

    // Activate the motors
    joint_enable_motor(&joint1);
    joint_enable_motor(&joint2);

    // Setting up the serial port
    Serial.setTimeout(1000);
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Serial port initialized successfully!");

    // Set M1 to the origin
    if (joint_home_motor(&joint1) == false)
    {
        joint_disable_motor(&joint1);
        throw_error("Failed to detect the magnet center of MOTOR 1");
        while (true);
    }
    
    /*
    // Set M2 to the origin
    if (joint_home_motor(&joint2) == false)
    {
        joint_disable_motor(&joint2);
        throw_error("Failed to detect the magnet center of MOTOR 2");
        while (true);
    }
    */

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
                throw_error("Invalid input format");
            }
        }
    }
    else if (mode == PROCESS)
    {
        joint_rotate_motor(&joint1, target_angle_joint1);
        joint_rotate_motor(&joint2, target_angle_joint2);
        
        uint16_t sensor_values[SAMPLES] = { 0 };
        capture_sensor_data(sensor_values, SAMPLES);
        transmit_sensor_data(sensor_values, SAMPLES);
            
        mode = LISTEN;
    }
    else
    {
        throw_error("Unknown mode");
        
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

inline void sleep_joints_after_timeout()
{
    joint_sleep_motor_after_timeout(&joint1);
    joint_sleep_motor_after_timeout(&joint2);
}
