#include <Arduino.h>

#include "task_networking.h"

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Ethernet.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <USB.h>

EthernetClient ethclient;
WiFiClient wificlient;

Client *client;

extern USBCDC USBSerial;

#define TASK_NETWORKING_FREQ 500

// Ethernet Definitions
byte mac[] = { 0x2C, 0xF7, 0xF1, 0x08, 0x39, 0x7E };
char server[] = "www.google.com";

// Wifi Definitions
char ssid[] = "fleet-monitor";     //  your network SSID (name)
char pass[] = "password";  // your network password

int ethernet_connected = 0;
int wifi_connected = 0;
int connected = 0;

void check_connection_status();
void eth_init();
void wifi_init();

void task_networking(void *pvParameter)
{
    
    eth_init();
    wifi_init();

    const TickType_t task_freq = TASK_NETWORKING_FREQ;
    TickType_t task_last_tick = xTaskGetTickCount();

    while(1){
        check_connection_status();
        if(connected){
            if (client->connect(server, 80)) {
                //USBSerial.print("connected to ");
                //USBSerial.println(client->remoteIP());
                // Make a HTTP request:
                client->println("GET /webhp");
                client->println("Host: www.google.com");
                client->println("Connection: close");
                client->println();
            } else {
                // if you didn't get a connection to the server:
                USBSerial.println("connection failed");
            }
            vTaskDelay(2000);
            int byteCount = 0;
            int len = client->available();
            if (len > 0) {
                byte buffer[80];
                if (len > 80) len = 80;
                client->read(buffer, len);
                USBSerial.write(buffer, len); // show in the serial monitor (slows some boards)
                byteCount = byteCount + len;
            }

            // if the server's disconnected, stop the client:
            if (!client->connected()) {
                USBSerial.println();
                USBSerial.println("disconnecting.");
                client->stop();
                USBSerial.print("Received ");
                USBSerial.println(byteCount);
            }
        }
        vTaskDelayUntil(&task_last_tick, task_freq); 
    } 
}


void eth_init(){
    Ethernet.init(34);
    Ethernet.begin(mac, 1000, 1000);
}

void wifi_init(){
    WiFi.begin(ssid, pass);
}

void check_connection_status(){
    static EthernetHardwareStatus eth_harware_status = EthernetNoHardware;
    static EthernetLinkStatus eth_link_status = Unknown;
    static IPAddress eth_ip(0,0,0,0);

    static wl_status_t wifi_status = WL_NO_SHIELD;
    static IPAddress wifi_ip(0,0,0,0);
    
    /** Ethernet stuff **/
    IPAddress no_ip(0,0,0,0);

    // Diagnostics Print, only on change
    if(eth_harware_status != Ethernet.hardwareStatus() || eth_link_status != Ethernet.linkStatus() || eth_ip != Ethernet.localIP()){
        eth_harware_status = Ethernet.hardwareStatus();
        eth_link_status = Ethernet.linkStatus();
        eth_ip = Ethernet.localIP();
        USBSerial.print("Hardware Status: ");
        USBSerial.println(eth_harware_status == EthernetW5500 ? "UP" : "DOWN");
        USBSerial.print("Link Status: ");
        USBSerial.println(eth_link_status == LinkON ? "UP" : "DOWN");
        USBSerial.print("IP Adress: ");
        USBSerial.println(eth_ip);
    }

    // Check for Hardware connection
    if(Ethernet.hardwareStatus() != EthernetW5500){
        USBSerial.println("No Hardware Found, trying again..");
        Ethernet.begin(mac, 1000, 1000);
    }
    // Check for Link Connection
    else if(Ethernet.linkStatus() != LinkON){
        Ethernet.setLocalIP(no_ip);
    }

    // Check both Link and IP
    if (Ethernet.localIP() == no_ip && Ethernet.linkStatus() == LinkON) {
        USBSerial.println("Waiting for IP..");
        Ethernet.begin(mac, 1000, 1000);
    }  
    
    ethernet_connected = ((Ethernet.localIP() != no_ip) && (Ethernet.linkStatus() == LinkON));

    /** WiFi Stuff **/
    // Diagnostics Print, only on change
    if(wifi_status != WiFi.status() || wifi_ip != WiFi.localIP()){
        wifi_status = WiFi.status();
        wifi_ip = WiFi.localIP();
        USBSerial.print("WiFi Status: ");
        USBSerial.println(wifi_status);
        USBSerial.print("IP Adress: ");
        USBSerial.println(wifi_ip);
    }
    
    if(WiFi.status() != WL_CONNECTED){
        if(WiFi.status() == WL_NO_SHIELD){
            USBSerial.println("WiFi init failed, trying again..");
            WiFi.begin(ssid, pass);
        } else {
            USBSerial.println("Waiting for WiFi Connection..");
        }
    }

    wifi_connected = ((WiFi.localIP() != no_ip) && (WiFi.status() == WL_CONNECTED));

    if(ethernet_connected) client = &ethclient;
    else if(wifi_connected) client = &wificlient;
    else client = NULL;
    
    //connected = wifi_connected | ethernet_connected;
}


