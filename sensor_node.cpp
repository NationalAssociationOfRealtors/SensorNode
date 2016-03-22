#include "application.h"
#include "libraries/AES/AES.h"
#include "libraries/AnalogSensor/AnalogSensor.h"
#include "libraries/DHT/DHT.h"
#include "libraries/CO2Monitor/CO2Monitor.h"
#include "libraries/HttpClient/HttpClient.h"
#include "libraries/DustSensor/DustSensor.h"
#include "key.h"

SerialDebugOutput debugOutput(9600, ALL_LEVEL);

SYSTEM_MODE(MANUAL);

DHT dht(D4, DHT22);
DustSensor dust(D6);
AnalogSensor light(A0);
AnalogSensor voc(A1);
CO2Monitor co2;
String myIDStr = Particle.deviceID();
String API_VERSION = String("v1.0");
HttpClient http;
FuelGauge fuel;
char path[64];
char cookie[32];

http_request_t request;
http_response_t response;
http_header_t headers[] = {
    { "Content-Type", "application/json" },
    { "Accept" , "application/json" },
    { "Cookie", "lablog=2983749283749287349827349" },
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};

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
    connect_cellular();
    Serial.println("running");
    Serial.println(request.path);
    String s_path = String("/api/"+API_VERSION+"/node/"+myIDStr+"/sensors");
    s_path.toCharArray(path, 64);
    String cook = String("lablog="+myIDStr);
    cook.toCharArray(cookie, 32);
    request.hostname = "crtlabsdev.realtors.org";
    request.port = 80;
    request.path = path;
    light.init();
    voc.init();
    co2.init();
    dht.begin();
    dust.begin();
}

unsigned int nextTime = 0;
unsigned int next = 1800000;
void loop() {
    light.read();
    voc.read();
    dust.read();
    if (nextTime > millis()) return;
    nextTime = millis() + next;
    if(Cellular.ready()){
        char json_body[96];
        char body_out[96];
        CellularSignal sig = Cellular.RSSI();
        sprintf(json_body, "{\"t\":%.2f,\"h\":%.2f,\"l\":%d,\"c\":%d,\"v\":%d,\"d\":%.2f,\"f\":%.2f,\"n\":130, \"r\":%d}", dht.readTemperature(false), dht.readHumidity(), light.read(), co2.read(), voc.read(), dust.read(), fuel.getSoC(), sig.rssi);
        Serial.println(json_body);
        aes_128_encrypt(json_body, KEY, body_out);
        memcpy(request.body, body_out, 96);
        http.post(request, response, headers);
        Serial.print("Response status: ");
        Serial.println(response.status);
        if(response.status == -1){
            fix_connection();
        }
    }else{
        connect_cellular();
    }
}
