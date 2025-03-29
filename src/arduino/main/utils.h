/*
 * File     : utils.h
 * Author   : Mauro Leme
 * Date     : December 12, 2024
 * Purpose  : Utilities of main.ino
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


enum joint_id           { JOINT1    = 0     , JOINT2    = 1    };
enum motor_direction    { CW        = LOW   , CCW      = HIGH };
enum system_state       { LISTEN    = 0     , PROCESS   = 1    };

// Port definitions
#define                 M1_STEP_BIT         PD6
#define                 M1_DIR_BIT          PD7
#define                 M1_EN_BIT           PB0

#define                 M2_STEP_BIT         PD5
#define                 M2_DIR_BIT          PD4
#define                 M2_EN_BIT           PB4

#define                 HALL_M1_BIT         PC0
#define                 HALL_M2_BIT         PC1

#define                 RF_BIT              PC2

// Macros for direct PIN manipulation
#define                 ENABLE_M1()         PORTB &= ~_BV(M1_EN_BIT)
#define                 DISABLE_M1()        PORTB |= _BV(M1_EN_BIT)
#define                 STEP_M1()           PORTD &= ~_BV(M1_STEP_BIT);        \
                                            delayMicroseconds(DELTAT);         \
                                            PORTD |= _BV(M1_STEP_BIT);         \
                                            delayMicroseconds(DELTAT)
#define                 SET_DIR_M1(dir)     do                                 \
                                            {                                  \
                                                if (dir == CCW)                \
                                                    PORTD |= _BV(M1_DIR_BIT);  \
                                                else                           \
                                                    PORTD &= ~_BV(M1_DIR_BIT); \
                                            } while (0)
#define                 CONFIG_M1()         DDRD |= _BV(M1_STEP_BIT) |         \
                                                    _BV(M1_DIR_BIT);           \
                                            DDRB |= _BV(M1_EN_BIT)

#define                 ENABLE_M2()         PORTB &= ~_BV(M2_EN_BIT)
#define                 DISABLE_M2()        PORTB |= _BV(M2_EN_BIT)
#define                 STEP_M2()           PORTD &= ~_BV(M2_STEP_BIT);        \
                                            delayMicroseconds(DELTAT);         \
                                            PORTD |= _BV(M2_STEP_BIT);         \
                                            delayMicroseconds(DELTAT)
#define                 SET_DIR_M2(dir)     do                                 \
                                            {                                  \
                                                if (dir == HIGH)               \
                                                    PORTD |= _BV(M2_DIR_BIT);  \
                                                else                           \
                                                    PORTD &= ~_BV(M2_DIR_BIT); \
                                            } while (0)
#define                 CONFIG_M2()         DDRD |= _BV(M2_STEP_BIT) |         \
                                                    _BV(M2_DIR_BIT);           \
                                            DDRB |= _BV(M2_EN_BIT)

#define                 CONFIG_M()          CONFIG_M1(); CONFIG_M2()
                                          

#define                 ENABLE_M()          ENABLE_M1(); ENABLE_M2()
#define                 DISABLE_M()         DISABLE_M1(); DISABLE_M2()

#define                 CONFIG_HALL_M1()    DDRC &= ~_BV(HALL_M1_BIT)
#define                 CONFIG_HALL_M2()    DDRC &= ~_BV(HALL_M2_BIT)

#define                 CONFIG_HALL()       CONFIG_HALL_M1(); CONFIG_HALL_M2()
#define                 READ_HALL(joint)    (bitRead(PINC, (joint == JOINT1) ? \
                                             HALL_M1_BIT : HALL_M2_BIT) == 0)

#define                 CONFIG_RF()         DDRC |= _BV(RF_BIT)

// Constants definitions
const uint32_t          DELTAT              = 100;
const uint8_t           MICROSTEPS_TO_DEG   = 16;
const size_t            SAMPLES             = 10;
const motor_direction   DEFAULT_DIRECTION   = CW;
const uint32_t          MOTOR_SLEEP_TIMEOUT = 10000000;
uint32_t                LAST_ACTIVE_M1      = micros();
uint32_t                LAST_ACTIVE_M2      = micros();
const uint16_t          BUF_SIZE            = 32;

// Error reporting function
static inline void throw_error(const char *message)
{
    char error[128];
    snprintf(error, sizeof(error), "Error: %s.", message);
    Serial.println(error);
}

// Function prototypes
bool home_motor_to_origin(const joint_id joint);
void capture_sensor_data(uint16_t *sensor_values, size_t samples);
void rotate_motor_to_next_sample(const uint32_t wanted_ang_m1, const uint32_t wanted_ang_m2);
void inline rotate_motor_step(const joint_id joint, const motor_direction direction);
void transmit_sensor_data(uint16_t *sensor_values, size_t samples);
void inline sleep_motor();

#endif // UTILS_H
