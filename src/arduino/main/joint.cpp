/*
 * File     : joint.cpp
 * Author   : Mauro Leme
 * Date     : March 31, 2024
 * Purpose  : Implements the functions declared in `joint.h` to control 
 *            a robotic joint. Provides motor movement, homing routines, 
 *            direction control, and sensor reading functionality.
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


// Setting up the joint
void joint_init(joint_t *joint)
{
    // Configure motor pins as output
    (*(portModeRegister(digitalPinToPort(joint->step_pin))) = OUTPUT);
    (*(portModeRegister(digitalPinToPort(joint->dir_pin)))  = OUTPUT);
    (*(portModeRegister(digitalPinToPort(joint->en_pin)))   = OUTPUT);
    
    // Configure hall sensor pin as input
    (*(portModeRegister(digitalPinToPort(joint->hall_pin))) = INPUT_PULLUP);
}

// Toggling motor
void joint_enable_motor(joint_t *joint)
{
    *(portOutputRegister(digitalPinToPort(joint->en_pin))) &=
        ~_BV(digitalPinToBitMask(joint->en_pin));
}

void joint_disable_motor(joint_t *joint)
{
    *(portOutputRegister(digitalPinToPort(joint->en_pin))) |=
        _BV(digitalPinToBitMask(joint->en_pin));
}

// Motor direction
static void _Joint_set_motor_direction(joint_t *joint,
                                       const motor_direction direction)
{
    *(portOutputRegister(digitalPinToPort(joint->dir_pin))) =
        (direction == CW) ?
        (*(portOutputRegister(digitalPinToPort(joint->dir_pin))) &
         ~_BV(digitalPinToBitMask(joint->dir_pin))) :
        (*(portOutputRegister(digitalPinToPort(joint->dir_pin))) |
         _BV(digitalPinToBitMask(joint->dir_pin)));
}

void joint_set_default_motor_direction(const motor_direction direction)
{
    DEFAULT_DIRECTION = direction; 
}

inline motor_direction joint_get_default_direction()
{
    return DEFAULT_DIRECTION;
}

// Motor rotation
static void _Joint_step_motor(joint_t *joint, 
                              const motor_direction direction)
{
    joint_enable_motor(joint);
    _Joint_set_motor_direction(joint, direction); 

    *(portOutputRegister(digitalPinToPort(joint->step_pin))) &=
        ~_BV(digitalPinToBitMask(joint->step_pin));
    delayMicroseconds(DELTA_T);
    *(portOutputRegister(digitalPinToPort(joint->step_pin))) |=
        _BV(digitalPinToBitMask(joint->step_pin));
    delayMicroseconds(DELTA_T);

    MOTOR_LAST_ACTIVE = micros();
}

void joint_rotate_motor(joint_t *joint, int32_t target_angle)
{
    target_angle              = (target_angle % 360 + 360) % 360;
    int32_t         diff      = (target_angle - ANGLE + 540) % 360 - 180;    
    uint32_t        steps     = DEG_TO_STEP((diff ^ (diff >> 31)) - 
                                            (diff >> 31));
    motor_direction direction = (diff >= 0) ? DEFAULT_DIRECTION :
                                              !DEFAULT_DIRECTION;

    for (uint32_t i = 0; i < steps; i++)
    {
        _Joint_step_motor(joint, direction);
    }
    ANGLE = target_angle;
}

// Motor homing
bool joint_home_motor(joint_t *joint)
{
    uint16_t steps_completed = 0;
    uint16_t start_step      = 0;
    uint16_t end_step;

    // Case where the sensor is already detecting the magnet, so the motor
    // rotates backwards until it doesn't detect it anymore
    while (joint_read_hall(joint)) 
    { 
        _Joint_step_motor(joint, (motor_direction)!DEFAULT_DIRECTION);
    }

    // Find the start and end of the magnet
    do
    {
        _Joint_step_motor(joint, DEFAULT_DIRECTION);
        steps_completed++;
        
        delayMicroseconds(HOMING_DELAY);
        
        bool hall_state  = joint_read_hall(joint);
        start_step      |= hall_state * (!start_step * steps_completed); 
        end_step         = steps_completed * (!hall_state && start_step);
    } while (steps_completed < MAX_HOMING_STEPS);

    // If the magnet was not found, throw an error
    if (steps_completed == MAX_HOMING_STEPS)
    {
        return false;
    }

    // Move to the center of the magnet
    uint16_t central_steps = (end_step - start_step) >> 1;
    while (central_steps--) 
    {
        _Joint_step_motor(joint, (motor_direction)!DEFAULT_DIRECTION);
        delayMicroseconds(HOMING_DELAY);
    }

    return true;
}

// Sleep motor
void joint_sleep_motor_after_timeout(joint_t *joint)
{
    if (micros() - MOTOR_LAST_ACTIVE >= MOTOR_SLEEP_TIMEOUT)
    {
        joint_disable_motor(joint);
    }
}

// Hall reading
bool joint_read_hall(joint_t *joint)
{
    return bitRead(*(portInputRegister(digitalPinToPort(joint->hall_pin))),
                   digitalPinToBitMask(joint->hall_pin));
}
