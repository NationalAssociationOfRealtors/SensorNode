#include "application.h"
#include "libraries/AES/AES.h"
#include "libraries/AnalogSensor/AnalogSensor.h"
#include "libraries/DHT/DHT.h"
#include "libraries/CO2Monitor/CO2Monitor.h"
#include "libraries/HttpClient/HttpClient.h"
#include "libraries/DustSensor/DustSensor.h"
#include "key.h"

SYSTEM_MODE(MANUAL);

// An UDP instance to let us send and receive packets over UDP
UDP udp;
unsigned int port = 5005;
IPAddress ip(192, 168, 1, 45);
DHT dht(D4, DHT22);
DustSensor dust(D6);
AnalogSensor light(A0);
AnalogSensor voc(A1);
CO2Monitor co2;
String myIDStr = Particle.deviceID();
char id[24];
FuelGauge fuel;

void connect_cellular(){
    Cellular.on();
    while(!Cellular.ready()){
        Cellular.connect();
        Serial.println("Connecting to Cellular Network...");
        delay(500);
    }
}

void fix_connection(){
    Serial.println("Fixing Network Connection");
    Cellular.disconnect();
    delay(1000);
    Cellular.off();
    delay(1000);
    Cellular.on();
    delay(1000);
    connect_cellular();
}

void setup() {
    Serial.begin(9600);
    Serial.println("beginning");
    RGB.control(true);
    RGB.color(0, 255, 255);
    RGB.brightness(255);
    myIDStr.toCharArray(id, 24);
    connect_cellular();
    Serial.println("running");
    Serial.println("Setup Response Callback");
    udp.begin(8008);
    light.init();
    voc.init();
    co2.init();
    dht.begin();
    dust.begin();
}

unsigned int nextTime = 0;
unsigned int next = 600000;//10 minutes
void loop() {
    light.read();
    voc.read();
    dust.read();
    if (nextTime > millis()) return;
    nextTime = millis() + next;
    if(Cellular.ready()){
        char json_body[128];
        unsigned char body_out[256];
        CellularSignal sig = Cellular.RSSI();
        sprintf(json_body, "{\"t\":%.2f,\"h\":%.2f,\"l\":%d,\"c\":%d,\"v\":%d,\"d\":%.2f,\"n\":130,\"i\":\"%s\",\"f\":%.2f,\"r\":%d}", dht.readTemperature(false), dht.readHumidity(), light.read(), co2.read(), voc.read(), dust.read(), id, fuel.getSoC(), sig.rssi);
        aes_128_encrypt(json_body, KEY, body_out);
        Serial.println(json_body);
        udp.beginPacket(ip, port);
        int bytes = udp.write(body_out, 160);
        udp.endPacket();
        Serial.println(bytes);
    }else{
        connect_cellular();
    }
}
