//@Biswajit
#include "LoRa.h"
#include "WString.h"
#include <stdio.h>
#include <time.h>

#include <wiringPi.h>
#include "Print.h"
#include "itoa.h"
#include <iostream>
#include <fstream>

#include <string>
#include <curl/curl.h>
#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::seconds

const int csPin = 10;          // LoRa radio chip select
const int resetPin = 3;       // LoRa radio reset
const int irqPin = 7;         // change for your board; must be a hardware interrupt pin

const int ledSendPin = 0;     
const int ledReceivePin = 1;  

String outgoing;            

uint8_t msgCount = 0;           
uint8_t localAddress = 0x34;     // address of this device
uint8_t destination = 0xDC;      // destination to send to
long lastSendTime = 0;        
unsigned int interval = 2000;          
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JSON payload
/*std::string constructJsonPayload(int node_id, int gateway, String r1, String y1, String b1, String r2, String y2, String b2) {
    std::string jsonPayload =
        "{"
            "\"node_id\": \"" + std::to_string(node_id) + "\","
            "\"gateway\": " + std::to_string(gateway) + ","
            "\"dynamic_data\": {"
                "\"R1\": \"" + std::string(r1.c_str()) + "\","
                "\"Y1\": \"" + std::string(y1.c_str()) + "\","
                "\"B1\": \"" + std::string(b1.c_str()) + "\","
                "\"R2\": \"" + std::string(r2.c_str()) + "\","
                "\"Y2\": \"" + std::string(y2.c_str()) + "\","
                "\"B2\": \"" + std::string(b2.c_str()) + "\""
            "}"
        "}";
    return jsonPayload;
}*/

std::string constructJsonPayload(int node_id, int gateway, String r1, String y1, String b1, String r2, String y2, String b2) 
   {
    std::string jsonPayload = 
        "{"
            "\"node_id\": \"" + std::to_string(node_id) + "\","
            "\"gateway\": " + std::to_string(gateway) + ","
            "\"dynamic_data\": {"
                "\"R1\": " + std::string(r1.c_str()) + ","
                "\"Y1\": " + std::string(y1.c_str()) + ","
                "\"B1\": " + std::string(b1.c_str()) + ","
                "\"R2\": " + std::string(r2.c_str()) + ","
                "\"Y2\": " + std::string(y2.c_str()) + ","
                "\"B2\": " + std::string(b2.c_str()) + 
            "}"
        "}";
    return jsonPayload;
}

void get_post(const std::string& jsonPayload) {
    // Initialize a curl session
    CURL *curl;
    CURLcode res;

    // Initialize curl
    curl = curl_easy_init();
    if(curl) {
        // Set the URL for the POST request
        curl_easy_setopt(curl, CURLOPT_URL, "http://20.244.35.38/node_data_create/");

        // Set the request type to POST
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Set the JSON payload
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPayload.c_str());

        // Set the content type header
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        //headers = curl_slist_append(headers, "Cookie: csrftoken=TAsBfO6iu9s87Am5qtj33q5qNK6tplni");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);

        // Check for errors
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setupLEDs() {
    pinMode(ledSendPin, OUTPUT);
    pinMode(ledReceivePin, OUTPUT);
    digitalWrite(ledSendPin, LOW);
    digitalWrite(ledReceivePin, LOW);
}

void blinkLED(int pin) {
    digitalWrite(pin, HIGH);
    delay(100);  // LED on for 100 ms
    digitalWrite(pin, LOW);
}

void sendMessage(String outgoing) {
  digitalWrite(ledSendPin, HIGH);
  LoRa.beginPacket();                   
  LoRa.write(destination);              
  LoRa.write(localAddress);             
  LoRa.write(msgCount);                 
  LoRa.write(outgoing.length());        
  LoRa.print(outgoing);                 
  LoRa.endPacket();                     
  digitalWrite(ledSendPin, LOW);
  msgCount++;                           
}


void onReceive(int packetSize) {
  if (packetSize == 0) return;         
  digitalWrite(ledReceivePin, HIGH);
  // read packet header bytes:
  int recipient = LoRa.read();          
  uint8_t sender = LoRa.read();            
  uint8_t incomingMsgId = LoRa.read();     
  uint8_t incomingLength = LoRa.read();    

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
      printf("error: message length does not match length\n");
      digitalWrite(ledReceivePin, LOW);
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0x34) {
      printf("This message is not for me.\n");
      digitalWrite(ledReceivePin, LOW);
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  cout << "Received from: 0x" <<  String(sender, HEX) << endl;
  String devID = String(sender, HEX);
  cout << "Sent to: 0x" << String(recipient, HEX) << endl;
  cout << "Message ID: " << String(incomingMsgId) << endl;
  cout << "Message length: " << String(incomingLength) <<endl;
  cout << "Message: " << incoming << endl;
  cout << "RSSI: " <<  String(LoRa.packetRssi()) << endl;
  cout << "Snr: " << String(LoRa.packetSnr())<< endl;
  std::cout << endl;
  digitalWrite(ledReceivePin, LOW);
  delay(100);
  //////////////////////////////////////////////////////////////////////////////////////////////////
  if(devID == "3d"){
  int idStart = incoming.indexOf("R1") + 2;
  int idEnd = incoming.indexOf(",", idStart);
  String data = incoming.substring(idStart, idEnd);
  String r_1 = data;

  int idStart1 = incoming.indexOf("R2") + 2;
  int idEnd1 = incoming.indexOf(",", idStart1);
  String data1 = incoming.substring(idStart1, idEnd1);
  String r_2 = data1;

  int idStart2 = incoming.indexOf("Y1") + 2;
  int idEnd2 = incoming.indexOf(",", idStart2);
  String data2 = incoming.substring(idStart2, idEnd2);
  String y_1 = data2;

  int idStart3 = incoming.indexOf("Y2") + 2;
  int idEnd3 = incoming.indexOf(",", idStart3);
  String data3 = incoming.substring(idStart3, idEnd3);
  String y_2 = data3;

  int idStart4 = incoming.indexOf("B1") + 2;
  int idEnd4 = incoming.indexOf(",", idStart4);
  String data4 = incoming.substring(idStart4, idEnd4);
  String b_1 = data4;

  int idStart5 = incoming.indexOf("B2") + 2;
  int idEnd5 = incoming.indexOf(",", idStart5);
  String data5 = incoming.substring(idStart5, idEnd5);
  String b_2 = data5;
  //std::string jsonPayload = constructJsonPayload(89, 123, r_1, y_1, b_1, r_2, y_2, b_2);
  get_post(constructJsonPayload(89, 123, r_1, y_1, b_1, r_2, y_2, b_2));
  }
//////////////////////////////////////////////////////////////////////////////////////////////////
  else if(devID == "b4"){
  int idStart0 = incoming.indexOf("R1") + 2;
  int idEnd0 = incoming.indexOf(",", idStart0);
  String data0 = incoming.substring(idStart0, idEnd0);
  String r_11 = data0;

  int idStart11 = incoming.indexOf("R2") + 2;
  int idEnd11 = incoming.indexOf(",", idStart11);
  String data11 = incoming.substring(idStart11, idEnd11);
  String r_22 = data11;

  int idStart22 = incoming.indexOf("Y1") + 2;
  int idEnd22 = incoming.indexOf(",", idStart22);
  String data22 = incoming.substring(idStart22, idEnd22);
  String y_11 = data22;

  int idStart33 = incoming.indexOf("Y2") + 2;
  int idEnd33 = incoming.indexOf(",", idStart33);
  String data33 = incoming.substring(idStart33, idEnd33);
  String y_22 = data33;

  int idStart44 = incoming.indexOf("B1") + 2;
  int idEnd44 = incoming.indexOf(",", idStart44);
  String data44 = incoming.substring(idStart44, idEnd44);
  String b_11 = data44;

  int idStart55 = incoming.indexOf("B2") + 2;
  int idEnd55 = incoming.indexOf(",", idStart55);
  String data55 = incoming.substring(idStart55, idEnd55);
  String b_22 = data55;
  //std::string jsonPayload = constructJsonPayload(89, 123, r_11, y_11, b_11, r_22, y_22, b_22);
  get_post(constructJsonPayload(89, 123, r_11, y_11, b_11, r_22, y_22, b_22));
  }
 /////////////////////////////////////////////////////////////////////////////////////////////////
 else {
    printf("Unknown device ID.\n");
  }

}


void setup() {

   printf("LoRa Duplex\n");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);   // set CS, reset, IRQ pin

  if (!LoRa.begin(433E6, 0)) {
    printf("Starting LoRa failed!\n");
    while (1);
  }
  printf("LoRa init succeeded.\n");
 
  /* seed the randaom */
  srand (time(NULL));
}

void loop() {
  if (millis() - lastSendTime > interval) {
    String message = "from id 0xc0";   // send a message
    sendMessage(message);
    cout << "Sending " << message << endl;
    lastSendTime = millis();            // timestamp the message
    interval = (rand() % 2000) + 1000;    // 1-3 seconds
  }

  // parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());

}


int  main(void) {
   setup();
   while(1) loop();
}
