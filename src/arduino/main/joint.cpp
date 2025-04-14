/*
 * File     : joint.cpp
 * Author   : Mauro Leme
 * Date     : March 31, 2024
 * Purpose  : Implements the functions declared in `joint.h' to control 
 *            a robotic joint. Provides motor movement, homing routines, 
 *            direction control and sensor reading functionality.
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

#include "joint.h"


// ===================================
// Setup
// ===================================

void Joint::Init()
{
    // Configure motor pins as output
    *portModeRegister(digitalPinToPort(step_pin_))   |=
        digitalPinToBitMask(step_pin_);
    *portModeRegister(digitalPinToPort(dir_pin_))    |=
        digitalPinToBitMask(dir_pin_);
    *portModeRegister(digitalPinToPort(en_pin_))     |=
        digitalPinToBitMask(en_pin_);
    
    // Configure hall sensor pin as input
    *portModeRegister(digitalPinToPort(hall_pin_))   &=
        ~digitalPinToBitMask(hall_pin_);
    *portOutputRegister(digitalPinToPort(hall_pin_)) |=
        digitalPinToBitMask(hall_pin_);
}


// ==============================
// Motor Toggling 
// ==============================

void Joint::EnableMotor()
{
    *portOutputRegister(digitalPinToPort(en_pin_))   &=
        ~digitalPinToBitMask(en_pin_);
}

void Joint::DisableMotor()
{
    *portOutputRegister(digitalPinToPort(en_pin_))   |=
        digitalPinToBitMask(en_pin_);
}


// ==============================
// Motor Direction
// ==============================

void Joint::SetDefaultMotorDirection(const motor_direction target_direction)
{
    default_direction = target_direction; 
}

motor_direction Joint::GetDefaultMotorDirection()
{
    return default_direction;
}

void Joint::SetMotorDirection(const motor_direction target_direction)
{
    *portOutputRegister(digitalPinToPort(dir_pin_)) =
        (target_direction == CW) ?
        (*portOutputRegister(digitalPinToPort(dir_pin_)) |
        digitalPinToBitMask(dir_pin_)) :
        (*portOutputRegister(digitalPinToPort(dir_pin_)) &
        ~digitalPinToBitMask(dir_pin_));

}


// ==============================
// Motor Control
// ==============================

void Joint::RotateMotor(int32_t target_angle)
{
    target_angle              = (target_angle % 360 + 360) % 360;
    int32_t         diff      = (target_angle - angle + 540) % 360 - 180;    
    uint32_t        steps     = DEG_TO_STEP((diff ^ (diff >> 31)) - 
                                            (diff >> 31));
    motor_direction direction = (diff >= 0) ? default_direction :
                                              !default_direction;

    for (uint32_t i = 0; i < steps; i++)
    {
        StepMotor(direction);
    }
    angle = target_angle;
}

void Joint::SleepMotorAfterTimeOut()
{
    if (micros() - motor_last_active >= MOTOR_SLEEP_TIMEOUT)
    {
        DisableMotor();
    }
}

void Joint::StepMotor(const motor_direction target_direction)
{
    EnableMotor();
    SetMotorDirection(target_direction); 

    *portOutputRegister(digitalPinToPort(step_pin_)) &=
        ~digitalPinToBitMask(step_pin_);
    delayMicroseconds(DELTA_T);
    *portOutputRegister(digitalPinToPort(step_pin_)) |=
        digitalPinToBitMask(step_pin_);
    delayMicroseconds(DELTA_T);

    motor_last_active = micros();
}


// ==============================
// Motor Homing
// ==============================

bool Joint::HomeMotor()
{
    const uint16_t MAX_HOMING_STEPS = DEG_TO_STEP(360);
    uint16_t       steps_completed  = 0;
    uint16_t       start_step       = 0;
    uint16_t       end_step;

    // Case where the sensor is already detecting the magnet, so the motor
    // rotates backwards until it doesn't detect it anymore
    while (ReadHall()) 
    {
        StepMotor((motor_direction)!default_direction);
    }

    // Find the start and end of the magnet
    do
    {
        StepMotor(default_direction);
        steps_completed++;
        
        delayMicroseconds(HOMING_DELAY);
        
        bool hall_state = ReadHall();
        if (hall_state && start_step == 0)
        {
            start_step = steps_completed;
        }
        else if (!hall_state && start_step != 0)
        {
            end_step = steps_completed;
            break;
        }
    }
    while (steps_completed < MAX_HOMING_STEPS);

    // If the magnet was not found, throw an error
    if (steps_completed == MAX_HOMING_STEPS)
    {
        return false;
    }

    // Move to the center of the magnet
    uint16_t central_steps = (end_step - start_step) >> 1;
    while (central_steps--) 
    {
        StepMotor((motor_direction)!default_direction);
        delayMicroseconds(HOMING_DELAY);
    }
    
    // Define angle as the origin
    angle = 0;

    return true;
}


// ==============================
// Motor Steps per Degree
// ==============================

void Joint::SetStepsPerDegree(const uint16_t target_steps_per_degree)
{
    steps_per_degree = target_steps_per_degree;
}

uint16_t Joint::GetStepsPerDegree()
{
    return steps_per_degree;
}


// ==============================
// Hall Effect Sensor Reading
// ==============================

bool Joint::ReadHall()
{
    return !(*portInputRegister(digitalPinToPort(hall_pin_)) &
             digitalPinToBitMask(hall_pin_));
}
