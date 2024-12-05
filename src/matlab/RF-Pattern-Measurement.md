# RF Radiation Pattern Measurement

This project measures RF radiation patterns using an Arduino and MATLAB integration. The system adjusts a receiving antenna's azimuth and elevation angles to capture signal strength at various points, generating a 3D radiation pattern.

## Features
- **Automated Measurement**: Controls a stepper motor for precise antenna positioning.
- **Data Integration**: Collects RF signal data via Arduino and processes it in MATLAB.
- **3D Visualization**: Generates 3D radiation patterns from collected data.

## Hardware Requirements
- **Stepper Motor**: NEMA 17 (200 steps/rev) with DRV8825 driver.
- **Microcontroller**: Arduino Mega.
- **RF Power Detector**: AD8313 module.
- **MATLAB**: For data acquisition and visualization.

## File Structure
- `code/`: Contains:
  - `matlab_code.m`: MATLAB script for data acquisition and visualization.
  - `arduino_code.ino`: Arduino sketch for controlling motors and reading RF data.
- `docs/`: Includes diagrams and user guides for the system.

## Getting Started
1. **Hardware Setup**:
   - Assemble the hardware as per `docs/measurement_diagram.png`.
   - Ensure the antenna, stepper motor, and RF detector are correctly connected.

2. **Software Setup**:
   - Load `arduino_code.ino` to the Arduino Mega.
   - Open `matlab_code.m` in MATLAB and configure the COM port for Arduino communication.

3. **Run the System**:
   - Execute the MATLAB script to control the stepper motor and capture RF data.
   - The collected data will be plotted as a 3D radiation pattern.

## How to Use
- Adjust the resolution (angle steps) in the MATLAB script to balance precision and measurement time.
- Modify motor settings in the Arduino code for different setups.

## Example Output
See below for a sample visualization of an antenna's radiation pattern:
![Sample Radiation Pattern](docs/sample_radiation_pattern.png)

## License
This project is licensed under the [MIT License](../LICENSE).

## Contributions
Suggestions and improvements are welcome! Please open an issue or submit a pull request.
