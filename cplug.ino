#include <WiFi.h>              // Library for Wi-Fi functionality
#include <WebServer.h>         // Library to create a web server
#include <Preferences.h>       // Library to manage non-volatile storage (NVS)
#include <esp_wifi.h>          // Library for ESP32 Wi-Fi functions (for MAC address setting)
#include <LiquidCrystal_I2C.h> // Library for I2C LCD
#include <DNSServer.h>         // Library for DNS server to implement captive portal

Preferences preferences;       // Create a Preferences object for storing Wi-Fi credentials
WebServer server(80);          // Create a web server on port 80
DNSServer dnsServer;           // Create a DNS server for captive portal
const byte DNS_PORT = 53;      // Standard DNS port
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD I2C address 0x27 with 16 columns and 2 rows

const int relayPin = 15;       // GPIO pin connected to the relay
const int buttonPin = 4;       // GPIO pin connected to the self-lock push button
bool relayState = LOW;         // Initial state of the relay (LOW = off)
bool buttonPressed = false;    // State to track if the button is pressed
uint8_t previousClientCount = 0; // To track changes in connected clients

uint8_t newMACAddress[] = {0x02, 0x65, 0x32, 0xac, 0x81, 0x4b};
String newHostname = "cplug1";
// Default credentials for the access point
const char* apSSID = "CPLUG1";       // Access point SSID
const char* apPassword = "**CPLUG1**";       // Access point password

// Variables to store Wi-Fi credentials (SSID and password)
String ssid = "";
String password = "";

// Variable to track the current display state
String previousDisplayState = "";

void setup() {
  Serial.begin(115200);                // Initialize serial communication at 115200 baud rate
  lcd.init();                          // Initialize the LCD
  lcd.backlight();                     // Turn on the LCD backlight

  WiFi.mode(WIFI_STA);
  Serial.print("[OLD] ESP32 MAC Address:  ");
  Serial.println(WiFi.macAddress());
  esp_wifi_set_mac(WIFI_IF_STA, newMACAddress); // Set MAC address for the station interface
  Serial.print("[NEW] ESP32 MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // Set new hostname
  WiFi.setHostname(newHostname.c_str());

  pinMode(relayPin, OUTPUT);           // Set relayPin as an output
  pinMode(buttonPin, INPUT_PULLUP);    // Configure the button pin with an internal pull-up resistor
  digitalWrite(relayPin, relayState);  // Set the relay to its initial state (off)

  // Load stored Wi-Fi credentials from non-volatile storage
  preferences.begin("wifi", false);             // Open NVS with "wifi" namespace
  ssid = preferences.getString("ssid", "");     // Retrieve stored SSID, or empty if not set
  password = preferences.getString("password", "");  // Retrieve stored password, or empty if not set

  // Try connecting to Wi-Fi or start AP if it fails
  if (ssid != "" && password != "") {
    if (!connectToWiFi()) {
      startAccessPoint();
    }
  } else {
    startAccessPoint();
  }

  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Define routes for the web server
  server.on("/", handleRoot);              // Root route, main page
  server.on("/toggle", handleToggle);      // Route to toggle the relay
  server.on("/config", handleConfig);      // Route for Wi-Fi configuration page
  server.on("/save", HTTP_POST, handleSave); // Route to save Wi-Fi settings
  server.onNotFound(handleCaptivePortal);  // Redirect all unknown requests to config page

  server.begin();   // Start the web server
}

void loop() {
  // Process DNS requests for captive portal
  dnsServer.processNextRequest();

  // Handle the web server
  server.handleClient();

  // Check the button state
  if (digitalRead(buttonPin) == LOW) {
    // If the button is pressed, keep the relay ON
    buttonPressed = true;
    digitalWrite(relayPin, HIGH); // Ensure the relay is ON
  } else if (buttonPressed) {
    // If the button was previously pressed and is now released
    buttonPressed = false;
    relayState = LOW; // Reset the relay state
    digitalWrite(relayPin, relayState); // Turn off the relay
  }

  // Update the LCD display based on the current state
  updateLCD();
}

// Function to connect to Wi-Fi using stored SSID and password
bool connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid.c_str(), password.c_str());  // Start Wi-Fi connection

  unsigned long startAttemptTime = millis();   // Record the start time of the connection attempt

  // Loop until connected or until 10 seconds have passed
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);             // Wait half a second between checks
    Serial.print(".");      // Print dots to indicate progress
  }

  // Check if connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi!");    // Print success message
    Serial.print("IP Address: ");               // Show assigned IP address
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\nFailed to connect to Wi-Fi."); // Print failure message
    return false;
  }
}

// Function to start an access point for Wi-Fi configuration
void startAccessPoint() {
  Serial.println("Starting Access Point...");
  WiFi.softAP(apSSID, apPassword);       // Start the access point with SSID and password
  Serial.print("Access Point IP: ");     // Print the IP address for clients
  Serial.println(WiFi.softAPIP());
}

// Function to update the LCD display based on the current state
void updateLCD() {
  uint8_t clientCount = WiFi.softAPgetStationNum(); // Get the number of connected clients
  String line1, line2;

  if (WiFi.status() == WL_CONNECTED || clientCount > 0) {
    // If connected to Wi-Fi or in AP mode with a client, show the management IP address
    line1 = "MANAGEMENT:";
    line2 = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  } else {
    // If in AP mode and no clients are connected, show the SSID and password
    line1 = "WiFi:   " + String(apSSID);
    line2 = "Pass: " + String(apPassword);
  }

  // Avoid updating the screen if the content is the same
  String newDisplayState = line1 + line2;
  if (newDisplayState != previousDisplayState) {
    previousDisplayState = newDisplayState;

    // Update the LCD display
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1.substring(0, 16)); // Ensure only 16 characters per line
    lcd.setCursor(0, 1);
    lcd.print(line2.substring(0, 16)); // Ensure only 16 characters per line
  }
}

// Function to toggle the relay state via API
void handleToggle() {
  if (!buttonPressed) { // Allow toggling only if the button is not pressed
    relayState = !relayState;              // Invert the current state
    digitalWrite(relayPin, relayState);    // Set the relay to the new state
  }
  server.send(200, "text/plain", relayState ? "1" : "0"); // Send the new state as plain text ("1" for on, "0" for off)
}

// Function to handle the root web page, which displays relay state and control options
void handleRoot() {
  // HTML content with inline CSS and JavaScript for the main control page
  String html = "<!DOCTYPE html><html><head>"
                "<title>Relay Control</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; background-color: #f3f3f9; color: #333; padding: 20px; }"
                "h1 { color: #444; text-transform: uppercase; }"
                "#relayState { font-size: 1.2em; color: #007bff; font-weight: bold; }"
                ".button { padding: 15px 30px; font-size: 1em; color: white; background-color: #007bff; border: none; border-radius: 5px; cursor: pointer; transition: background 0.3s; text-decoration: none; display: inline-block; margin: 10px 0; }"
                ".button:hover { background-color: #0056b3; }"
                "</style>"
                "</head><body>"
                "<h1>" + newHostname + "</h1>"
                "<p><span id='relayState'>" + String(relayState ? "On" : "Off") + "</span></p>"

                // Toggle relay button
                "<button class='button' onclick='toggleRelay()'>Toggle</button><br>"

                // Link to the configuration page
                "<a href='/config' class='button'>Configure Wi-Fi</a>"

                "<script>"
                "function toggleRelay() {"
                "  var xhr = new XMLHttpRequest();"
                "  xhr.open('GET', '/toggle', true);"
                "  xhr.onload = function() {"
                "    if (xhr.status === 200) {"
                "      document.getElementById('relayState').innerText = xhr.responseText === '1' ? 'On' : 'Off';"
                "    }"
                "  };"
                "  xhr.send();"
                "}"
                "</script>"
                "</body></html>";
  server.send(200, "text/html", html);   // Send the HTML page to the client
}

// Function to display Wi-Fi configuration page with SSID and password fields
void handleConfig() {
  // HTML form for Wi-Fi configuration page with password visibility toggle and a link to return to the main page
  String html = "<!DOCTYPE html><html><head>"
                "<title><title>WiFi Configuration</title></title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; background-color: #f3f3f9; color: #333; padding: 20px; }"
                "h1 { color: #007bff; }"
                "form { display: inline-block; margin-top: 20px; text-align: left; }"
                "label { font-weight: bold; display: block; margin-bottom: 5px; }"
                "input[type='text'], input[type='password'] { width: 100%; padding: 10px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 5px; }"
                "input[type='submit'] { padding: 10px 20px; font-size: 1em; color: white; background-color: #007bff; border: none; border-radius: 5px; cursor: pointer; transition: background 0.3s; }"
                "input[type='submit']:hover { background-color: #0056b3; }"
                "</style>"
                "</head><body>"
                "<h1>WiFi Configuration</h1>"
                "<form action=\"/save\" method=\"POST\">"   // Form to enter SSID and password
                "<label for=\"ssid\">SSID:</label>"
                "<input type=\"text\" name=\"ssid\" id=\"ssid\" required><br>"
                "<label for=\"password\">Password:</label>"
                "<input type=\"password\" name=\"password\" id=\"password\" required>"
                "<input type=\"checkbox\" onclick=\"togglePassword()\"> Show password<br><br>" // Checkbox to toggle password visibility
                "<input type=\"submit\" value=\"Save\">"    // Submit button
                "</form>"
                "<script>"
                "function togglePassword() {"              // JavaScript function to toggle password visibility
                "  var passwordField = document.getElementById('password');"
                "  passwordField.type = passwordField.type === 'password' ? 'text' : 'password';"
                "}"
                "</script>"
                "</body></html>";
  server.send(200, "text/html", html);   // Send HTML form to client
}

// Function to handle form submission and save Wi-Fi credentials
void handleSave() {
  ssid = server.arg("ssid");       // Get SSID from form input
  password = server.arg("password"); // Get password from form input

  // Store SSID and password in non-volatile storage
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);

  // Debugging lines to verify the stored SSID and password
  Serial.print("Stored SSID: ");
  Serial.println(preferences.getString("ssid", ""));
  Serial.print("Stored Password: ");
  Serial.println(preferences.getString("password", ""));

  // Confirmation page HTML
  String html = "<!DOCTYPE html><html><head><title>Configuration Saved</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; background-color: #f3f3f9; color: #333; padding: 20px; }"
                "h1 { color: #007bff; }"
                "p { font-size: 1.1em; }"
                "</style>"
                "</head><body><h1>Configuration Saved</h1>"
                "<p>The device will now attempt to connect to the WiFi network.</p>"
                "</body></html>";
  server.send(200, "text/html", html); // Send confirmation page to client

  delay(2000);   // Wait for 2 seconds before restarting
  ESP.restart(); // Restart ESP32 to attempt Wi-Fi connection with new credentials
}
// Function to handle captive portal and redirect all requests to configuration page
void handleCaptivePortal() {
  // Diagnostic print statements
  Serial.println("Captive Portal Triggered!");
  Serial.print("Client IP: ");
  Serial.println(server.client().remoteIP());
  Serial.print("Requested URI: ");
  Serial.println(server.uri());
  
  // Redirect all requests to the configuration page
  server.sendHeader("Location", "/config", true);
  server.send(302, "text/plain", "Redirecting to configuration page");
  
  // Additional diagnostic information
  Serial.println("Redirect sent to configuration page");
}
