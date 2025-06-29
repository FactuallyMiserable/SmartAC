# Smart AC Controller (ESP8266)

This project combines skills in electronics and embedded systems to transform a traditional ("dumb") AC unit into a web-controlled smart device. It uses an ESP8266 microcontroller programmed in the Arduino language (C/C++) to simulate remote button presses using BJT transistors and exposes a REST API via a local web server. Additionally, it updates a free DNS record using your current public IP address, allowing you to control your AC remotely even if your IP changes.

---

## 🛠 Features

- REST API to control AC state and temperature
- Supports different AC modes (cool, dry, fan, heat)
- Secure header-based authentication
- Periodic public IP checking and auto DNS record update (FreeDNS/Afraid.org)
- Hardware button simulation via GPIO (using BJT transistors)

---

## 📦 Installation

### 1. Hardware Setup

- ESP8266 NodeMCU board (e.g., ESP-12E or similar)
- NPN BJT transistors (e.g., 2N2222) to simulate button presses
- Connect each transistor's base to a GPIO pin via a ~1kΩ resistor
- Common emitter connected to ground
- Collectors go to the corresponding remote control button
- Optional: 5V relay or logic-level MOSFET to power on/off the remote

### 2. Arduino Environment Setup

1. Install the Arduino IDE (https://www.arduino.cc/en/software)
2. Add ESP8266 support:
   - Go to **File → Preferences**
   - In the “Additional Boards Manager URLs” field, add:
     ```
     http://arduino.esp8266.com/stable/package_esp8266com_index.json
     ```
   - Open **Tools → Board → Boards Manager**
   - Search for `ESP8266` and install

3. Select your board:
   - Go to **Tools → Board → NodeMCU 1.0 (ESP-12E Module)**

4. Install Required Libraries:
   - `ESP8266WiFi`
   - `ESP8266WebServer`
   - `ESP8266HTTPClient`
   - `WiFiClientSecure` (included with ESP8266 package)

---

## 🔧 Configuration

Open the `.ino` or `.cpp` file and replace the following macros with your own values:

```cpp
#define WIFI_SSID       "YourWiFiName"
#define WIFI_PASSWORDS  "YourWiFiPassword"

#define SUBDOMAIN       "example.mooo.com"
#define AFRAID_API_URL  "https://freedns.afraid.org/dynamic/update.php?xxxxxxxxxxxx"
#define AUTH_KEY        "YourCustomAuthKey"
```

---

## 📡 REST API Examples

### ✅ Turn On the AC (Set to 24°C, Cool Mode)

**Endpoint:**  
```

## API
GET /aircon
```

**Required Headers:**

| Header | Value Example   | Description                        |
|--------|------------------|------------------------------------|
| `Auth` | `MySecretKey123` | Your authentication key            |
| `Temp` | `24`             | Temperature (16–30 allowed)        |
| `Mode` | `c`              | Mode: `c`=Cool, `d`=Dry, `v`=Fan, `h`=Heat |

#### 💻 Example via `curl`:

```bash
curl -X GET http://your-subdomain.example.com/aircon \
  -H "Auth: MySecretKey123" \
  -H "Temp: 24" \
  -H "Mode: c"
```

#### 📬 Example via Postman:

- **Method:** `GET`
- **URL:** `http://your-subdomain.example.com/aircon`
- **Headers:**
  - `Auth: MySecretKey123`
  - `Temp: 24`
  - `Mode: c`

---

### ❌ Turn Off the AC

**Endpoint:**  
```
GET /turnoff
```

**Required Header:**
| Header | Value Example   | Description             |
|--------|------------------|-------------------------|
| `Auth` | `MySecretKey123` | Your authentication key |

#### 💻 Example via `curl`:

```bash
curl -X GET http://your-subdomain.example.com/turnoff \
  -H "Auth: MySecretKey123"
```

#### 📬 Postman:

- **Method:** `GET`
- **URL:** `http://your-subdomain.example.com/turnoff`
- **Header:**
  - `Auth: MySecretKey123`

---

### 🧪 Response Examples

#### If Auth is wrong or missing:
```
HTTP/1.1 401 Unauthorized
Unauthorized
```

#### If temperature is invalid (outside 16–30 range):
```
HTTP/1.1 500 Internal Server Error
Temp is invalid
```

#### On success:
```
HTTP/1.1 200 OK
Aircon is now ON
```
or
```
Aircon is now OFF
```

---
