#include <iostream>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <json/json.h>
#include <regex>
#include "LoRa.h"
#include "WString.h"
#include <stdio.h>
#include <time.h>
#include <wiringPi.h>
#include "Print.h"
#include "itoa.h"
#include <fstream>
#include <thread>  
#include <chrono> 
using namespace std;
const int csPin = 10;          // LoRa radio chip select
const int resetPin = 3;       // LoRa radio reset
const int irqPin = 7;         // change for your board; must be a hardware interrupt pin

const int ledSendPin = 0;     // GPIO pin for LED indicating sending
const int ledReceivePin = 1;  // GPIO pin for LED indicating receiving

String outgoing;              // outgoing message

uint8_t msgCount = 0;            // count of outgoing messages
uint8_t localAddress = 0xF0;     // address of this device
uint8_t destination = 0xDC;      // destination to send to
long lastSendTime = 0;        // last send time
unsigned int interval = 2000;          // interval between sends

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
    digitalWrite(ledSendPin, HIGH);   // Turn on sending LED
    LoRa.beginPacket();                   // start packet
    LoRa.write(destination);              // add destination address
    LoRa.write(localAddress);             // add sender address
    LoRa.write(msgCount);                 // add message ID
    LoRa.write(outgoing.length());        // add payload length
    LoRa.print(outgoing);                 // add payload
    LoRa.endPacket();                     // finish packet and send it
    digitalWrite(ledSendPin, LOW);        // Turn off sending LED
    msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
    if (packetSize == 0) return;          // if there's no packet, return

    digitalWrite(ledReceivePin, HIGH);    // Turn on receiving LED

    // read packet header bytes:
    int recipient = LoRa.read();          // recipient address
    uint8_t sender = LoRa.read();            // sender address
    uint8_t incomingMsgId = LoRa.read();     // incoming msg ID
    uint8_t incomingLength = LoRa.read();    // incoming msg length

    String incoming = "";

    while (LoRa.available()) {
        incoming += (char)LoRa.read();
    }

    if (incomingLength != incoming.length()) {   // check length for error
        printf("error: message length does not match length\n");
        digitalWrite(ledReceivePin, LOW);        // Turn off receiving LED
        return;                             // skip rest of function
    }

    // if the recipient isn't this device or broadcast,
    if (recipient != localAddress && recipient != 0xFF) {
        printf("This message is not for me.\n");
        digitalWrite(ledReceivePin, LOW);        // Turn off receiving LED
        return;                             // skip rest of function
    }

    // if message is for this device, or broadcast, print details:
    std::cout << "Received from: 0x" <<  String(sender, HEX) << std::endl;
    std::cout << "Sent to: 0x" << String(recipient, HEX) << std::endl;
    std::cout << "Message ID: " << String(incomingMsgId) << std::endl;
    std::cout << "Message length: " << String(incomingLength) <<std::endl;
    std::cout << "Message: " << incoming << std::endl;
    std::cout << "RSSI: " <<  String(LoRa.packetRssi()) << std::endl;
    std::cout << "Snr: " << String(LoRa.packetSnr())<< std::endl;
    std::cout << std::endl;

    digitalWrite(ledReceivePin, LOW);        // Turn off receiving LED
}

// Function to extract values from the string
void extractDataFromOrderedDict(const std::string& orderedDictStr) {
    // Regular expression to match key-value pairs in the format (key, value)
    std::regex pairRegex("\\('([^']*)',\\s*([0-9.]+)\\)");
    std::smatch matches;
    std::string::const_iterator searchStart(orderedDictStr.cbegin());

    // Define variables to store the extracted values
    double R1, Y1 , B1 ;
    double R2, Y2 , B2 ;

    while (std::regex_search(searchStart, orderedDictStr.cend(), matches, pairRegex)) {
        std::string key = matches[1];
        double value = std::stod(matches[2]);
        //String value = value.c_str()
        //std::string value = matches[2];


        if (key == "R1") {
            R1 = value;
        } else if (key == "Y1") {
            Y1 = value;
        } else if (key == "B1") {
            B1 = value;
        } else if (key == "R2") {
            R2 = value;
        } else if (key == "Y2") {
            Y2 = value;
        } else if (key == "B2") {
            B2 = value;
        }

        searchStart = matches.suffix().first;
    }

    // Print the extracted values
    std::cout << "R1: " << R1 << std::endl;
    std::cout << "Y1: " << Y1 << std::endl;
    std::cout << "B1: " << B1 << std::endl;
    std::cout << "R2: " << R2 << std::endl;
    std::cout << "Y2: " << Y2 << std::endl;
    std::cout << "B2: " << B2 << std::endl;

    // Prepare the message to send via LoRa
   // outgoing = "R1:" + String(R3) + ",Y3:" + String(Y3) + ",B3:" + String(B3) + ",R4:" + String(R4) + ",Y4:" + String(Y4) + ",B4:" + String(B4);

    string msg = "R1" + to_string(R1) + "," + "Y1" + to_string(Y1) + "," + "B1" + to_string(B1) + "," 
               + "R2" + to_string(R2) + "," + "Y2" + to_string(Y2) + "," + "B2" + to_string(B2);
    //std::cout << "Sending " << msg << std::endl;
    outgoing = msg.c_str();
}

// Callback function to handle the response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

void retrieveDataFromServer() {
    // Initialize a cURL session
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Initialize cURL
    curl = curl_easy_init();
    if (curl) {
        // Set the URL for the GET request
        curl_easy_setopt(curl, CURLOPT_URL, "http://20.244.35.38/get_last_node_data/");

        // Set the write callback function to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request, res will get the return code
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else {
            // Output the response data
            std::cout << "Response data: " << readBuffer << std::endl;

            // Parse the JSON response
            Json::CharReaderBuilder readerBuilder;
            Json::Value root;
            std::string errs;

            std::istringstream s(readBuffer);
            if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
                std::cerr << "Error parsing JSON: " << errs << std::endl;
                return;
            }

            // Extract individual values
            int id = root["id"].asInt();
            std::string time = root["time"].asString();
            Json::Value counter = root["counter"];
            int nodeId = root["NodeId"].asInt();
            int datagateway = root["datagateway"].asInt();

            // Print the values
            std::cout << "ID: " << id << std::endl;
            std::cout << "Time: " << time << std::endl;
            std::cout << "Counter: " << (counter.isNull() ? "null" : counter.asString()) << std::endl;
            std::cout << "NodeId: " << nodeId << std::endl;
            std::cout << "Datagateway: " << datagateway << std::endl;

            // Extract dynamic_data directly as a JSON object
            Json::Value dynamicData = root["dynamic_data"];
            std::cout << "value of Data after extracting from database" << std::endl;
            std::string dynamicDataStr = root["dynamic_data"].asString();
            // Remove quotes around the string
            if (dynamicDataStr.front() == '"' && dynamicDataStr.back() == '"') {
                dynamicDataStr = dynamicDataStr.substr(1, dynamicDataStr.length() - 2);
            }
            // Extract values from the ordered dict
            extractDataFromOrderedDict(dynamicDataStr);
            // Iterate over the dynamic_data object and print key-value pairs
            for (Json::Value::const_iterator it = dynamicData.begin(); it != dynamicData.end(); ++it) {
                std::cout << it.key().asString() << ": " << (*it).asDouble() << std::endl;
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }
}

void setup() {
    printf("LoRa Duplex\n");

    wiringPiSetup();  // Initialize wiringPi
    setupLEDs();      // Setup LED pins

    // override the default CS, reset, and IRQ pins (optional)
    LoRa.setPins(csPin, resetPin, irqPin);   // set CS, reset, IRQ pin

    if (!LoRa.begin(433E6, 0)) {
        printf("Starting LoRa failed!\n");
        while (1);
    }
    printf("LoRa init succeeded.\n");

    // Seed the random number generator
    srand(time(NULL));
}

void loop() {
    if (millis() - lastSendTime > interval) {
        // Retrieve data from the server
        retrieveDataFromServer();

        // Send the data via LoRa
        sendMessage(outgoing);
        std::cout << "Sending " << outgoing << std::endl;

        lastSendTime = millis();            // Timestamp the message
        interval = (rand() % 2000) + 1000;  // 1-3 seconds
    }

    // Parse for a packet, and call onReceive with the result
    onReceive(LoRa.parsePacket());
}

int main(void) {
    setup();
    while(1) loop();
}
