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
// Macros
// ===================================

#define DEG_TO_STEP(deg)  ((deg) * steps_per_degree)
#define STEP_TO_DEG(step) ((step) / steps_per_degree)


// ===================================
// Configuration Constants
// ===================================

const uint16_t DELTA_T             = 1000;
const uint16_t HOMING_DELAY        = 10000;
const uint32_t MOTOR_SLEEP_TIMEOUT = 10000000;


// ===================================
// Types
// ===================================

typedef enum { CW = LOW, CCW = HIGH } motor_direction;


// ===================================
// Class Declaration 
// ===================================

class Joint
{
    public:
        Joint(uint8_t step_pin, uint8_t dir_pin,
              uint8_t en_pin, uint8_t hall_pin)
            : step_pin_(step_pin), dir_pin_(dir_pin)
            , en_pin_(en_pin), hall_pin_(hall_pin)
            , angle(0), motor_last_active(0), default_direction(CW)
            , steps_per_degree(16)
            {}
        void Init();
        void EnableMotor();
        void DisableMotor();
        void SetDefaultMotorDirection(const motor_direction target_direction);
        motor_direction GetDefaultMotorDirection(); 
        void RotateMotor(const int32_t target_angle);
        void SleepMotorAfterTimeOut();
        bool HomeMotor();
        void SetStepsPerDegree(const uint16_t target_steps_per_degree);
        uint16_t GetStepsPerDegree();
        bool ReadHall();

    private:
        void SetMotorDirection(const motor_direction target_direction);
        void StepMotor(const motor_direction target_direction);
        
        // ===================================
        // Joint State
        // ===================================
        
        uint16_t        steps_per_degree;
        int32_t         angle;
        uint32_t        motor_last_active;
        motor_direction default_direction;
        
        // ===================================
        // Hardware Pins
        // ===================================
        
        uint8_t         step_pin_;
        uint8_t         dir_pin_;
        uint8_t         en_pin_;
        uint8_t         hall_pin_;
};

#endif // JOINT_H
