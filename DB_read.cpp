#include <iostream>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <json/json.h>
#include <regex>

// Function to extract values from the string
void extractDataFromOrderedDict(const std::string& orderedDictStr) {
  // Regular expression to match key-value pairs in the format (key, value)
  std::regex pairRegex("\\('([^']*)',\\s*([0-9.]+)\\)");
  std::smatch matches;
  std::string::const_iterator searchStart(orderedDictStr.cbegin());

  // Define variables to store the extracted values
  double R1 = 0.0, Y1 = 0.0, B1 = 0.0;
  double R2 = 0.0, Y2 = 0.0, B2 = 0.0;

  while (std::regex_search(searchStart, orderedDictStr.cend(), matches,
                           pairRegex)) {
    std::string key = matches[1];
    double value = std::stod(matches[2]);

    // Store the value based on the key
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
}

// Callback function to handle the response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

int main() {
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
                return 1;
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
            dynamicDataStr =
                dynamicDataStr.substr(1, dynamicDataStr.length() - 2);
          }
          // Extract values from the ordered dict
          extractDataFromOrderedDict(dynamicDataStr);
            // Iterate over the dynamic_data object and print key-value pairs
            for (Json::Value::const_iterator it = dynamicData.begin(); it != dynamicData.end(); ++it) 
            {
                std::cout << it.key().asString() << ": " << (*it).asDouble() << std::endl;
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }

    return 0;
}
