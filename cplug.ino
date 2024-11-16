#include <WiFi.h>              // Library for Wi-Fi functionality
#include <WebServer.h>         // Library to create a web server
#include <Preferences.h>       // Library to manage non-volatile storage (NVS)
#include <esp_wifi.h>          // Library for ESP32 Wi-Fi functions (for MAC address setting)
#include <LiquidCrystal_I2C.h> // Library for I2C LCD

Preferences preferences;       // Create a Preferences object for storing Wi-Fi credentials
WebServer server(80);          // Create a web server on port 80
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD I2C address 0x27 with 16 columns and 2 rows

const int relayPin = 15;       // GPIO pin connected to the relay
const int buttonPin = 4;       // GPIO pin connected to the self-lock push button
bool relayState = LOW;         // Initial state of the relay (LOW = off)
uint8_t previousClientCount = 0; // To track changes in connected clients
bool buttonHoldOverride = false; // To track button hold priority

uint8_t newMACAddress[] = {0x02, 0x65, 0x32, 0xac, 0x81, 0x4b};
String newHostname = "cplug1";
// Default credentials for the access point
const char* apSSID = "cplug1";       // Access point SSID
const char* apPassword = "**cplug1**";       // Access point password

// Variables to store Wi-Fi credentials (SSID and password)
String ssid = "";
String password = "";

// Function to display a temporary message on LCD and return to base screen after 5 seconds
void displayTempMessage(String line1, String line2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  if (line2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
  delay(5000); // Display message for 5 seconds
  updateBaseScreen(); // Return to base screen
}

// Function to update the base screen based on connection status
void updateBaseScreen() {
  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    // Connected to WiFi, show IP for connection
    lcd.setCursor(0, 0);
    lcd.print("Connect to:");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
  } else {
    // Access Point mode, show SSID and password
    lcd.setCursor(0, 0);
    lcd.print("SSID: " + String(apSSID));
    lcd.setCursor(0, 1);
    lcd.print("Pass: " + String(apPassword));
  }
}

void setup() {
  Serial.begin(115200);                // Initialize serial communication at 115200 baud rate
  lcd.init();                          // Initialize the LCD
  lcd.backlight();                     // Turn on the LCD backlight
  updateBaseScreen();                  // Set initial base screen

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

  // If Wi-Fi credentials are available, try connecting to Wi-Fi
  if (ssid != "" && password != "") {
    connectToWiFi();
  }

  // If the connection fails, start an access point for configuration
  if (WiFi.status() != WL_CONNECTED) {
    startAccessPoint();
  }

  // Define routes for the web server
  server.on("/", handleRoot);              // Root route, main page
  server.on("/toggle", handleToggle);      // Route to toggle the relay
  server.on("/config", handleConfig);      // Route for Wi-Fi configuration page
  server.on("/save", HTTP_POST, handleSave); // Route to save Wi-Fi settings

  server.begin();   // Start the web server
}

void loop() {
  static String lastDisplayedText = "";
  static bool buttonPreviousState = HIGH;

  server.handleClient();

  bool buttonCurrentState = digitalRead(buttonPin);

  if (buttonCurrentState == LOW) {
    if (buttonPreviousState == HIGH) {
      relayState = HIGH;
      digitalWrite(relayPin, relayState);
      buttonHoldOverride = true; // Activer la priorité du bouton
      Serial.println("Relay ON (Button Pressed)");

      String currentText = "Button Control:\nRelay ON (Held)";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Button Control:");
      lcd.setCursor(0, 1);
      lcd.print("Relay ON (Held)");
      lastDisplayedText = currentText;
    }
  } 
  else {
    if (buttonPreviousState == LOW) {
      relayState = LOW;
      digitalWrite(relayPin, relayState);
      buttonHoldOverride = false; // Désactiver la priorité du bouton
      Serial.println("Relay OFF (Button Released)");
      
      updateBaseScreen();
      lastDisplayedText = "";
    }
  }

  buttonPreviousState = buttonCurrentState;

  // Update the LCD display based on the AP client connection status
  uint8_t clientCount = WiFi.softAPgetStationNum(); // Get the number of connected clients
  if (clientCount != previousClientCount) {
    previousClientCount = clientCount; // Update the previous client count

    if (clientCount > 0) {
      // If a client is connected, show the AP IP address for configuration
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Config IP:");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.softAPIP());
    } else if (WiFi.status() != WL_CONNECTED) {
      // If no client is connected and Wi-Fi is not connected, show SSID and password
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(String("SSID:") + apSSID);
      lcd.setCursor(0, 1);
      lcd.print(String("Pass:") + apPassword);
      delay(2000); // Display SSID briefly
    }
  }
}

// Function to connect to Wi-Fi using stored SSID and password
void connectToWiFi() {
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

    // Update base screen with connection info
    updateBaseScreen();
  } else {
    Serial.println("\nFailed to connect to Wi-Fi."); // Print failure message
    displayTempMessage("WiFi Connect", "Failed");
  }
}

// Function to start an access point for Wi-Fi configuration
void startAccessPoint() {
  Serial.println("Starting Access Point...");
  WiFi.softAP(apSSID, apPassword);       // Start the access point with SSID and password
  Serial.print("Access Point IP: ");     // Print the IP address for clients
  Serial.println(WiFi.softAPIP());

  // Update base screen with AP info
  updateBaseScreen();
}

// Function to handle the root web page, which displays relay state and control options
void handleRoot() {
  // HTML content with inline CSS and JavaScript for the main control page
  String html = "<!DOCTYPE html><html><head>"
                "<title>Relay Control</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; background-color: #f3f3f9; color: #333; padding: 20px; }"
                "h1 { color: #444; }"
                "#relayState { font-size: 1.2em; color: #007bff; font-weight: bold; }"
                ".button { padding: 15px 30px; font-size: 1em; color: white; background-color: #007bff; border: none; border-radius: 5px; cursor: pointer; transition: background 0.3s; text-decoration: none; display: inline-block; margin: 10px 0; }"
                ".button:hover { background-color: #0056b3; }"
                "</style>"
                "</head><body>"
                "<h1>Relay Control</h1>"
                "<p>Relay state: <span id='relayState'>" + String(relayState ? "On" : "Off") + "</span></p>"

                // Link to toggle relay state
                "<a href='#' class='button' onclick=\"toggleRelay()\">Toggle Relay</a><br>"

                // Link to Wi-Fi configuration page
                "<a href=\"/config\" class='button'>Configure Wi-Fi</a>"

                "<script>"
                "function toggleRelay() {"
                "  let stateElement = document.getElementById('relayState');"
                "  let currentState = stateElement.innerText;"
                "  "
                "  // Optimistic UI update"
                "  stateElement.innerText = (currentState === 'On') ? 'Off' : 'On';"
                "  "
                "  let xhr = new XMLHttpRequest();"
                "  xhr.open('GET', '/toggle', true);"
                "  xhr.timeout = 2000; // 2-second timeout"
                "  "
                "  xhr.onreadystatechange = function() {"
                "    if (xhr.readyState === 4) {"
                "      if (xhr.status === 200) {"
                "        console.log('Toggle response:', xhr.responseText);"
                "        stateElement.innerText = xhr.responseText === '1' ? 'On' : 'Off';"
                "      } else {"
                "        console.error('Toggle failed, reverting state');"
                "        stateElement.innerText = currentState;"
                "      }"
                "    }"
                "  };"
                "  "
                "  xhr.onerror = function() {"
                "    console.error('Network error');"
                "    stateElement.innerText = currentState;"
                "  };"
                "  "
                "  xhr.send();"
                "}"
                "function holdRelay() { fetch('/hold'); }"       // Send request to hold relay
                "function releaseRelay() { fetch('/toggle'); }"  // Send request to release relay
                "function fiveSecRelay() {"                      // Activate relay for 5 seconds
                "  fetch('/5sec')"
                "    .then(response => response.text())"
                "    .then(state => {"
                "      document.getElementById('relayState').innerText = state === '1' ? 'On' : 'Off';"
                "    });"
                "}"
                "</script>"
                "</body></html>";
  server.send(200, "text/html", html);   // Send the HTML page to the client
}

// Function to toggle the relay state
void handleToggle() {
  // Si le bouton est actuellement maintenu enfoncé, ignorer la demande de toggle
  if (buttonHoldOverride) {
    server.send(200, "text/plain", "1"); // Renvoyer que le relais reste ON
    return;
  }

  relayState = !relayState;              
  digitalWrite(relayPin, relayState);    
  
  displayTempMessage("Relay:", relayState ? "Turned ON" : "Turned OFF");
  
  server.send(200, "text/plain", relayState ? "1" : "0");
}



// Function to display Wi-Fi configuration page with SSID and password fields
void handleConfig() {
  // HTML form for Wi-Fi configuration page with password visibility toggle and a link to return to the main page
  String html = "<!DOCTYPE html><html><head>"
                "<title>Wi-Fi Configuration</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; background-color: #f3f3f9; color: #333; padding: 20px; }"
                "h1 { color: #444; }"
                "form { display: inline-block; margin-top: 20px; text-align: left; }"
                "label { font-weight: bold; display: block; margin-bottom: 5px; }"
                "input[type='text'], input[type='password'] { width: 100%; padding: 10px; margin-bottom: 15px; border: 1px solid #ccc; border-radius: 5px; }"
                "input[type='submit'] { padding: 10px 20px; font-size: 1em; color: white; background-color: #007bff; border: none; border-radius: 5px; cursor: pointer; transition: background 0.3s; }"
                "input[type='submit']:hover { background-color: #0056b3; }"
                ".button { padding: 10px 20px; font-size: 1em; color: white; background-color: #007bff; border: none; border-radius: 5px; cursor: pointer; transition: background 0.3s; text-decoration: none; display: inline-block; margin-top: 15px; }"
                ".button:hover { background-color: #0056b3; }"
                "</style>"
                "</head><body>"
                "<h1>Wi-Fi Configuration</h1>"
                "<form action=\"/save\" method=\"POST\">"   // Form to enter SSID and password
                "<label for=\"ssid\">SSID:</label>"
                "<input type=\"text\" name=\"ssid\" id=\"ssid\" required><br>"
                "<label for=\"password\">Password:</label>"
                "<input type=\"password\" name=\"password\" id=\"password\" required>"
                "<input type=\"checkbox\" onclick=\"togglePassword()\"> Show Password<br><br>" // Checkbox to toggle password visibility
                "<input type=\"submit\" value=\"Save\">"    // Submit button
                "</form>"
                "<a href=\"/\" class=\"button\">Return to Main Page</a>" // Link styled as a button for returning to main page
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

  // Display temporary message about configuration save
  displayTempMessage("WiFi Config", "Saved & Restart");

  // Confirmation page HTML
  String html = "<!DOCTYPE html><html><head><title>Configuration Saved</title>"
                "<style>"
                "body { font-family: Arial, sans-serif; text-align: center; background-color: #f3f3f9; color: #333; padding: 20px; }"
                "h1 { color: #007bff; }"
                "p { font-size: 1.1em; }"
                "</style>"
                "</head><body><h1>Configuration Saved</h1>"
                "<p>The device will now attempt to connect to the Wi-Fi network.</p>"
                "</body></html>";
  server.send(200, "text/html", html); // Send confirmation page to client

  delay(2000);   // Wait for 2 seconds before restarting
  ESP.restart(); // Restart ESP32 to attempt Wi-Fi connection with new credentials
}
