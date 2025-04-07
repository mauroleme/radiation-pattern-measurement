/*
 * File     : joint.h
 * Author   : Mauro Leme
 * Date     : March 31, 2024
 * Purpose  : Defines the data structures and function prototypes 
 *            for controlling a robotic joint using a stepper motor.
 *            Includes motor control, direction handling, homing,
 *            and hall sensor reading.
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

#ifndef JOINT_H
#define JOINT_H

#include <Arduino.h>


// ===================================
// Configuration Constants
// ===================================

static const uint16_t DELTA_T             = 100;
static const uint16_t HOMING_DELAY        = 10000;
static const uint32_t MOTOR_SLEEP_TIMEOUT = 10000000;

// Steps per degree (modifiable at runtime)
static uint16_t       STEPS_PER_DEGREE    = 16;


// ===================================
// Macros
// ===================================

#define DEG_TO_STEP(deg)  ((deg) * STEPS_PER_DEGREE)
#define STEP_TO_DEG(step) ((step) / STEPS_PER_DEGREE)


// ===================================
// Types
// ===================================

typedef enum { CW = LOW, CCW = HIGH } motor_direction;

typedef struct
{
    // Motor pins
    uint8_t step_pin;
    uint8_t dir_pin;
    uint8_t en_pin;

    // Hall pin
    uint8_t hall_pin;
} joint_t;


// ===================================
// Global State
// ===================================

static volatile int32_t angle             = 0;
static uint32_t         motor_last_active = 0;
static motor_direction  default_direction = CW;


// ===================================
// Setup
// ===================================

void joint_init(joint_t *joint);


// ==============================
// Motor Toggling 
// ==============================

void joint_enable_motor(joint_t *joint);
void joint_disable_motor(joint_t *joint);


// ==============================
// Motor Direction
// ==============================

void joint_set_default_motor_direction(const motor_direction direction);
motor_direction joint_get_default_direction();

// Internal helper
static void _Joint_set_motor_direction(joint_t *joint,
                                       const motor_direction);


// ==============================
// Motor Control
// ==============================

void joint_rotate_motor(joint_t *joint, const int32_t target_angle);
void joint_sleep_motor_after_timeout(joint_t *joint);

// Internal helper
static void _Joint_step_motor(joint_t *joint,
                              const motor_direction direction);


// ==============================
// Motor Homing
// ==============================

bool joint_home_motor(joint_t *joint);


// ==============================
// Motor Steps per Degree
// ==============================

void joint_set_steps_per_degree(const uint16_t target_steps_per_degree);
uint16_t joing_get_steps_per_degree();


// ==============================
// Hall Effect Sensor Reading
// ==============================

bool joint_read_hall(joint_t *joint);

#endif // JOINT_H
