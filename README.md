# Radiation Pattern Measurement
This repository includes the source files for a project designed to measure 
the radiation pattern of various antenas. The project involves controlling two
stepper motos for precise antenna positioning and using an RF radiation pattern
measurement sensor integrated in the Arduino-based system. The collected data
is transmited to Matlab, where it is used to generate a model based of the
antenna's radiation pattern and compare it to the antenna's ideal theoretical
model.

## Directory Structure
The project is organized into two main folders:
- **Arduino**               : Contains the source code for the microcontroller,
responsible for motor control, data acquisition and transmission;
- **MATLAB**                : Contains the source code for processing data from
the microcontroller and building the antenna's radiation pattern model.

## Hardware Components
This project includes following hardware components:
- **Microcontroller**       : Arduino Mega 2560 Rev3;
- **Stepper Motor Driver**  : DRV8825 driver module;
- **RF Sensor**             : RF radiation pattern measurement sensor.

## Contributions
Contributions to improve this project are welcome! Feel free to open issues or
submite pull requests.
