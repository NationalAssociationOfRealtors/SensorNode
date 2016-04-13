#include "application.h"
#include "libraries/AES/AES.h"
#include "libraries/AnalogSensor/AnalogSensor.h"
#include "libraries/DHT/DHT.h"
#include "libraries/CO2Monitor/CO2Monitor.h"
#include "libraries/HttpClient/HttpClient.h"
#include "libraries/DustSensor/DustSensor.h"
#include "key.h"

SYSTEM_MODE(MANUAL);

UDP udp;
unsigned int port = 5683;
IPAddress ip(63, 156, 247, 113);
DHT dht(D4, DHT22);
DustSensor dust(D6);
AnalogSensor light(A0);
AnalogSensor voc(A1);
CO2Monitor co2;
char id[36];
String myIDStr = Particle.deviceID();

void connect_wifi(){
    WiFi.on();
    while(!WiFi.ready()){
        WiFi.connect();
        Serial.println("Connecting to WiFi Network...");
        delay(500);
    }
}

void fix_connection(){
    Serial.println("Fixing Network Connection");
    WiFi.disconnect();
    delay(1000);
    WiFi.off();
    delay(1000);
    WiFi.on();
    delay(1000);
    connect_wifi();
}

void setup() {
    Serial.begin(9600);
    Serial.println("beginning");
    RGB.control(true);
    RGB.color(0, 255, 255);
    RGB.brightness(255);
    connect_wifi();
    Serial.println("running");
    myIDStr.toCharArray(id, 36);
    udp.begin(port);
    light.init();
    voc.init();
    co2.init();
    dht.begin();
    dust.begin();
}

unsigned int nextTime = 0;
unsigned int next = 1000;//1 second
void loop() {
    light.read();
    voc.read();
    dust.read();
    if (nextTime > millis()) return;
    nextTime = millis() + next;
    if(WiFi.ready()){
        udp.stop();
        udp.begin(port);
        Serial.println(WiFi.localIP());
        char json_body[160];
        unsigned char body_out[256];
        sprintf(json_body, "{\"t\":%.2f,\"h\":%.2f,\"l\":%.2f,\"c\":%.2f,\"v\":%d,\"d\":%d,\"n\":130.0,\"i\":\"%s\"}", dht.readTemperature(false), dht.readHumidity(), light.read(), co2.read(), voc.read(), dust.read(), id);
        aes_128_encrypt(json_body, KEY, body_out);
        Serial.println(myIDStr);
        Serial.println(id);
        Serial.println(json_body);
        int bytes = udp.sendPacket(body_out, 160, ip, port);
        if (bytes < 0) {
            Serial.println("UDP Error");
        }
        Serial.println(bytes);
    }else{
        connect_wifi();
    }
}
