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
#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include <string.h>
#include <math.h>
#include <algorithm>
#include <locale> 
#include <unordered_set>
#include <cctype>
#include <map>
#include "globals.h"

// loadMap will be called with the name of the file that stores the "layer-2"
// map data accessed through StreetsDatabaseAPI: the street and intersection 
// data that is higher-level than the raw OSM data). 
// This file name will always end in ".streets.bin" and you 
// can call loadStreetsDatabaseBIN with this filename to initialize the
// layer 2 (StreetsDatabase) API.
// If you need data from the lower level, layer 1, API that provides raw OSM
// data (nodes, ways, etc.) you will also need to initialize the layer 1 
// OSMDatabaseAPI by calling loadOSMDatabaseBIN. That function needs the 
// name of the ".osm.bin" file that matches your map -- just change 
// ".streets" to ".osm" in the map_streets_database_filename to get the proper
// name.


//declare database
databases database;

float avg_lat;
double max_lat;
double min_lat; 
double max_lon; 
double min_lon; 


//implementing conversion functions
double x_from_lon(double lon) {
    double x = kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat) * (lon);
    return x;
}

double y_from_lat(double lat) {
    double y = kEarthRadiusInMeters * kDegreeToRadian * lat;
    return y;
}

double lon_from_x(double x) {
    double lon = x / (kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat));
    return lon;
}

double lat_from_y(double y) {
    double lat = y / (kEarthRadiusInMeters * kDegreeToRadian);
    return lat;
}

//helper functions that create databases
//create a MultiMap of simplified street names and their indexes
void simplifiedStreetNames_streetIdx (){
    for(int i = 0; i < getNumStreets(); i++){
        std::string streetName = getStreetName(i);
        
        //remove blank spaces and change to lowercase 
        streetName.erase(std::remove(streetName.begin(), streetName.end(), ' '), streetName.end()); //code snippet from https://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
        std::transform(streetName.begin(), streetName.end(), streetName.begin(), ::tolower); // code snippet from https://www.geeksforgeeks.org/conversion-whole-string-uppercase-lowercase-using-stl-c/
        (database.streetName_and_streetID).insert(std::make_pair(streetName,i)); 
    }    
}

//create a 2D vector to store corresponding street segment IDs for each intersection 
void intersections_streetSegments (){
    (database.intersection_street_segments).resize(getNumIntersections());

    for (int intersection = 0; intersection < getNumIntersections(); ++intersection) {
        for (int i = 0; i < getNumIntersectionStreetSegment(intersection); ++i) {
            int ss_id = getIntersectionStreetSegment(intersection, i);
            database.intersection_street_segments[intersection].push_back(ss_id);
        }
    }
}

//create a 2D vector to store corresponding street segment IDs for each street 
void streetID_streetSegments(){
    (database.streetID_street_segments).resize(getNumStreets());

    for (int i = 0; i < getNumStreetSegments(); i++) {
        StreetSegmentInfo temp_segment = getStreetSegmentInfo(i);
        int temp_street_id = temp_segment.streetID;
        (database.streetID_street_segments)[temp_street_id].push_back(i);
    }
}

//create a 2D vector to store corresponding intersection IDs for each street 
void streetID_Intersections(){
    (database.streetID_intersections).resize(getNumStreets());

    for (int i = 0; i < getNumStreets(); i++) {
        for (auto it = database.streetID_street_segments[i].begin(); it != database.streetID_street_segments[i].end(); it++) {
            StreetSegmentIdx ss_idx = *it;
            StreetSegmentInfo ss_info = getStreetSegmentInfo(ss_idx);
            database.streetID_intersections[i].push_back(ss_info.from);
            database.streetID_intersections[i].push_back(ss_info.to);
        }

        //erase duplicates
        std::unordered_set<int> s;
        for (auto j : database.streetID_intersections[i]) {
            s.insert(j);
        }
        database.streetID_intersections[i].assign(s.begin(), s.end());

        std::copy(s.begin(), s.end(), database.streetID_intersections[i].begin());
    }
}

//create a vector to store the time taken to travel each street segment  
void streetSegment_travelTime(){
    for (int streetSegNum = 0; streetSegNum < getNumStreetSegments(); streetSegNum++) {
        double totalStreetSegmentLength = 0, firstSegmentLength = 0, lastSegmentLength = 0, curveSegmentLength = 0;
        StreetSegmentInfo street_segment = getStreetSegmentInfo(streetSegNum);

        if (street_segment.numCurvePoints == 0) {
            LatLon posFrom = getIntersectionPosition(street_segment.from);
            LatLon posTo = getIntersectionPosition(street_segment.to);
            std::pair <LatLon, LatLon> segment(posFrom, posTo);
            totalStreetSegmentLength = findDistanceBetweenTwoPoints(segment);
        }

        else {
            int curvePoints = street_segment.numCurvePoints;
            
            //create a dynamic array to store the curve points in the street segment
            LatLon *segmentCurvePoints = new LatLon [curvePoints];
            for (int i = 0; i < curvePoints; i++) {
                segmentCurvePoints[i] = getStreetSegmentCurvePoint(streetSegNum, i);
            }

            //compute distance between from point and first curvePoint(0)
            LatLon posFrom = getIntersectionPosition(street_segment.from);
            std::pair <LatLon, LatLon> firstSegment(posFrom, segmentCurvePoints[0]);
            firstSegmentLength = findDistanceBetweenTwoPoints(firstSegment);

            //compute distance between last curvePoint and to point(numCurvePoints - 1)
            LatLon posTo = getIntersectionPosition(street_segment.to);
            std::pair <LatLon, LatLon> lastSegment(segmentCurvePoints[street_segment.numCurvePoints - 1], posTo);
            lastSegmentLength = findDistanceBetweenTwoPoints(lastSegment);

            //compute distance of each street segment excluding the first and the last street segment
            for (int i = 0; i < curvePoints - 1; i++) {
                std::pair<LatLon, LatLon> tempSegment(segmentCurvePoints[i], segmentCurvePoints[i + 1]);
                curveSegmentLength += findDistanceBetweenTwoPoints(tempSegment);
            }
            totalStreetSegmentLength = firstSegmentLength + lastSegmentLength + curveSegmentLength;
            delete[] segmentCurvePoints;
        }
        double speed = street_segment.speedLimit;
        database.street_segment_travelTime.push_back(totalStreetSegmentLength / speed);
    }
}

void set_MaxMinLatLon_avgLat(){
    
    max_lat = getIntersectionPosition(0).latitude();
    min_lat = max_lat;
    max_lon = getIntersectionPosition(0).longitude();
    min_lon = max_lon;
    
    
    for (int i = 0; i < getNumIntersections(); i++) {
        //database.intersections[i].name = getIntersectionName(i);

        max_lat = std::max(max_lat, getIntersectionPosition(i).latitude());
        min_lat = std::min(min_lat, getIntersectionPosition(i).latitude());
        max_lon = std::max(max_lon, getIntersectionPosition(i).longitude());
        min_lon = std::min(min_lon, getIntersectionPosition(i).longitude());
    }

    //average lat for Cartesian transformation
    avg_lat = (min_lat + max_lat) / 2;
}

void intersections_database(){
    //set intersections database
    database.intersections.resize(getNumIntersections());
    
    //change intersection points to Cartesian coordinates
    for (int i = 0; i < getNumIntersections(); i++) {
        database.intersections[i].name = getIntersectionName(i);
        database.intersections[i].x = x_from_lon(getIntersectionPosition(i).longitude());
        database.intersections[i].y = y_from_lat(getIntersectionPosition(i).latitude());
    }
}

//set POI database
void POI_database(){
    database.POIs.resize(getNumPointsOfInterest());

    for (int i = 0; i < getNumPointsOfInterest(); i++) {
        database.POIs[i].name = getPOIName(i);
        database.POIs[i].x = x_from_lon(getPOIPosition(i).longitude());
        database.POIs[i].y = y_from_lat(getPOIPosition(i).latitude());
        database.POIs[i].id = getPOIOSMNodeID(i);
    }
}

//set streets database
void streets_database(){
    database.streets.resize(getNumStreetSegments());

    for (int i = 0; i < getNumStreetSegments(); i++) {

        //for each street segment, obtain its intersection IDs "from" and "to"
        //obtain each intersection ID's position via calling getIntersectionPosition (type LatLon)
        LatLon startingSeg = LatLon(getIntersectionPosition(getStreetSegmentInfo(i).from).latitude(), getIntersectionPosition(getStreetSegmentInfo(i).from).longitude());
        LatLon endingSeg = LatLon(getIntersectionPosition(getStreetSegmentInfo(i).to).latitude(), getIntersectionPosition(getStreetSegmentInfo(i).to).longitude());

        //convert LatLon into Cartesian coord and draw line for each segment
        database.streets[i].start_x = x_from_lon(startingSeg.longitude());
        database.streets[i].start_y = y_from_lat(startingSeg.latitude());
        database.streets[i].end_x = x_from_lon(endingSeg.longitude());
        database.streets[i].end_y = y_from_lat(endingSeg.latitude());
        database.streets[i].mid_x = 0.5 * (database.streets[i].start_x + database.streets[i].end_x);
        database.streets[i].mid_y = 0.5 * (database.streets[i].start_y + database.streets[i].end_y);

        
        double start_x = database.streets[i].start_x, start_y = database.streets[i].start_y;
        double end_x = database.streets[i].end_x, end_y = database.streets[i].end_y;
        
        //set rotation of names
        double rotation = 0;

        //if the x coordinates of start and end are equal
        if (end_x == start_x) {
            rotation = 90;
            
            //if end y and start y are in reverse
            if (end_y < start_y){
                database.streets[i].reverse = true;
            }
        } 
        else {
            rotation = std::atan(abs((end_y - start_y) / (end_x - start_x))) / kDegreeToRadian;
        }
        
        //setting the correct rotations of the names and figuring out the directions of the ways
        //when from is seen as the origin and to is in first quadrant 
        if (end_x > start_x && end_y > start_y) {
            database.streets[i].angle = rotation;
        } 
        
        //to is in second quadrant
        if (end_x < start_x && end_y > start_y){
            database.streets[i].angle = -1 * rotation;
            database.streets[i].reverse = true;
        }
        
        //to is in third quadrant
        if (end_x < start_x && end_y < start_y){
            database.streets[i].angle = rotation;
            database.streets[i].reverse = true;
        }
        
        //to is in fourth quadrant
        if (end_x > start_x && end_y < start_y){
            database.streets[i].angle = -1 * rotation;
        }
        
        //to is on positive x axis
        if (end_x > start_x && end_y == start_y){
            database.streets[i].angle = rotation;
        }
        
        //to is on negative x axis
        if (end_x < start_x && end_y == start_y){
            database.streets[i].angle = rotation;
            database.streets[i].reverse = true;
        }
        
        //set name of the street
        database.streets[i].name = getStreetName(getStreetSegmentInfo(i).streetID);
        
        //set one way boolean
        database.streets[i].oneWay = getStreetSegmentInfo(i).oneWay;
    }
}

//creating an unordered map for OSMID and way pointer   
void OSMID_wayValue(){
    for (int i = 0; i < getNumberOfWays(); i++){
            
        //get the way pointer and OSMID
        const OSMWay* OSMWay_ptr = getWayByIndex (i);
        OSMID WayID = OSMWay_ptr->id();
        
        std::string key, value;

        //loop through the tags and push into unordered map when key is highway
        for (int j = 0; j < getTagCount(OSMWay_ptr); j++) {
            
            std::tie(key, value) = getTagPair(OSMWay_ptr, j);
            
            if (key == "highway" || key == "railway")
                database.OSMID_wayType[WayID] = value;
        }
    }
}

void OSMID_nodeValue(){
    for (int i = 0; i < getNumberOfNodes(); i++){
            
        //get the node pointer and OSMID
        const OSMNode* OSMNode_ptr = getNodeByIndex(i);
        OSMID NodeID = OSMNode_ptr->id();
        
        std::string key, value;

        //loop through the tags and push into unordered map when key is amenity or shop
        for (int j = 0; j < getTagCount(OSMNode_ptr); j++) {
            
            std::tie(key, value) = getTagPair(OSMNode_ptr, j);
            
            if(key == "amenity" || key == "aeroway" || key == "shop"){
                database.OSMID_nodeType[NodeID] = value;
            }
        }
    }
}


bool loadMap(std::string map_streets_database_filename) {
    bool load_successful = false; //Indicates whether the map has loaded 
    //successfully

    std::cout << "loadMap: " << map_streets_database_filename << std::endl;

    // Load your map related data structures here.

    load_successful = loadStreetsDatabaseBIN(map_streets_database_filename);
    
    
    
    if(load_successful == false){
        return false;
    }
    
    //declare database
    set_MaxMinLatLon_avgLat();
    
    //load databases
    simplifiedStreetNames_streetIdx();
    intersections_streetSegments();
    streetID_streetSegments();
    streetID_Intersections();
    streetSegment_travelTime();
    intersections_database();
    POI_database();
    streets_database();
    OSMID_wayValue();
    OSMID_nodeValue();
    
  
    load_successful = true; //Make sure this is updated to reflect whether
    //loading the map succeeded or failed

    return load_successful;
}

void closeMap() {

    //Delete the three vectors created to free memory
    std::vector<std::vector < StreetSegmentIdx >> ().swap(database.intersection_street_segments);
    std::vector<std::vector < StreetSegmentIdx >> ().swap(database.streetID_street_segments);
    std::vector<std::vector < StreetIdx >> ().swap(database.streetID_intersections);
    std::vector<double>().swap(database.street_segment_travelTime);
    std::multimap<std::string, StreetIdx> ().swap(database.streetName_and_streetID);

    closeStreetDatabase();
}

double findDistanceBetweenTwoPoints(std::pair<LatLon, LatLon> points) {
    //compute difference between x1,x2 and y1,y2 respectively, returns the sqrt of the sum of the squares
    //note xCoord is computed via kEarthRadiusInMeters * kDegreeToRadian * cos(latitudeAverage)
    
    double xDiff = kEarthRadiusInMeters * kDegreeToRadian * cos(0.5 * kDegreeToRadian * (points.first.latitude() + points.second.latitude())) * (points.second.longitude() - points.first.longitude());
    double yDiff = kEarthRadiusInMeters * kDegreeToRadian * (points.second.latitude() - points.first.latitude());
    return sqrt(xDiff * xDiff + yDiff * yDiff);
}

double findStreetSegmentLength(StreetSegmentIdx street_segment_id) {
    //obtain time taken to travel the segment via the data structure created in loadMap
    double segmentTravelTime = database.street_segment_travelTime[street_segment_id];
    
    //obtain length by multiplying time with speed
    double segmentLength = segmentTravelTime * (getStreetSegmentInfo(street_segment_id).speedLimit);
    
    return segmentLength;
}

double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id) {
    //obtain travel time via the precomputed vector
    return database.street_segment_travelTime[street_segment_id];
}

int findClosestIntersection(LatLon my_position) {
    int minDist, minIndex = 0;

    //loop through all intersections and find the distance from my position
    //compare current and previous distances and update if closer
    for (int i = 0; i < getNumIntersections(); i++) {
        std::pair <LatLon, LatLon> positionPair(getIntersectionPosition(i), my_position);
        double dist = findDistanceBetweenTwoPoints(positionPair);

        //save the minimum distance and the corresponding intersection index
        if (i == 0 || dist < minDist) {
            minDist = dist;
            minIndex = i;
        }
    }

    return minIndex;
}

std::vector<StreetSegmentIdx> findStreetSegmentsOfIntersection(IntersectionIdx intersection_id) {
    //use intersection_street_segments vector created at load
    return database.intersection_street_segments[intersection_id];
}

std::vector<std::string> findStreetNamesOfIntersection(IntersectionIdx intersection_id) {
    //vector to store names of the streets
    std::vector<std::string> streetNames;
    
    int ss_num = getNumIntersectionStreetSegment(intersection_id);

    //loop through the street segment IDs attached to the intersection
    for (int i = 0; i < ss_num; i++) {
        int ss_id = database.intersection_street_segments[intersection_id][i];
        
        //find the street name using the segment info and add to the vector
        StreetSegmentInfo ss_info = getStreetSegmentInfo(ss_id);
        std::string streetName = getStreetName(ss_info.streetID);

        streetNames.push_back(streetName);
    }

    return streetNames;
}

std::vector<IntersectionIdx> findAdjacentIntersections(IntersectionIdx intersection_id) {
    //create a vector to store the intersections
    //use the function findStreetSegmentsOfIntersection to find its adjacent street segments
    
    std::vector<IntersectionIdx> adjacentIntersections;
    std::vector<StreetSegmentIdx> adjacentStreetSegments = findStreetSegmentsOfIntersection(intersection_id);

    //run a for loop to check its boolean oneWay
    //  if oneWay is false, return the adjacent intersection
    //  else oneWay is true
    //      if intersection is the 'from' point of the street segment, return the 'to' point
    //      else ignore
    
    for (std::vector<int>::iterator it = adjacentStreetSegments.begin(); it != adjacentStreetSegments.end(); it++) {
        StreetSegmentInfo street_segment = getStreetSegmentInfo(*it);
        if (street_segment.oneWay == false) {
            if (street_segment.from == intersection_id) {
                adjacentIntersections.push_back(street_segment.to);
            } 
            else {
                adjacentIntersections.push_back(street_segment.from);
            }
        } else {
            if (street_segment.from == intersection_id) {
                adjacentIntersections.push_back(street_segment.to);
            }
        }
    }
    
    //create an unordered set and store the vector into it
    //copy back to the vector

    std::unordered_set<int> s;
    for (auto i : adjacentIntersections) {
        s.insert(i);
    }
    adjacentIntersections.assign(s.begin(), s.end());

    std::copy(s.begin(), s.end(), adjacentIntersections.begin());

    return adjacentIntersections;
}

std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id) {
    //return the corresponding intersections using streetID_intersections database
    return database.streetID_intersections[street_id];
}

std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(std::pair<StreetIdx, StreetIdx> street_ids) {
    //create a vector for each street to store its intersections using function findIntersectionsOfStreet
    //create another vector to store the overlapping intersection points
    
    std::vector<IntersectionIdx> firstStreet = findIntersectionsOfStreet(street_ids.first);
    std::vector<IntersectionIdx> secondStreet = findIntersectionsOfStreet(street_ids.second);
    std::vector<IntersectionIdx> overlap;
    
    //sort firstStreet and secondStreet
    //use set_intersection of the two vectors to obtain the intersection points they have in common

    std::sort(firstStreet.begin(), firstStreet.end());
    std::sort(secondStreet.begin(), secondStreet.end());

    std::set_intersection(firstStreet.begin(), firstStreet.end(), secondStreet.begin(), secondStreet.end(), std::back_inserter(overlap));

    return overlap;
}

std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix) {

    std::vector<StreetIdx> matchingStreetIds;
    
    //check if prefix is empty - if empty, return an empty vector
    if (street_prefix.empty()) {
        return std::vector<StreetIdx>();
    }
    
    //erase all blank spaces and change street_prefix into lowercase
    street_prefix.erase(std::remove(street_prefix.begin(), street_prefix.end(), ' '), street_prefix.end()); //code snippet from https://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
    std::transform(street_prefix.begin(), street_prefix.end(), street_prefix.begin(), ::tolower); // code snippet from https://www.geeksforgeeks.org/conversion-whole-string-uppercase-lowercase-using-stl-c/
    
    //find the first instance of the prefix 
    auto itLow = database.streetName_and_streetID.lower_bound(street_prefix);
    
    //loop through the multi-map while prefix matches and push into vector 
    while (itLow != database.streetName_and_streetID.end() && ((itLow -> first).compare(0, street_prefix.size(), street_prefix)) == 0){
        matchingStreetIds.push_back(itLow -> second);
        itLow++;
    }
    
    return matchingStreetIds;
}

double findStreetLength(StreetIdx street_id) {

    double StreetLength = 0;

    //loop through the street segments
    for (auto it = 0; it < database.streetID_street_segments[street_id].size(); it++) {
        StreetSegmentIdx segment = database.streetID_street_segments[street_id][it];
        //add the segment length after each iteration
        StreetLength += findStreetSegmentLength(segment);
    }

    return StreetLength;
}

LatLonBounds findStreetBoundingBox(StreetIdx street_id) {

    //initialize the min/max of latitude and longitude that describes the bounding box
    double latMin = 0;
    double latMax = 0;
    double lonMin = 0;
    double lonMax = 0;

    //for all the streets with streetId
    //take each segment, update the largest/smallest latitude and longitude

    for (int i = 0; i < getNumStreetSegments(); i++) {
        if (getStreetSegmentInfo(i).streetID == street_id) {

            //simplify variable names
            double latFrom_temp = getIntersectionPosition(getStreetSegmentInfo(i).from).latitude();
            double latTo_temp = getIntersectionPosition(getStreetSegmentInfo(i).to).latitude();
            double lonFrom_temp = getIntersectionPosition(getStreetSegmentInfo(i).from).longitude();
            double lonTo_temp = getIntersectionPosition(getStreetSegmentInfo(i).to).longitude();

            //first segment always takes all values of min/max
            if (latMax == 0) {

                //compare the segment's from & to and the larger takes max, and the other takes the min
                if (latFrom_temp > latTo_temp) {
                    latMax = latFrom_temp;
                    latMin = latTo_temp;
                } else if (latFrom_temp < latTo_temp) {
                    latMax = latTo_temp;
                    latMin = latFrom_temp;
                } else if (latFrom_temp == latTo_temp) {
                    latMax = latTo_temp;
                    latMin = latTo_temp;
                }

                if (lonFrom_temp > lonTo_temp) {
                    lonMax = lonFrom_temp;
                    lonMin = lonTo_temp;
                } else if (lonFrom_temp < lonTo_temp) {
                    lonMax = lonTo_temp;
                    lonMin = lonFrom_temp;
                } else if (lonFrom_temp == lonTo_temp) {
                    lonMax = lonTo_temp;
                    lonMin = lonTo_temp;
                }

                //if the street has curve points, carry out the procedure with the curve points as well
                if (getStreetSegmentInfo(i).numCurvePoints != 0) {

                    //update value if if any one of the curve points exceeds max or is less than min obtained from the segment
                    for (int j = 0; j < getStreetSegmentInfo(i).numCurvePoints; j++) {
                        if (getStreetSegmentCurvePoint(i, j).latitude() < latMin) {
                            latMin = getStreetSegmentCurvePoint(i, j).latitude();
                        }
                        if (getStreetSegmentCurvePoint(i, j).latitude() > latMax) {
                            latMax = getStreetSegmentCurvePoint(i, j).latitude();
                        }
                        if (getStreetSegmentCurvePoint(i, j).longitude() < lonMin) {
                            lonMin = getStreetSegmentCurvePoint(i, j).longitude();
                        }
                        if (getStreetSegmentCurvePoint(i, j).longitude() > lonMax) {
                            lonMax = getStreetSegmentCurvePoint(i, j).longitude();
                        }
                    }
                }
            } 
            //if not the first segment, compare with existing value
            else {
                if (latTo_temp > latMax) {
                    latMax = latTo_temp;
                }
                if (latTo_temp < latMin) {
                    latMin = latTo_temp;
                }
                if (lonTo_temp > lonMax) {
                    lonMax = lonTo_temp;
                }
                if (lonTo_temp < lonMin) {
                    lonMin = lonTo_temp;
                }
                if (latFrom_temp > latMax) {
                    latMax = latFrom_temp;
                }
                if (latFrom_temp < latMin) {
                    latMin = latFrom_temp;
                }
                if (lonFrom_temp > lonMax) {
                    lonMax = lonFrom_temp;
                }
                if (lonFrom_temp < lonMin) {
                    lonMin = lonFrom_temp;
                }
                if (getStreetSegmentInfo(i).numCurvePoints != 0) {
                    //take into account the curve points
                    for (int j = 0; j < getStreetSegmentInfo(i).numCurvePoints; j++) {
                        if (getStreetSegmentCurvePoint(i, j).latitude() < latMin) {
                            latMin = getStreetSegmentCurvePoint(i, j).latitude();
                        }
                        if (getStreetSegmentCurvePoint(i, j).latitude() > latMax) {
                            latMax = getStreetSegmentCurvePoint(i, j).latitude();
                        }
                        if (getStreetSegmentCurvePoint(i, j).longitude() < lonMin) {
                            lonMin = getStreetSegmentCurvePoint(i, j).longitude();
                        }
                        if (getStreetSegmentCurvePoint(i, j).longitude() > lonMax) {
                            lonMax = getStreetSegmentCurvePoint(i, j).longitude();
                        }
                    }
                }
            }
        }
    }

    //define a LatLonBounds object which will contain all 4 bound points
    LatLonBounds streetBounds;

    LatLon minBounds(latMin, lonMin);
    LatLon maxBounds(latMax, lonMax);

    streetBounds.max = maxBounds;
    streetBounds.min = minBounds;

    return streetBounds;


}

POIIdx findClosestPOI(LatLon my_position, std::string POIname) {
    // make a vector consisting of the all the poi locations
    // compare using a for loop to return the shortest one

    double shortestDistance = 0;
    double currentDistance = 0;
    POIIdx closestPOIIdx = 0;

    //loop through all POIs; if the name matches input, insert into the vector; if no match, increment.
    for (int i = 0; i < getNumPointsOfInterest(); i++) { 
        if (getPOIName(i) == POIname) { 

            //create a pair of the match POI's position and my position
            std::pair <LatLon, LatLon> PositionPOIPair(my_position, getPOIPosition(i));
            //compute distance to current POI
            currentDistance = findDistanceBetweenTwoPoints(PositionPOIPair); 
            
            if (shortestDistance == 0) { //if first POI match, that POI has to be the closest
                shortestDistance = currentDistance;
                closestPOIIdx = i;
            } else if (shortestDistance > currentDistance) { //update only if current shorter than previous shortest
                shortestDistance = currentDistance;
                closestPOIIdx = i;
            }
        }
    }

    return closestPOIIdx;
}

double findFeatureArea(FeatureIdx feature_id) {
    //initialize totalFeatureArea
    double totalFeatureArea = 0;
    
    //obtain the first point and the last point of the feature area
    LatLon firstPoint = getFeaturePoint(feature_id, 0);
    LatLon lastPoint = getFeaturePoint(feature_id, getNumFeaturePoints(feature_id) - 1);

    //check whether the first point matches the last point - if not, it is not a close area
    if (firstPoint == lastPoint) {
        for (int i = 0; i < getNumFeaturePoints(feature_id) - 1; i++) {
            //obtain the current point and the next point using getFeaturePoint
            //compute the area by multiplying the average x value with the difference in y value
            LatLon current_point = getFeaturePoint(feature_id, i);
            LatLon next_point = getFeaturePoint(feature_id, i + 1);
            double current_point_y = kEarthRadiusInMeters * kDegreeToRadian * current_point.latitude();
            double next_point_y = kEarthRadiusInMeters * kDegreeToRadian * next_point.latitude();
            double x_average = 0.5 * kEarthRadiusInMeters * kDegreeToRadian * cos(0.5 * kDegreeToRadian * (current_point.latitude() + next_point.latitude())) * (current_point.longitude() + next_point.longitude());
            double area = abs(x_average * (next_point_y - current_point_y));
            
            //if the y coordinate of the next point is greater than that of the current point, add the computed area to the total area
            if (next_point_y > current_point_y) {
                totalFeatureArea += area;
            } 
            else {
                totalFeatureArea -= area;
            }
        }
    } 
    else {
        return 0;
    }

    return abs(totalFeatureArea);
}