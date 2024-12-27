#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <HomeSpan.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

struct DEV_Identify : Service::AccessoryInformation {

  SpanCharacteristic *identify; // reference to the Identify Characteristic

  DEV_Identify(const char *name, const char *manu, const char *sn,
               const char *model, const char *version)
      : Service::AccessoryInformation() {

    new Characteristic::Name(name); // create all the required Characteristics
                                    // with values set based on above arguments
    new Characteristic::Manufacturer(manu);
    new Characteristic::SerialNumber(sn);
    new Characteristic::Model(model);
    new Characteristic::FirmwareRevision(version);
    identify =
        new Characteristic::Identify(); // store a reference to the Identify
                                        // Characteristic for use below
  }

  boolean update() { return (true); }
};

// A standalone Temperature sensor
struct DEV_TempSensor : Service::TemperatureSensor {

  // reference to the Current Temperature Characteristic
  SpanCharacteristic *temp;

  // constructor() method
  DEV_TempSensor() : Service::TemperatureSensor() {

    // start dhttemp Object
    dht.begin();

    // instantiate the Current Temperature Characteristic
    temp = new Characteristic::CurrentTemperature(-10.0);
    // expand the range from the HAP default of 0-100 to -50 to 100 to allow for
    // negative temperatures
    temp->setRange(-50, 100);

    // initialization message
    Serial.print("Configuring Temperature Sensor");
    Serial.print("\n");

  } // end constructor

  void loop() {

    // the temperature refreshes every 10 seconds by the elapsed time
    if (temp->timeVal() > 10000) {
      // read temperature from sensor dht22
      float temperature = dht.readTemperature();
      // set the new temperature; this generates an Event Notification and also
      // resets the elapsed time
      if (!isnan(temperature)) {
        temp->setVal(temperature);
      }

      Serial.print("Temperature Update: ");
      Serial.print(temperature);
      Serial.print(" ; ");
    }
  } // loop
};

// Replace with your own network credentials
const char *ssid = "wifi";
const char *password = "pass";

void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  display.clearDisplay();
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  homeSpan.setLogLevel(1);
  homeSpan.begin(Category::Bridges, "Sensor Bridge");

  new SpanAccessory();
  new DEV_Identify("HomeKit", "SmartHomeFactory", "123", "Bridge", "0.7");
  new Service::HAPProtocolInformation();
  new Characteristic::Version("1.1.0");

  new SpanAccessory();
  new DEV_Identify("Temp Sensor", "SmartHomeFactory", "123", "Bridge", "0.7");
  new DEV_TempSensor();
}

void loop() {
  delay(2000);
  homeSpan.poll();

  // clear
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  float t = dht.readTemperature();
  display.println("Temperature: ");
  display.print(t);
  display.print(" C \n");

  float h = dht.readHumidity();
  display.println("Humidity: ");
  display.print(h);
  display.print(" %");

  display.display();
}
