# Stepper Motor Control

This project demonstrates how to precisely control a stepper motor using an Arduino and the DRV8825 stepper motor driver. The system is designed for high precision applications, where microstepping (1/16) is used to achieve smoother motion.

## Features
- **Precise Control**: Supports microstepping for smooth and accurate movements.
- **Direction Control**: Easily change the motor's rotation direction.
- **Speed Adjustment**: Configure step delays to control the motor's speed.

## Hardware Requirements
- **Stepper Motor**: NEMA 17 (200 steps/rev).
- **Motor Driver**: DRV8825 (configured for 1/16 microstepping).
- **Microcontroller**: Arduino Mega.
- **Power Supply**: 12V or 24V DC, depending on motor specifications.

## File Structure
- `code/`: Contains the Arduino sketch (`main_code.ino`).
- `docs/`: Includes wiring diagrams and additional documentation.

## Getting Started
1. **Hardware Setup**:
   - Connect the motor driver and stepper motor as shown in `docs/wiring_diagram.png`.
   - Ensure the DRV8825 is configured for 1/16 microstepping using the correct jumper settings.

2. **Load the Code**:
   - Open `main_code.ino` in the Arduino IDE.
   - Upload the sketch to your Arduino Mega.

3. **Test the Motor**:
   - Power the motor driver and Arduino.
   - The motor should perform a series of precise rotations as programmed.

## How to Use
- Modify the delay in the loop section to adjust the motor's speed.
- Change the `direction` pin state to reverse the motor's direction.

## License
This project is licensed under the [MIT License](../LICENSE).

## Contributions
Feel free to contribute by opening issues or submitting pull requests. Feedback is always welcome!
