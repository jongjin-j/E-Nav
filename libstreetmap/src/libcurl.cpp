#include <iostream>
#include <string.h>
#include "libcurl.h"
#include <curl/curl.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <string>

using namespace std;
using boost::property_tree::ptree;
using boost::property_tree::read_json;

typedef struct MyCustomStruct {
    char *url = NULL;
    unsigned int size = 0;
    char *response = NULL;
} MyCustomStruct;

/* buffer holds the data received from curl_easy_perform()
 * size is always 1
 * nmemb is the number of bytes in buffer
 * userp is a pointer to user data (i.e. myStruct from main)
 *
 * Should return same value as nmemb, else it will signal an error to libcurl
 * and curl_easy_perform() will return an error (CURLE_WRITE_ERROR). This is
 * useful if you want to signal an error has occured during processing.
 */
static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    cout << "In my own custom callback function" << endl;

    if (buffer && nmemb && userp) {
        MyCustomStruct *pMyStruct = (MyCustomStruct *)userp;

        // Reads from struct passed in from main
        cout << "Sucessfully queried page at URL: " << pMyStruct->url << endl;

        // Writes to struct passed in from main
        cout << "Storing received buffer into custom struct..." << endl;
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

void PrintWeatherInfo(ptree &ptRoot) {
    double temp = 0;
    double feels_like = 0;
    //double longitude = 0, latitude = 0;
    
    /*ptree &main = ptRoot.get_child("main");
    BOOST_FOREACH(auto &child, main){
        if("temp" == child.second.data()){
            temp = child.second.get<double>("temp");
        }
    }*/
    
    temp = ptRoot.get<double>("main.temp");
    cout << temp << endl;

    return;
}

bool loadCityWeatherData() {
    string cityName = "Toronto";
    
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        cout << "ERROR: Unable to initialize libcurl" << endl;
        cout << curl_easy_strerror(res) << endl;
        return 0;
    }

    CURL *curlHandle = curl_easy_init();
    if (!curlHandle ) {
        cout << "ERROR: Unable to get easy handle" << endl;
        return 0;
    } 
    else {
        char errbuf[CURL_ERROR_SIZE] = {0};
        //string cityName = "Toronto";
        char targetURL[] = "http://api.openweathermap.org/data/2.5/weather?q=Toronto&appid=69d3de6a38e4f0c9aa59c4235f670765";
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
            cout << "Current weather in " << cityName << ": " << endl;
            cout << "====================" << endl << endl;
            
            double temp = ptRoot.get<double>("main.temp") - 273.15;
            double feels_like = ptRoot.get<double>("main.feels_like") - 273.15;
            double pressure = ptRoot.get<double>("main.pressure");
            double humidity = ptRoot.get<double>("main.humidity");
  
            cout << temp << endl;
            cout << feels_like << endl;
            cout << pressure << endl;
            cout << humidity << endl;
            
            cout << endl << "====================" << endl;
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

    return 0;
}