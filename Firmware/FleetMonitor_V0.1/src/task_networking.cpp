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




extern USBCDC USBSerial;

#define TASK_NETWORKING_FREQ 500

// Ethernet Definitions
byte mac[] = { 0x2C, 0xF7, 0xF1, 0x08, 0x39, 0x7E };
String server = "http://jsonplaceholder.typicode.com/comments?id=10";
int port = 80;

HTTPClient client;

// Wifi Definitions
char ssid[] = "fleet-monitor";     //  your network SSID (name)
char pass[] = "password";  // your network password

bool ethernet_connected = false;
bool wifi_connected = false;
bool network_connected = false;

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

    static TickType_t wifi_disconnect_time = xTaskGetTickCount();
    
    /** Ethernet stuff **/
    IPAddress no_ip(0,0,0,0);

    // Diagnostics Print, only on change
    if(eth_harware_status != Ethernet.hardwareStatus() || eth_link_status != Ethernet.linkStatus() || eth_ip != Ethernet.localIP()){
        eth_harware_status = Ethernet.hardwareStatus();
        eth_link_status = Ethernet.linkStatus();
        eth_ip = Ethernet.localIP();
        USBSerial.print("[ETH] Hardware Status: ");
        USBSerial.println(eth_harware_status == EthernetW5500 ? "UP" : "DOWN");
        USBSerial.print("[ETH] Link Status: ");
        USBSerial.println(eth_link_status == LinkON ? "UP" : "DOWN");
        USBSerial.print("[ETH] IP Adress: ");
        USBSerial.println(eth_ip);

        if(Ethernet.hardwareStatus() != EthernetW5500){
            USBSerial.println("[ETH] No Hardware Found!");
        }

        if (Ethernet.localIP() == no_ip && Ethernet.linkStatus() == LinkON){
            USBSerial.println("[ETH] Waiting for IP..");
        }
    }

    // Check for Hardware connection
    if(Ethernet.hardwareStatus() != EthernetW5500){
        Ethernet.begin(mac, 1000, 1000);
    }
    // Check for Link Connection
    else if(Ethernet.linkStatus() != LinkON){
        Ethernet.setLocalIP(no_ip);
    }

    // Check both Link and IP
    if (Ethernet.localIP() == no_ip && Ethernet.linkStatus() == LinkON) {
        Ethernet.begin(mac, 1000, 1000);
    }  
    
    ethernet_connected = ((Ethernet.localIP() != no_ip) && (Ethernet.linkStatus() == LinkON));

    /** WiFi Stuff **/
    // Diagnostics Print, only on change
    if(wifi_status != WiFi.status() || wifi_ip != WiFi.localIP()){
        wifi_status = WiFi.status();
        wifi_ip = WiFi.localIP();
        USBSerial.print("[WiFi] Status: ");
        USBSerial.println(wifi_status);
        USBSerial.print("[WiFi] IP Adress: ");
        USBSerial.println(wifi_ip);

        if(WiFi.status() == WL_NO_SHIELD){
            USBSerial.println("[WiFi] Init failed, trying again..");
        }
        else if(WiFi.status() == WL_DISCONNECTED){
            USBSerial.println("[WiFi] No connection");
        } else if(WiFi.status() != WL_CONNECTED){
            USBSerial.println("[WiFi] Waiting for Connection..");
        }
    }
    
    if(WiFi.status() != WL_CONNECTED){
        if(WiFi.status() == WL_NO_SHIELD){
            WiFi.begin(ssid, pass);
        }
        else{
            if((wifi_disconnect_time + 60000) < xTaskGetTickCount()){
                WiFi.begin(ssid, pass);
                wifi_disconnect_time = xTaskGetTickCount();
            }
        }
    }

    wifi_connected = ((WiFi.localIP() != no_ip) && (WiFi.status() == WL_CONNECTED));

    if(ethernet_connected) client.begin(ethclient, server);
    else if(wifi_connected) client.begin(ethclient, server); // TODO change to wifi / add port

    network_connected = wifi_connected | ethernet_connected;
    
}


