#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <map>

ESP8266WebServer server(80);

std::map<String, String> requestHeaders;

#define PLUS          13
#define MINUS         12
#define MODE          15
#define TOGGLE_ON_OFF 14
#define POWER_5V      5

// Creds
#define WIFI_SSID <wifi_ssid>
#define WIFI_PASSWORDS <wifi_password>
#define SUBDOMAIN <subdomain>
#define AFRAID_API_URL <afraid_api_url> //freedns.afraid.org
#define AUTH_KEY <auth_key>

int outputs[] = {PLUS, MINUS, MODE, TOGGLE_ON_OFF, POWER_5V};

struct TempInt {
    unsigned int value : 5; // 5 bits for storing values between 0 and 31
};

const char* ssid = WIFI_SSID; // Wi-Fi SSID
const char* password = WIFI_PASSWORDS; // Wi-Fi password

const char* subdomain = SUBDOMAIN;
const char* afraidApiUrl = AFRAID_API_URL;

String authKey = AUTH_KEY;

unsigned long previousMillis = 0; // Stores the last time action was taken
const int minutesForWifiCheck = 5;
const long interval = minutesForWifiCheck * 60 * 1000; // 5 minutes in milliseconds (5 * 60 * 1000)

void powerAC(bool on) {
  if (on) {
    digitalWrite(POWER_5V, HIGH);
    delay(5000);
  } else {
    digitalWrite(POWER_5V, LOW);
    delay(250);
  }
}

void pushButton(int pin) {
  digitalWrite(pin, HIGH);
  delay(300);
  digitalWrite(pin, LOW);
  delay(250);
}

void toggleOnOff(void) {
  pushButton(TOGGLE_ON_OFF);
}

void handleAircon(void) {
  if (requestHeaders["Auth"] != authKey) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }

  char mode = (char)requestHeaders["Mode"][0];
  struct TempInt temp;
  temp.value = atoi(requestHeaders["Temp"].c_str());

  if (temp.value > 30 || temp.value < 16) {
    server.send(500, "text/plain", "Temp is invalid");
    return;
  }

  server.send(200, "text/plain", "Aircon is now ON");

  aircon(mode, temp);
  delay(150);
}


void aircon(char mode, struct TempInt temp) {
  // Give power to remote and wait for it to load
  powerAC(true);

  // Toggle it on
  toggleOnOff();

  bool error = setMode(mode);


  if (error) {
    // Turn ac off to avoid problems
    toggleOnOff();

    // Cut power
    powerAC(false);
    return;
  }

  changeTemp(temp, mode);
  delay(100);

  // Cut power
  powerAC(false);
}

bool setMode(char mode) {
  unsigned int times = 0;
  
  switch (mode) {
    case 'c':
      times = 1;
      break;
    case 'd':
      times = 2;
      break;
    case 'v':
      times = 3;
      break;
    case 'h':
      times = 4;
      break;
    default:
      return true;
  }

  for (int i = 0; i < times; i++) {
    pushButton(MODE);
  }

  return false;
}

void changeTemp(struct TempInt temp, char mode) {
  int change = temp.value - calcTemp(mode);

  if (change > 0) {
    //PLUS
    for (int i = 0; i < change; i++) {
      pushButton(PLUS);
    }
  } else if (change < 0) {
    //MINUS
    for (int i = 0; i < -1 * change; i++) {
      pushButton(MINUS);
    }
  }
}

int calcTemp(char mode) {
  // 25: C, D, V
  // 28: H
  switch (mode) {
    case 'h':
      return 28;
      break;
    default:
      return 25;
  }
}

void handleTurnoff() {
  if (requestHeaders["Auth"] != authKey) {
    server.send(401, "text/plain", "Unauthorized");
    return;
  }
  // Example logic for /turnoff endpoint
  turnOff();
  server.send(200, "text/plain", "Aircon is now OFF");
}

void turnOff() {
  powerAC(true);

  toggleOnOff();
  toggleOnOff();

  powerAC(false);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void captureHeaders() {
  // Clear the dictionary/map before adding new headers
  requestHeaders.clear();
  
  // Populate the dictionary with request headers
  for (int i = 0; i < server.headers(); i++) {
    String headerName = server.headerName(i);
    String headerValue = server.header(i);
    requestHeaders[headerName] = headerValue;
    
    // Optionally print the headers to the Serial Monitor for debugging
    // Serial.print(headerName);
    // Serial.print(": ");
    // Serial.println(headerValue);
  }
}


void mountOutputs(void) {
  for (int i = 0; i < (sizeof(outputs) / sizeof(outputs[0])); i++) {
    pinMode(outputs[i], OUTPUT);
  }
}

void connectWiFi(void) {
  WiFi.begin(ssid, password); // Connect to Wi-Fi

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected to Wi-Fi!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Print ESP32's IP address
}

bool ipMatchesSubdomain(String publicIP, String subdomainIP) {
  return (publicIP == subdomainIP);
}

String getSubdomainIP(const char* subdomain) {
  IPAddress domainIP;
  if (WiFi.hostByName(subdomain, domainIP)) {
    return domainIP.toString();
  } else {
    Serial.println("An error occured fetching subdomain's current ip.");
    return "";
  }
}

String getPublicIP(void) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client,"http://api.ipify.org"); // API to fetch public IP
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String publicIP = http.getString(); // Get response payload as string
    // Serial.print("Public IP Address: ");
    // Serial.println(publicIP);

    return publicIP;
  } else {
    Serial.print("Error in HTTP request, Code: ");
    Serial.println(httpResponseCode);
  }
  
  return "";
  http.end(); // Close connection
}

void updateDNS(const String& ipAddress, const char* afraidApiUrl) {
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

  client->setInsecure();

  HTTPClient https;

  String fullUrl = String(afraidApiUrl) + "&address=" + ipAddress; // Append IP to the update URL
  https.begin(*client, fullUrl); // Initialize HTTP request
  int httpResponseCode = https.GET(); // Send GET request

  if (httpResponseCode > 0) {
    String response = https.getString(); // Get response payload
    Serial.println("DNS Update Response:");
    Serial.println(response);
  } else {
    Serial.print("Error updating DNS. HTTP Code: ");
    Serial.println(httpResponseCode);
  }

  https.end(); // Close connection
}

void wifiSetup(void) {
  connectWiFi();

  //Update DNS record's IP if necessary
  String externalIP = getPublicIP();
  Serial.print("Public IP Address: ");
  Serial.println(externalIP);

  if (!ipMatchesSubdomain(externalIP, getSubdomainIP(subdomain))) {
    Serial.println("External IP address doesn't match domain. Updating the DNS record!");
    updateDNS(externalIP, afraidApiUrl);
  }
}

void webserverSetup(void) {
  server.on("/aircon", HTTP_GET, []() {
    captureHeaders(); // Capture headers when /aircon is accessed
    handleAircon();
  });

  server.on("/turnoff", HTTP_GET, []() {
    captureHeaders(); // Capture headers when /turnoff is accessed
    handleTurnoff();
  });

  // Handle not found routes
  server.onNotFound(handleNotFound);

  // Start the server
  const char *headerKeys[] = {"Mode", "Auth", "Temp"};
  size_t headerKeysSize = sizeof(headerKeys) / sizeof(char *);
  server.collectHeaders(headerKeys, headerKeysSize);
  server.begin();
  Serial.println("Web server started");
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("========================================");
  mountOutputs();
  
  wifiSetup();

  webserverSetup();
}

void loop() {
  unsigned long currentMillis = millis(); // Get the current time
  // put your main code here, to run repeatedly:
  if (WiFi.status() != WL_CONNECTED){
    wifiSetup();
    return;
  }

  if (currentMillis - previousMillis >= interval) {
    // Save the last time the action was executed
    previousMillis = currentMillis;

    String externalIP = getPublicIP();
    if (!ipMatchesSubdomain(externalIP, getSubdomainIP(subdomain))) {
      Serial.println("External IP address doesn't match domain. Updating the DNS record!");
      updateDNS(externalIP, afraidApiUrl);
    }
  }

  server.handleClient();

}
