Here’s an updated `README.md` with your requested additions:

---

# ESP32 Relay Control with LCD and Button

This project allows you to control a relay using an ESP32 microcontroller. It provides a simple and intuitive web interface for toggling the relay state, displays relevant information on an I2C LCD, and includes a physical button for direct relay control. 

## Features

1. **Wi-Fi Modes**:
   - Connects to an existing Wi-Fi network using stored credentials.
   - Creates an Access Point (AP) if no network credentials are available.

2. **Dynamic LCD Display**:
   - Displays the AP SSID and password when in AP mode without clients.
   - Shows the URL for relay control when a client connects to the AP or when connected to Wi-Fi.

3. **Physical Button Control**:
   - Allows direct control of the relay.
   - Button press takes priority over API or web-based control.

4. **Web Interface**:
   - Responsive relay control using modern AJAX (XHR).
   - Displays the current relay state without page refreshes.

5. **Non-Volatile Storage**:
   - Saves Wi-Fi credentials using the ESP32's Preferences library.

6. **Custom 3D-Printed Case**:
   - A 3D-printable STL file for the case is included in the `assets` folder.

## Requirements

### Software
- Arduino IDE or Arduino CLI
- ESP32 Board Support Package
- Required Libraries:
  - `WiFi.h`
  - `WebServer.h`
  - `Preferences.h`
  - `esp_wifi.h`
  - `LiquidCrystal_I2C.h`

### Hardware
- **ESP32 Microcontroller**
- **I2C LCD Display**: 16x2 with an I2C interface.
- **Relay Module**
- **Self-Lock Push Button**
- **Voltage Converter**: Converts 110V-240V AC to 5V DC for the ESP32.
- **NEMA 5-15R Socket**: A standard US receptacle for the relay-controlled device.
- **IEC 320 C14 Socket (AS-05, AC 250 V, 10 A)**: Common power inlet for devices.

**Note**: See the [Hardware](#hardware) section below for links to the components used.

## Installation

1. Clone this repository or copy the source code to your local machine.
2. Open the project in Arduino IDE or place it in a suitable directory for the Arduino CLI.
3. Install the required libraries:
   - Use the Arduino Library Manager or Arduino CLI.
   - Ensure the ESP32 board package is installed.
4. Connect your ESP32 and upload the code.

## Usage

1. **Initial Configuration**:
   - When powered on for the first time, the ESP32 will start in AP mode. The LCD will display the SSID and password for the AP.
   - Connect to the AP and navigate to the displayed URL to configure Wi-Fi credentials.

2. **Relay Control**:
   - Access the web interface at the URL displayed on the LCD to toggle the relay state.
   - Press the physical button to control the relay directly. The button press overrides web control.

3. **LCD Display**:
   - Shows the current mode and necessary connection details:
     - AP mode without clients: SSID and password.
     - AP mode with clients: URL for relay control.
     - Wi-Fi connected: URL for relay control.

## Hardware

| Component                     | Description                                     | Link                     |
|-------------------------------|-------------------------------------------------|--------------------------|
| ESP32 Microcontroller         | The heart of the project.                      | [Add Link]               |
| I2C LCD Display (16x2)        | Displays connection details and status.        | [Add Link]               |
| Relay Module                  | Controls the electrical load.                  | [Add Link]               |
| Self-Lock Push Button         | Physical control of the relay.                 | [Add Link]               |
| Voltage Converter (110V-240V to 5V) | Powers the ESP32 from AC mains.               | [Add Link]               |
| NEMA 5-15R Socket             | Standard US receptacle for connected devices.  | [Add Link]               |
| IEC 320 C14 Socket (AS-05)    | Power inlet for AC connections.                | [Link](https://www.amazon.com/Baomain-Panel-Power-Socket-Connector/dp/B00WFYS1HS/ref=sr_1_1?crid=2AH4T4U21RJY0&dib=eyJ2IjoiMSJ9.o2AdHKFsyHimIjMr6q8nZA8UOt_GoRbGCrPzT085l6LmGa75eYUp2Iu93e91JlG-ZjGe-yor4HFuf1mvtRVtcm5KmduDByGhFHxt-kzASViMKf0OOzjHaLeEWpvepVyfPifnHwS5HujjkfdvMaR1ys0JTfGZoBOZZayyiODlNlgXQRUbeBY8whl7jbCTqxhqqfOeOG1r7tnm2FsTqq1ykwNWGQkgUMIFuup9lu8L_98.qGuYcbvfJGRp2hODLt11jA4VYX2CxGktAQs5OteeDsE&dib_tag=se&keywords=IEC%2B320%2BC14%2BAS-05%2BAC%2B250%2BV%2B10%2BA&qid=1731795545&sprefix=iec%2B320%2Bc14%2Bas-05%2Bac%2B250%2Bv%2B10%2Ba%2Caps%2C54&sr=8-1&th=1)             |



## Custom Case

- A 3D-printable case designed for this project is available in the `assets` folder.
- The STL file can be printed with standard PLA or ABS material.

## Example Web Interface

- **Relay State**: Displays whether the relay is `ON` or `OFF`.
- **Toggle Button**: Instantly toggles the relay state.
- Uses modern AJAX (XHR) for a seamless user experience.

## Known Issues and Future Improvements

1. **Button Behavior**:
   - Relay state may not update on the web interface while the button is pressed. This behavior is by design to prioritize physical control.

2. **Suggestions**:
   - Add password protection to the web interface for enhanced security.
   - Include OTA (Over-the-Air) updates for easier firmware management.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

Replace the placeholders in the `Hardware` section with the appropriate links to your components. Let me know if you'd like further adjustments!