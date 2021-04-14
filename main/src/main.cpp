/* 
 * Copyright 2021 University of Toronto
 *
 * Permission is hereby granted, to use this software and associated 
 * documentation files (the "Software") in course work at the University 
 * of Toronto, or for personal use. Other uses are prohibited, in 
 * particular the distribution of the Software either publicly or to third 
 * parties.
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <iostream>
#include <string>
#include "StreetsDatabaseAPI.h"
#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "m4.h"
#include "OSMDatabaseAPI.h"
#include "libcurl.h"
#include "globals.h"

//Program exit codes
constexpr int SUCCESS_EXIT_CODE = 0;        //Everyting went OK
constexpr int ERROR_EXIT_CODE = 1;          //An error occured
constexpr int BAD_ARGUMENTS_EXIT_CODE = 2;  //Invalid command-line usage

//The default map to load if none is specified
std::string default_map_path = "/cad2/ece297s/public/maps/toronto_canada.streets.bin";
std::string map_name = "/cad2/ece297s/public/maps/toronto_canada.osm.bin";


// The start routine of your program (main) when you are running your standalone
// mapper program. This main routine is *never called* when you are running 
// ece297exercise (the unit tests) -- those tests have their own main routine
// and directly call your functions in /libstreetmap/src/ to test them.
// Don't write any code in this file that you want run by ece297exerise -- it 
// will not be called!
int main(int argc, char** argv) {

    std::string map_path;
    if(argc == 1) {
        //Use a default map
        map_path = default_map_path;
    } else if (argc == 2) {
        //Get the map from the command line
        map_path = argv[1];
    } else {
        //Invalid arguments
        std::cerr << "Usage: " << argv[0] << " [map_file_path]\n";
        std::cerr << "  If no map_file_path is provided a default map is loaded.\n";
        return BAD_ARGUMENTS_EXIT_CODE;
    }

    //Load the map and related data structures
    /*
    bool load_OSM_success = loadOSMDatabaseBIN(map_name);
    if(!load_OSM_success) {
        std::cerr << "Failed to load OSM map '" << map_name << "'\n";
        return ERROR_EXIT_CODE;
    }
    */
    bool load_success = loadMap(map_path);
    if(!load_success) {
        std::cerr << "Failed to load map '" << map_path << "'\n";
        return ERROR_EXIT_CODE;
    }
    
    //create a list of files in map directory
    createFileList("/cad2/ece297s/public/maps");
    
    loadCityWeatherData(map_path);

    std::cout << "Successfully loaded map '" << map_path << "'\n";
    std::cout << "Successfully loaded OSM map '" << map_name << "'\n";

    //You can now do something with the map data
    
    std::vector<StreetSegmentIdx> path = findPathBetweenIntersections(1, 1041, 15);
    /*std::cout << path.size() << std::endl;
    for (int i = 0; i< path.size(); i++){
        std::cout << path[i] << std::endl; 
    }
    std::cout << "total time: " << computePathTravelTime(path, 15) << std::endl;
    
    StreetSegmentInfo segment = getStreetSegmentInfo(29);
    std::string name = getStreetName(segment.streetID);
    std::cout << name << std::endl;*/
    
    std::vector<DeliveryInf> deliveries;
    std::vector<IntersectionIdx> depots;
    std::vector<CourierSubPath> result_path;
    float turn_penalty;
    
    deliveries = {DeliveryInf(122474, 97031), DeliveryInf(71346, 53400), DeliveryInf(50160, 73642), DeliveryInf(122474, 53400), DeliveryInf(52922, 99578), DeliveryInf(122474, 53400), DeliveryInf(122474, 20263), DeliveryInf(71346, 73642), DeliveryInf(119721, 73642)};
    depots = {11072, 5162, 100304};
    turn_penalty = 15.000000000;
    result_path = travelingCourier(deliveries, depots, turn_penalty);
    
    /*deliveries = {DeliveryInf(23975, 52829), DeliveryInf(28343, 20239), DeliveryInf(100949, 118269)};
    depots = {11317, 1263, 141824};
    turn_penalty = 15.000000000;
    result_path = travelingCourier(deliveries, depots, turn_penalty);
    
    std::vector<CourierSubPath> k = travelingCourier(deliveries, depots, 15);
    */
    for(int i = 0; i < result_path.size(); i++){
        std::cout << result_path[i].start_intersection << std::endl;
        //std::cout << k[i].end_intersection << std::endl;
    }
    
    //Clean-up the map data and related data structures
    std::cout << "Closing map\n";
 
    drawMap();
    closeMap(); 
    //closeOSMDatabase();

    return SUCCESS_EXIT_CODE;
}
