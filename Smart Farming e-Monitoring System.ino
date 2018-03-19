                            //Smart Farming e-Monitoring System
                          //BSKAP-A-THON 2k18 Competition
                          

#include <ArduinoJson.h>
#include <BoodskapCommunicator.h>
#include <DHT.h>

#define CONFIG_SIZE 512
#define REPORT_INTERVAL 10000 //10 Seconds
#define MESSAGE_ID 3124 //Message defined in the platform

#define DHTPIN D2
#define DHTTYPE DHT11

//Moisture Readings
int moisturePin = A0;
int Moisture = 0;

//Sunlight
int pinLight = D0;

//Flow Readings

byte sensorInterrupt = 0;  // Node MCU = D3
byte sensorPin       = 2;

float calibrationFactor = 4.5;
volatile byte pulseCount;  
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;

int sample = 0;


#define DEF_WIFI_SSID "Hackathon"  //Your WiFi SSID
#define DEF_WIFI_PSK "Smart Farming" //Your WiFi password
#define DEF_DOMAIN_KEY "TJGYUOZMUF" //your DOMAIN Key
#define DEF_API_KEY "ebqaneP8KmCR" //Your API Key
#define DEF_DEVICE_MODEL "BSKP-GSR" //Your device model
#define DEF_FIRMWARE_VER "1.0.0" //Your firmware version

BoodskapTransceiver Boodskap(UDP); //MQTT, UDP, HTTP
DHT dht(DHTPIN, DHTTYPE);
uint32_t lastReport = 0;

void sendReading();

void setup() {

  Serial.begin(115200);
  pinMode(5,OUTPUT); //Relay
   //flow reading
       
      pinMode(sensorPin, INPUT);
      digitalWrite(sensorPin, HIGH);
      pulseCount        = 0;
      flowRate          = 0.0;
      flowMilliLitres   = 0;
      totalMilliLitres  = 0;
      oldTime           = 0;
      attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

  StaticJsonBuffer<CONFIG_SIZE> buffer;
  JsonObject &config = buffer.createObject();

  config["ssid"] = DEF_WIFI_SSID;
  config["psk"] = DEF_WIFI_PSK;
  config["domain_key"] = DEF_DOMAIN_KEY;
  config["api_key"] = DEF_API_KEY;
  config["dev_model"] = DEF_DEVICE_MODEL;
  config["fw_ver"] = DEF_FIRMWARE_VER;
  config["dev_id"] = String("ESP8266-") + String(ESP.getChipId()); //Your unique device ID

  config["api_path"] = "https://api.boodskap.io"; //HTTP API Base Path Endpoint
  config["api_fp"] = "B9:01:85:CE:E3:48:5F:5E:E1:19:74:CC:47:A1:4A:63:26:B4:CB:32"; //In case of HTTPS enter your server fingerprint (https://www.grc.com/fingerprints.htm)
  config["udp_host"] = "udp.boodskap.io"; //UDP Server IP
  config["udp_port"] = 5555; //UDP Server Port
  config["mqtt_host"] = "mqtt.boodskap.io"; //MQTT Server IP
  config["mqtt_port"] = 1883; //MQTT Server Port
  config["heartbeat"] = 45; //seconds

  Boodskap.setup(config);
}

void loop() {

  Boodskap.loop();

  if ((millis() - lastReport) >= REPORT_INTERVAL) {
    sendReading();
    lastReport = millis();
  }
}

void sendReading() {

//Temperature and Humidity
 float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temperature = dht.readTemperature();
  Serial.print("Temperature = ");
  Serial.println(temperature);
  Serial.print("C");
  Serial.print("Humidity = ");
  Serial.println(humidity);
  Serial.print("%");
  

 //Sunlight Readings
  
  int lightsensor = digitalRead(pinLight);
  Serial.print(" \n light=");
  Serial.print(lightsensor);
  delay(1000);
  

 //Moisture readings
     int output_value = analogRead(moisturePin);
     Moisture = map(output_value,1024,500,0,100);
     Serial.println("Moisture = ");
     Serial.println(Moisture);
     Serial.println("%");
     
    if(Moisture<90)
  {
     digitalWrite(5,LOW);
     Serial.println("soil is dry and pump ON");
  }
 else if(Moisture>90 && Moisture<120)
 {
     digitalWrite(5,LOW);
     Serial.println("soil is humid"); 
 }
 else
  {
     digitalWrite(5,HIGH);
     Serial.println("soil is wet and pump OFF");
  }
     delay(1000);

    
//Flow readings
    if((millis() - oldTime) > 1000)   
   { 
    detachInterrupt(sensorInterrupt);
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    oldTime = millis();  
    flowMilliLitres = (flowRate / 60) * 1000;
    totalMilliLitres += flowMilliLitres;      
    unsigned int frac;    
    Serial.println("Flow rate: ");
    Serial.print(int(flowRate)); 
    Serial.print("L/min");
    Serial.print("\t");    
    Serial.print("Output Liquid Quantity: ");        
    Serial.print(totalMilliLitres);
    Serial.println("mL"); 
    Serial.print("\t");       
    Serial.print(totalMilliLitres);
    Serial.print("L");
    pulseCount = 0;    
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }

  
  StaticJsonBuffer<512> buffer;
  JsonObject &data = buffer.createObject();
  data["t"] = temperature;
  data["h"] = humidity;
  data["light"] = lightsensor;
  data["moisture"] = Moisture; 
  data["flow"] = flowRate;
  data["totalml"] = totalMilliLitres;
  
   
  Boodskap.sendMessage(MESSAGE_ID, data);
}


void pulseCounter()
{
  pulseCount++;
}  
