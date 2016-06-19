#include "application.h"
#include "libraries/AES/AES.h"
#include "libraries/AnalogSensor/AnalogSensor.h"
#include "libraries/DHT/DHT.h"
#include "libraries/CO2Monitor/CO2Monitor.h"
#include "libraries/DustSensor/DustSensor.h"
#include "key.h"

SYSTEM_MODE(MANUAL);

// A UDP instance to let us send and receive packets over UDP
UDP udp;
unsigned int port = 5683;
IPAddress ip(199, 223, 216, 35);
DHT dht(D4, DHT22);
DustSensor dust(D6);
AnalogSensor light(A0);
AnalogSensor voc(A1);
CO2Monitor co2;
String myIDStr = Particle.deviceID();
char id[24];
FuelGauge fuel;
unsigned int nextTime = 0;
unsigned int next = 600000;//10 minutes
unsigned int wait = 10000;//how long wait for ack packet before resending
unsigned int wait_packet = 0;
unsigned int attempts = 0;
unsigned int max_attempts = 5;
bool sent = false;
bool response = false;


void connect_cellular(){
    Cellular.on();
    while(!Cellular.ready()){
        Cellular.connect();
        Serial.println("Connecting to Cellular Network...");
        delay(500);
    }
}

void fix_connection(){
    Serial.println("Resetting Network Stack");
    System.sleep(SLEEP_MODE_DEEP, 60);
}

void setup() {
    Serial.begin(9600);
    Serial.println("beginning");
    RGB.control(true);
    RGB.color(0, 255, 255);
    RGB.brightness(255);
    myIDStr.toCharArray(id, 24);
    connect_cellular();
    udp.begin(port);
    Serial.println("running");
    light.init();
    voc.init();
    co2.init();
    dht.begin();
    dust.begin();
}

bool check_packet(){
    if(udp.parsePacket() > 0){
        char ack = udp.read();
        Serial.print("ACK: "); Serial.println(ack);
        udp.flush();
        return true;
    }
    return false;
}

void send_packet(){
    if(Cellular.ready()){
        light.read();
        voc.read();
        dust.read();
        char json_body[128];
        unsigned char body_out[256];
        CellularSignal sig = Cellular.RSSI();
        sprintf(json_body, "{\"t\":%.2f,\"h\":%.2f,\"l\":%d,\"c\":%d,\"v\":%d,\"d\":%.2f,\"n\":130,\"i\":\"%s\",\"f\":%.2f,\"r\":%d}", dht.readTemperature(false), dht.readHumidity(), light.read(), co2.read(), voc.read(), dust.read(), id, fuel.getSoC(), sig.rssi);
        aes_128_encrypt(json_body, KEY, body_out);
        Serial.println(Cellular.localIP());
        Serial.println(json_body);
        udp.beginPacket(ip, port);
        int bytes = udp.write(body_out, 160);
        udp.endPacket();
        Serial.println(bytes);
        wait_packet = millis()+wait;
        attempts++;
        if(attempts > max_attempts){
            fix_connection();
        }
    }else{
        connect_cellular();
    }
}

void loop() {
    if(sent && !response && check_packet()){
        sent = false;
        response = true;
        attempts = 0;
        Serial.println("ACK received!");
    }else if(sent && !response && (millis() > wait_packet)){
        Serial.println("No ACK: resend");
        send_packet();
    }
    if (nextTime > millis()) return;
    send_packet();
    sent = true;
    response = false;
    nextTime = millis() + next;
}
