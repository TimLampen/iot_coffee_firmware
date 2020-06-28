#include <ESP8266WiFi.h>
#include <Arduino.h>
#include "EEPROM.h"


const char* localSSID    = "IoT Coffee Setup";
const char* localPassword = "iot_coffee_password";
const int ssidAddr = 0;
const int passwordAddr = 64;


WiFiServer server(80);
String header;
int status = 0;//0=booting up, 1=needs wifi details, 2=connected to main wifi


void writeString(char add,String data){
    int _size = data.length();
    int i;
    for(i=0;i<_size;i++){
        EEPROM.write(add+i,data[i]);
    }
    EEPROM.write(add+_size,'\0');
    EEPROM.commit();
}


String readString(char add){
    char data[100]; //Max 100 Bytes
    int len=0;
    unsigned char k;
    k=EEPROM.read(add);
    while(k != '\0' && len<500){
        k=EEPROM.read(add+len);
        data[len]=k;
        len++;
    }
    data[len]='\0';
    return String(data);
}

bool connectToWifi(String ssid, String password){
    Serial.println('\n');

    Serial.println("trying to connect using ssid="+ssid + " password="+password);

    WiFi.begin(ssid, password);             // Connect to the network
    Serial.print("Connecting to ");
    Serial.print(ssid);
    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 60) { // Wait for the Wi-Fi to connect
        delay(500);
        Serial.print('.');
        counter++;
    }

    if(counter==60) {
        Serial.println("Unable to connect to the wifi");
        return false;
    }

    Serial.println('\n');
    Serial.println("Connection established!");
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    server.stop();
    return true;
}

void listenForWifiDetails(){
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New Client.");
        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);
                header += c;
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();


                        String wifiInfo = header.substring(header.indexOf(" /"), header.indexOf(" HTTP"));
                        wifiInfo.replace("/", "");

                        String wifiSSID = wifiInfo.substring(wifiInfo.indexOf(" ")+1, wifiInfo.indexOf("@"));
                        wifiSSID.replace("%20"," ");

                        String wifiPassword = wifiInfo.substring(wifiInfo.indexOf("@")+1, wifiInfo.length());

                        Serial.println("WIFI INFO = " + wifiSSID + " / " + wifiPassword);

                        writeString(ssidAddr, wifiSSID);
                        writeString(passwordAddr, wifiPassword);

                        if(connectToWifi(wifiSSID, wifiPassword)){
                            status = 2;
                            return;
                        }
                        client.println();
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        header = "";
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("setting up...");

    delay(2000);
    Serial.println("trying to read string");

    if(readString(ssidAddr)!=nullptr){
        Serial.println("not null");
        String SSID = readString(ssidAddr);
        String password = readString(passwordAddr);
        Serial.println("read SSID/Password=" +  SSID + "/" + password);
        if(connectToWifi(SSID, password)) {
            status = 2;
            return;
        }
    }

    delay(2000);

    Serial.println("null ssid and password");

    status = 1;
    Serial.println("Setting AP (Access Point)…");
    WiFi.softAP(localSSID, localPassword);

    IPAddress IP = WiFi.softAPIP();
    Serial.println("AP IP address: ");
    Serial.println(IP);

    server.begin();
}

void loop(){
    if(status == 1)
        listenForWifiDetails();
}
