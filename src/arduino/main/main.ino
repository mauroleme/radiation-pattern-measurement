/*
 * File     : main.ino
 * Author   : Mauro Leme
 * Date     : December 12, 2024
 * Purpose  : Retrieves sensor data and sends it via serial port.
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
    CONFIG_M();
    CONFIG_HALL();
    CONFIG_RF();

    // Activate the motors and set initial direction
    SET_DIR_M1(DEFAULT_DIRECTION);
    SET_DIR_M2(DEFAULT_DIRECTION);
    ENABLE_M();

    // Setting up the serial port
    Serial.setTimeout(1000);
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Serial port initialized successfully!");

    // Set M1 to the origin
    if (home_motor_to_origin(JOINT1) == false)
    {
        DISABLE_M();
        Serial.println("Error: Failed to detect the magnet center.");
        while (true);
    }

    // Set M2 to the origin
    if (home_motor_to_origin(JOINT2) == false)
    {
        DISABLE_M();
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
    static int mode      = LISTEN;
    int        wanted_ang_m1;
    int        wanted_ang_m2;
    
    if (mode == LISTEN)
    {
        if (Serial.available() > 0)
        {
            String input     = Serial.readStringUntil('\n');
            int    comma_ind = input.indexOf(','); 
            wanted_ang_m1    = input.substring(0, comma_ind).toInt();
            wanted_ang_m2    = input.substring(comma_ind + 1).toInt();
            
            mode             = PROCESS;
        }
    }
    else if (mode == PROCESS)
    {
        uint16_t sensor_values[SAMPLES]  = { 0 };
        
        rotate_motor_to_next_sample(wanted_ang_m1, wanted_ang_m2);
        capture_sensor_data(sensor_values, SAMPLES);
        transmit_sensor_data(sensor_values, SAMPLES);
            
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

    sleep_motor();
}

bool home_motor_to_origin(const joint_id joint)
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
    uint16_t        start_step      = 0;
    uint16_t        end_step;
    uint16_t        HALL_PIN;
    
    if (joint == JOINT1)
    {
        HALL_PIN = HALL_M1_BIT;
    }
    else
    {
        HALL_PIN = HALL_M2_BIT;
    }

    // Case where the sensor is already detecting the magnet, so the motor
    // rotates backwards until it doesn't detect it anymore
    while (analogRead(HALL_PIN) < HALL_THRESHOLD) 
    { 
        rotate_motor_step(joint, (motor_direction)(!DEFAULT_DIRECTION)); 
    }

    do
    {
        rotate_motor_step(joint, DEFAULT_DIRECTION);
        delayMicroseconds(HOMING_DELAY);
        steps_completed++;
        
        uint16_t hall_value = analogRead(HALL_PIN);

        if (hall_value < HALL_THRESHOLD && start_step == 0)
        {
            start_step = steps_completed;
        }
        else if (hall_value >= HALL_THRESHOLD && start_step != 0)
        {
            end_step = steps_completed;
            break;
        }
    } while (steps_completed < MAX_STEPS);

    // If no center point was reached, throw an error
    if (steps_completed == MAX_STEPS)
    {
        return false;
    }

    for (uint16_t central_step = (end_step - start_step) / 2; central_step > 0;
         central_step--)
    {
        rotate_motor_step(joint, (motor_direction)(!DEFAULT_DIRECTION));
        delayMicroseconds(HOMING_DELAY);
    }

    Serial.println("Motor homed.");
    return true;
}

void capture_sensor_data(uint16_t *sensor_values, size_t samples)
{
    for (size_t i = 0; i < samples; i++) 
    {
        sensor_values[i] = analogRead(RF_BIT);
        delayMicroseconds(10000);
    }
}

void rotate_motor_to_next_sample(const uint32_t wanted_ang_m1, const uint32_t wanted_ang_m2)
{
    static uint32_t        ANG_M1   = 0;
    static uint32_t        ANG_M2   = 0;
    const  size_t          DELTA_M1 = abs(wanted_ang_m1 - ANG_M1) *
                                      MICROSTEPS_TO_DEG;
    const  size_t          DELTA_M2 = abs(wanted_ang_m2 - ANG_M2) *
                                      MICROSTEPS_TO_DEG;
    const  motor_direction DIR_M1   = wanted_ang_m1 < ANG_M1 ? !DEFAULT_DIRECTION :
                                                            DEFAULT_DIRECTION;
    const  motor_direction DIR_M2   = wanted_ang_m2 < ANG_M2 ? !DEFAULT_DIRECTION :
                                                            DEFAULT_DIRECTION;
    for (size_t i = 0; i < DELTA_M1; i++)
    {
        rotate_motor_step(JOINT1, DIR_M1); 
    }
    for (size_t i = 0; i < DELTA_M2; i++)
    {
        rotate_motor_step(JOINT2, DIR_M2);
    }
}

void inline rotate_motor_step(const joint_id joint, const motor_direction direction)
{
    if (joint == JOINT1)
    {
        ENABLE_M1();
        SET_DIR_M1(direction);
        STEP_M1();

        LAST_ACTIVE_M1 = micros();
    }
    else
    {
        ENABLE_M2();
        SET_DIR_M2(direction);
        STEP_M2();

        LAST_ACTIVE_M2 = micros();
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

void inline sleep_motor()
{
    if ((uint32_t)(micros() - LAST_ACTIVE_M1) >= MOTOR_SLEEP_TIMEOUT)
    {
        DISABLE_M1();
    }
    if ((uint32_t)(micros() - LAST_ACTIVE_M2) >= MOTOR_SLEEP_TIMEOUT)
    {
        DISABLE_M2();
    }
}
