#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>
#include <unistd.h>
#include <curl/curl.h>
#include <cmath> // Include for fabs()

using namespace std;

#define ADS1115_ADDR1 0x48
#define ADS1115_ADDR2 0x49
#define REFERENCE_VOLTAGE 4.096
#define MAX_ADC_VALUE 32768.0 // 2^15
#define SENSITIVITY 0.05 // Sensitivity in A/V, adjust based on your sensor's datasheet
#define NOISE_THRESHOLD 0.02 // Adjust this based on your sensor's noise level
#define NUM_SAMPLES 100
#define CURRENT_THRESHOLD 2.9//CURRENT VALUE MAX IS 2.9A

// Define LED GPIO pins
#define LED1_PIN 13
#define LED2_PIN 19
#define LED3_PIN 26

double sensor1_current;
double sensor2_current;
double sensor3_current;
double sensor4_current;
double sensor5_current;
double sensor6_current;

// JSON payload
   std::string constructJsonPayload(int node_id, int gateway, double sensor1_current, double sensor2_current, double sensor3_current, double sensor4_current, double sensor5_current, double sensor6_current) {
    std::string jsonPayload = 
        "{"
            "\"node_id\": \"" + std::to_string(node_id) + "\","
            "\"gateway\": " + std::to_string(gateway) + ","
            "\"dynamic_data\": {"
                "\"R3\": " + std::to_string(sensor1_current) + ","
                "\"Y3\": " + std::to_string(sensor2_current) + ","
                "\"B3\": " + std::to_string(sensor3_current) + ","
                "\"R4\": " + std::to_string(sensor4_current) + ","
                "\"Y4\": " + std::to_string(sensor5_current) + ","
                "\"B4\": " + std::to_string(sensor6_current) + 
            "}"
        "}";
    return jsonPayload;
}
void get_post(){
    // Initialize a curl session
    CURL *curl;
    CURLcode res;

   // Construct the JSON payload
    std::string jsonPayload = constructJsonPayload(8967, 12345, sensor1_current, sensor2_current, sensor3_current, sensor4_current, sensor5_current, sensor6_current);

     
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

class ADS1115 {
public:
    ADS1115(int address) {
        ads1115 = wiringPiI2CSetup(address);
        if (ads1115 == -1) {
            std::cerr << "Failed to initiate ADS1115 at address " << std::hex << address << std::endl;
            exit(1);
        }
    }

    int readADC(int channel) {
        int config = 0x4000 | (channel << 12); // Single-ended input, channel selection
        config |= 0x8000; // Start single-conversion
        config |= 0x0183; // 1600SPS, default settings

        wiringPiI2CWriteReg16(ads1115, 0x01, config);
        //usleep(8000); // Wait for conversion (1600SPS -> 625us per conversion)

        int adcValue = wiringPiI2CReadReg16(ads1115, 0x00);
        adcValue = ((adcValue << 8) & 0xFF00) | ((adcValue >> 8) & 0x00FF); // Swap bytes

        return adcValue;
    }

    double convertToVoltage(int adcValue) {
        return (adcValue * REFERENCE_VOLTAGE) / MAX_ADC_VALUE;
    }

    double convertToCurrent(double voltage) {
        return voltage * SENSITIVITY;
    }



private:
    int ads1115;
};

void setup() {
    wiringPiSetup();
}

double readAverageCurrent(ADS1115& adc, int channel, int numSamples = NUM_SAMPLES) {
    double totalCurrent = 0.0;
    double offsetVoltage = 0.0; // Add this to correct any offset

    // Initial reading to determine offset
    for (int i = 0; i < numSamples; ++i) {
        int rawValue = adc.readADC(channel);
        double voltage = adc.convertToVoltage(rawValue);
        offsetVoltage += voltage;
        usleep(100); // Short delay between samples
    }
    offsetVoltage /= numSamples;

    // Actual reading with offset correction
    for (int i = 0; i < numSamples; ++i) {
        int rawValue = adc.readADC(channel);
        double voltage = adc.convertToVoltage(rawValue) - offsetVoltage;
        double current = adc.convertToCurrent(voltage);

        if (fabs(current) < NOISE_THRESHOLD) {
            current = 0.0; // Set current to 0 if it's below the noise threshold
        }

        totalCurrent += current;
        usleep(100); // Short delay between samples
    }

    return totalCurrent / numSamples;
}

void readSensors(ADS1115& adc1, ADS1115& adc2) {
    // Reading from the first ADS1115
     sensor1_current = readAverageCurrent(adc1, 0);
     sensor2_current = readAverageCurrent(adc1, 1);
     sensor3_current = readAverageCurrent(adc1, 2);

    // Reading from the second ADS1115
     sensor4_current = readAverageCurrent(adc2, 0);
     sensor5_current = readAverageCurrent(adc2, 1);
     sensor6_current = readAverageCurrent(adc2, 2);

    // Output readings
    std::cout << "Sensor 1: " << sensor1_current << " A" << std::endl;
    std::cout << "Sensor 2: " << sensor2_current << " A" << std::endl;
    std::cout << "Sensor 3: " << sensor3_current << " A" << std::endl;
    std::cout << "Sensor 4: " << sensor4_current << " A" << std::endl;
    std::cout << "Sensor 5: " << sensor5_current << " A" << std::endl;
    std::cout << "Sensor 6: " << sensor6_current << " A" << std::endl;

    // Check current thresholds and turn on/off LEDs
    //pin 13
    if (sensor1_current > CURRENT_THRESHOLD || sensor4_current > CURRENT_THRESHOLD) {
        digitalWrite(LED1_PIN, HIGH);
        } else {
        digitalWrite(LED1_PIN, LOW);
    }

    // Check current thresholds and turn on/off LEDs
    //pin 19
    if (sensor2_current > CURRENT_THRESHOLD || sensor5_current > CURRENT_THRESHOLD) {
        digitalWrite(LED2_PIN, HIGH);
    } else {
        digitalWrite(LED2_PIN, LOW);
    }

    // Check current thresholds and turn on/off LEDs
    //pin 26
    if (sensor3_current > CURRENT_THRESHOLD || sensor6_current > CURRENT_THRESHOLD) {
        digitalWrite(LED3_PIN, HIGH);
    } else {
        digitalWrite(LED3_PIN, LOW);
    }

    //post to database
    get_post();
    std::cout <<endl;

    //usleep(1000000); // Simulate data reading interval
    delay(5000);
}

int main() {
    setup();
    ADS1115 adc1(ADS1115_ADDR1);
    ADS1115 adc2(ADS1115_ADDR2);

    while (true) {
        readSensors(adc1, adc2);
        //delay(5000); // Adjust the delay as needed
    }

    return 0;
}
