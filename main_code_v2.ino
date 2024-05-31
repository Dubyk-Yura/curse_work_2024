#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ThingSpeak.h"
#include "DHT.h"
#include "GRGB.h"

#define DHT22PIN D3
#define DHTTYPE DHT22
#define R_PIN D7
#define G_PIN D5
#define B_PIN D6

WiFiClient client;
DHT dht(DHT22PIN, DHTTYPE);
AsyncWebServer server(80);
GRGB led(COMMON_CATHODE, R_PIN, G_PIN, B_PIN);

const char *ssid = "TP-Link_1BA9";
const char *password = "982119225";
unsigned long myChannelNumber = 1;
const char *myWriteAPIKey = "SI9M1TOKNN0BWF22";
uint32_t timeStamp;
uint32_t sendDataDelay = 20000;
uint32_t ledTimeStamp;
uint16_t ledDelay = 25;
uint16_t i = 0;
float temperature;
float humidity;
float humidityAcceptableLevel = 70;
bool allowToSendData = true;
bool firstSuccessRequest = true;

void setup() {
  WiFi.mode(WIFI_STA);
  pinMode(R_PIN, OUTPUT);
  pinMode(G_PIN, OUTPUT);
  pinMode(B_PIN, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  ThingSpeak.begin(client);
  timeStamp = sendDataDelay - 100;
}

void loop() {
  if (humidity < humidityAcceptableLevel) {
    if (millis() - ledTimeStamp > ledDelay) {
      led.setWheel(i);
      i++;
      if (i == 1530) {
        i = 0;
      }
    }

  } else {
    digitalWrite(R_PIN, LOW);
    digitalWrite(G_PIN, LOW);
    digitalWrite(B_PIN, LOW);
  }

  checkTemperatureSendData();
}

void checkTemperatureSendData() {
  if ((millis() - timeStamp) > sendDataDelay) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Attempting to connect");
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
      }
      Serial.println("Connected.");
    }
    temperature = dht.readTemperature();
    Serial.print("Temperature (ºC): ");
    Serial.println(temperature);
    humidity = dht.readHumidity();
    Serial.print("Humidity (%): ");
    Serial.println(humidity);
    if (allowToSendData) {
      ThingSpeak.setField(1, temperature);
      ThingSpeak.setField(2, humidity);
      int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

      if (x == 200) {
        Serial.println("Channel update successful.");
        if (firstSuccessRequest) {
          Serial.print("ESP IP Address: http://");
          IPAddress IP = WiFi.localIP();
          Serial.println(IP);
          initSite();
        }
      }
    }

    timeStamp = millis();
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial;
            text-align: center;
            margin: 0px auto;
            padding-top: 30px;
        }

        .button {
            padding: 10px 20px;
            margin-bottom: 50px;
            font-size: 24px;
            text-align: center;
            outline: none;
            color: #fff;
            background-color: #2f4468;
            border: none;
            border-radius: 5px;
            box-shadow: 0 6px #999;
            cursor: pointer;
            -webkit-tap-highlight-color: rgba(0, 0, 0, 0);
        }

        .button:hover {
            background-color: #1f2e45
        }

        .button:active {
            background-color: #1f2e45;
            box-shadow: 0 4px #666;
            transform: translateY(2px);
        }

        .input1 {
            margin-bottom: 30px;
            padding: 6px 12px;
            font-size: 16px;
            font-weight: 700;
            line-height: 1.5;
            color: #212529;
            background-color: #fff;
            background-clip: padding-box;
            border: 1px solid #ced4da;
            appearance: none;
            border-radius: 4px;
            transition: border-color .15s ease-in-out, box-shadow .15s ease-in-out;
        }

        .label1 {
            font-size: 28px;
        }

        @media screen and (max-width: 480px) {
            .button {
                padding: 15px 100px 15px 10px;
                font-size: 10px;
            }

            h1 {
                font-size: 24px;
                padding-top: 20px;
            }
        }
    </style>
</head>

<body>
    <h1>ESP air freshener project</h1>
    <div>
        <button class="button" id="wifiButton" onclick="sendDataAccess();">
            Send data by wifi - <span id="wifiStatus">yes</span>
        </button>
    </div>
    <div>
        <label for="delay" class="label1">Delay between check temperature (in milliseconds):</label>
        <input type="number" class="input1" id="temperature" oninput="validateIntegerInput(this)" min="100"
            max="7200000">
        <button class="button" onclick="sendTemperatureData()">Send</button>
    </div>
    <div>
        <label for="levelHumidity" class="label1">Humidity when to start the device:</label>
        <input type="number" class="input1" id="humidity" oninput="validateIntegerInput(this)" min="0" max="100">
        <button class="button" onclick="sendHumidityData()">Send</button>
    </div>
    <script>
        var temperature = document.getElementById('temperature');
        var humidity = document.getElementById('humidity');

        function validateIntegerInput(input) {
            input.value = input.value.replace(/[^0-9]/g, '');
        }

        function sendDataAccess() {
            var wifiStatus = document.getElementById('wifiStatus');
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/", true);
            xhr.send();
            if (wifiStatus.textContent === 'yes') {
                xhr.open("GET", "/wifi_false", true);
                xhr.send();
                wifiStatus.textContent = 'no';
            } else {
                xhr.open("GET", "/wifi_true", true);
                xhr.send();
                wifiStatus.textContent = 'yes';
            }
        }

        function sendTemperatureData() {
            var tempValue = temperature.value;
            if (tempValue >= 100 && tempValue <= 7200000) {
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/set_temperature?value=" + tempValue, true);
                xhr.send();
            } else {
                alert("Please enter a temperature value between 100 and 7200000.");
            }
        }

        function sendHumidityData() {
            var humidityValue = humidity.value;
            if (humidityValue >= 0 && humidityValue <= 100) {
                var xhr = new XMLHttpRequest();
                xhr.open("GET", "/set_humidity?value=" + humidityValue, true);
                xhr.send();
            } else {
                alert("Please enter a humidity value between 0 and 100.");
            }
        }
    </script>
</body>

</html>
)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

void initSite() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/wifi_true", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Send data by wifi - true");
    allowToSendData = true;
    request->send(200, "text/plain", "ok");
  });

  server.on("/wifi_false", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Send data by wifi - false");
    allowToSendData = false;
    request->send(200, "text/plain", "ok");
  });

  server.on("/set_temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      sendDataDelay = value.toInt();
      Serial.print("Temperature value received: ");
      Serial.println(value);
    }
    request->send(200, "text/plain", "ok");
  });
  server.on("/set_humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      humidityAcceptableLevel = value.toInt();
      Serial.print("Humidity value received: ");
      Serial.println(value);
    }
    request->send(200, "text/plain", "ok");
  });

  server.onNotFound(notFound);
  server.begin();
}