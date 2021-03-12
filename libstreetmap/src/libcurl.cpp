#include <iostream>
#include <string.h>
#include "libcurl.h"
#include <curl/curl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <vector>
#include <cstring>

using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::read_json;
std::vector<double> weatherData;
std::string currentWeather;

typedef struct MyCustomStruct {
    char *url = NULL;
    unsigned int size = 0;
    char *response = NULL;
} MyCustomStruct;

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    cout << "In my own custom callback function" << endl;

    if (buffer && nmemb && userp) {
        MyCustomStruct *pMyStruct = (MyCustomStruct *)userp;

        // Reads from struct passed in from main
        cout << "Successfully queried page at URL: " << pMyStruct->url << endl;

        if (pMyStruct->response == NULL) {
            // Case when first time write_data() is invoked
            pMyStruct->response = new char[nmemb + 1];
            strncpy(pMyStruct->response, (char *)buffer, nmemb);
        }
        else {
            // Case when second or subsequent time write_data() is invoked
            char *oldResp = pMyStruct->response;

            pMyStruct->response = new char[pMyStruct->size + nmemb + 1];

            // Copy old data
            strncpy(pMyStruct->response, oldResp, pMyStruct->size);

            // Append new data
            strncpy(pMyStruct->response + pMyStruct->size, (char *)buffer, nmemb);

            delete []oldResp;
        }

        pMyStruct->size += nmemb;
        pMyStruct->response[pMyStruct->size] = '\0';
    }

    return nmemb;
}

void loadCityWeatherData() {
    string cityName = "Toronto";
    
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        cout << "ERROR: Unable to initialize libcurl" << endl;
        cout << curl_easy_strerror(res) << endl;
    }

    CURL *curlHandle = curl_easy_init();
    if (!curlHandle ) {
        cout << "ERROR: Unable to get easy handle" << endl;
    } 
    else {
        char errbuf[CURL_ERROR_SIZE] = {0};
        
        std::string firstPart = "http://api.openweathermap.org/data/2.5/weather?q=";
        std::string secondPart = "&appid=69d3de6a38e4f0c9aa59c4235f670765";
        std::string finalURL = firstPart + cityName + secondPart;
        
        char targetURL[finalURL.length()];
        for(int i = 0; i < finalURL.length() + 1; i++){
            targetURL[i] = finalURL[i];
        }
        
        //char targetURL[] = "http://api.openweathermap.org/data/2.5/weather?q=Toronto&appid=69d3de6a38e4f0c9aa59c4235f670765";
        
        MyCustomStruct myStruct;

        res = curl_easy_setopt(curlHandle, CURLOPT_URL, targetURL);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, errbuf);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, write_data);
        if (res == CURLE_OK)
            res = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &myStruct);

        myStruct.url = targetURL;

        if (res != CURLE_OK) {
            cout << "ERROR: Unable to set libcurl option" << endl;
            cout << curl_easy_strerror(res) << endl;
        } else {
            res = curl_easy_perform(curlHandle);
        }

        cout << endl << endl;
        if (res == CURLE_OK) {
            // Create an empty proper tree
            ptree ptRoot;
            
            istringstream issJsonData(myStruct.response);
            boost::property_tree::read_json(issJsonData, ptRoot);

            // Parsing and printing the data
            cout << "Loading Weather Data in  " << cityName << ": " << endl;
            
            double temp = ptRoot.get<double>("main.temp") - 273.15;
            double feels_like = ptRoot.get<double>("main.feels_like") - 273.15;
            int pressure = ptRoot.get<int>("main.pressure");
            int humidity = ptRoot.get<int>("main.humidity");
            double windSpeed = ptRoot.get<double>("wind.speed");
            int windDegree = ptRoot.get<double>("wind.deg");
            
            weatherData.push_back(temp);
            weatherData.push_back(feels_like);
            weatherData.push_back(pressure);
            weatherData.push_back(humidity);
            weatherData.push_back(windSpeed);
            weatherData.push_back(windDegree);
            
            //currentWeather = ptRoot.get<string>("weather.main");
            
        }
        else {
            cout << "ERROR: res == " << res << endl;
            cout << errbuf << endl;
        }

        if (myStruct.response)
            delete []myStruct.response;

        curl_easy_cleanup(curlHandle);
        curlHandle = NULL;
    }

    curl_global_cleanup();
}