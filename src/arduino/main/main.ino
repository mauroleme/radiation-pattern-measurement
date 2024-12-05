/*
 * File     : main.ino
 * Author   : Mauro Leme
 * Date     : December 12, 2024
 * Purpose  : Retrieves sensor data and sends it via serial port.
 *
 * License  : MIT License
 *
 *            Copyright (c) 2024 mauroleme
 *
 *            Permission is hereby granted, free of charge, to any person obtaining a copy
 *            of this software and associated documentation files (the "Software"), to deal
 *            in the Software without restriction, including without limitation the rights
 *            to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *            copies of the Software, and to permit persons to whom the Software is
 *            furnished to do so, subject to the following conditions:
 *
 *            The above copyright notice and this permission notice shall be included in all
 *            copies or substantial portions of the Software.
 *
 *            THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *            IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *            FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *            AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *            LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *            OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *            SOFTWARE.
 */

#include <Arduino.h>

// PIN definitions
const uint8_t M1_DIR_PIN        = 7;    // Motor direction
const uint8_t M1_STEP_PIN       = 6;    // Motor step
const uint8_t M1_EN_PIN         = 8;    // Motor enable
const uint8_t HALL_PIN          = A0;   // Hall sensor
const uint8_t RF_PIN            = A15;  // Radiofrequency module

// Constants definitions
const size_t samples            = 10;   // Number of samples that are read for
                                        // each request
const enum directions { RIGHT = LOW, LEFT = HIGH } direction;

void setup()
{
    // Setting up the pins
    pinMode(M1_DIR_PIN, OUTPUT);
    pinMode(M1_STEP_PIN, OUTPUT);
    pinMode(M1_EN_PIN, OUTPUT);
    pinMode(HALL_PIN, INPUT);
    pinMode(RF_PIN, INPUT);
    
    // Activate the motor and set initial direction
    digitalWrite(M1_EN_PIN, LOW);
    digitalWrite(M1_DIR_PIN, LOW);

    // Setting up the serial port
    Serial.setTimeout(1000);
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("Serial port initialized successfully!");

    // Set motor to the origin
    set_motor_to_origin();
}

void loop()
{
    /* State machine to handle different modes based on serial input
     *      - mode 0: Waiting for serial input. Once the input is received, it
     *                switches to the next mode.
     *      - mode 1: Rotates the motors and samples the sensor values. After
     *                completing, it sends the values via serial port and 
     *                resets to mode 0.
     */
    static int mode = 0;
    switch (mode)
    {
        case 0:
            if (Serial.available() > 0)
            {
                mode = Serial.parseInt();
            }
            break;
        case 1:
            uint16_t sensor_values[samples] = {0};
            motor_sampling_handler();
            read_sensor(sensor_values, samples);
            serial_write(sensor_values, samples);
            
            mode = 0;
            break;
        default:
            /* The buffer contains a warning string with the command received:
             *      - 46 chars for the literal string;
             *      - 11 chars to represent the number;
             *      - 1 char for null terminator.
             */
            char error[58];
            sprintf(error, "Unknown command: %d. Setting back to listen mode",
                            mode);
            Serial.println(error);
            
            mode = 0;
            break;
    }
}

void set_motor_to_origin()
{
    Serial.println("Starting search for motor origin...");

    /*
     * 1º passo - Ler o valor do sensor
     *      1.1º passo - Caso o sensor detete um valor abaixo do limiar, girar
     *                   para o sentido contrário até o valor do sensor estar
     *                   acima do limiar. Caso contrário, se o valor de partida
     *                   estiver acima do limiar, não se faz esta inicialização
     * 2º passo - Quando o valor do sensor for abaixo do limiar, guarda o
     *            passo para o qual houve esta deteção e continua a girar para
     *            a mesma direção
     * 3º passo - Quando o valor do sensor for acima do liminar, guarda o
     *            passo para o qual houve esta detecção
     */
   
    static const direction default_direction    = RIGHT;
    uint16_t               threshold            = 50;
    uint16_t               steps_completed      = 0;
    int16_t                start_step           = -1;
    int16_t                end_step             = -1;
    const uint16_t         MAX_STEPS            = 3200; // 16 * 200

    while (analogRead(HALL_PIN) < threshold) { motor_step(!default_direction); }

    for (; steps_completed < MAX_STEPS; steps_completed++)
    {
        uint16_t sensor_value = analogRead(HALL_PIN);

        if (sensor_value < threshold && start_step == -1)
        {
            start_step = steps_completed;
        }
        else if (sensor_value >= threshold && start_step != -1)
        {
            end_step = steps_completed;
            break;
        }
        motor_step(default_direction);
    }

    uint16_t central_step = (start_step + end_step) / 2;
    for (; central_step >= 0; central_step--)
    {
        motor_step(!default_direction);
    }
}

void motor_sampling_handler()
{
    static const uint8_t MICROSTEPS_TO_DEG  = 16;   // 1 degree == 16 microsteps
    static const uint8_t MAX_ANGLE          = 180;
    static int16_t       current_angle      = 0;    // define it as 0 since we suppose
                                                    // that the motor has been homed
    static direction    dir                 = RIGHT;
   
    // Set the motor back to home
    int16_t previous_angle = current_angle;
    if (current_angle == MAX_ANGLE || current_angle == -MAX_ANGLE)
    {
        dir = !dir;
        for (size_t i = 0; i < MICROSTEPS_TO_DEG * current_angle; i++)
        {
            motor_step(dir);
            current_angle -= i % (MICRO_STEPS_TO_DEG - 1);
        }

    }
   
    // If the motor was in the MINIMUM ANGLE, restart the sampling at 0 degrees
    // i.e. jump the iteration of a degree
    // In the future, two motors will be used, so when it reaches the end of
    // the minimum degree value, the sencond motor iterates to the next angle
    if (previous_angle == -MAX_ANGLE)
    {
        return;
    }

    // Move the motor 1 degree (16 microsteps)
    for (size_t i = 0; i < MICROSTEPS_TO_DEG; i++)
    {
        motor_step(dir);
    }
    current_angle++;
}

void motor_step(const direction dir)
{
    digitalWrite(M1_DIR_PIN, dir);

    digitalWrite(M1_STEP_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(M1_STEP_PIN, LOW);
    delayMicroseconds(1000);
}

void read_sensor(uint16_t *sensor_values, size_t samples)
{
    for (size_t i = 0; i < samples; i++) 
    {
        sensor_values[i] = analogRead(RF_PIN);
        delay(10);
    }
}
void serial_write(uint16_t *sensor_values, size_t samples)
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
