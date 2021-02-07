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
#include <math.h>
#include <algorithm> 

#define PI 3.14159265

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
std::vector<std::vector<StreetSegmentIdx>> intersection_street_segments;
std::vector<std::vector<StreetSegmentIdx>> streetID_street_segments;

bool loadMap(std::string map_streets_database_filename) {
    bool load_successful = false; //Indicates whether the map has loaded 
                                  //successfully

    std::cout << "loadMap: " << map_streets_database_filename << std::endl;
    
    //
    // Load your map related data structures here.
    //
    
    load_successful = loadStreetsDatabaseBIN(map_streets_database_filename);
    
    intersection_street_segments.resize(getNumIntersections());
    
    for(int intersection = 0; intersection < getNumIntersections(); ++intersection){
        for(int i = 0; i < getNumIntersectionStreetSegment(intersection); ++i){
            int ss_id = getIntersectionStreetSegment(intersection, i);
            intersection_street_segments[intersection].push_back(ss_id);
        }
    }
    
    streetID_street_segments.resize(getNumStreets());
    
    for(int i = 0; i < getNumStreetSegments(); i++){
        StreetSegmentInfo temp_segment = getStreetSegmentInfo(i);
        int temp_street_id = temp_segment.streetID;
        streetID_street_segments[temp_street_id].push_back(i);
    }

    load_successful = true; //Make sure this is updated to reflect whether
                            //loading the map succeeded or failed

    return load_successful;
}

void closeMap() {
    //Clean-up your map related data structures here
    closeStreetDatabase();
}

double findDistanceBetweenTwoPoints(std::pair<LatLon, LatLon> points){
    //return sqrt of x_difference^2 + y_difference^2 of the two points
    
    double latitudeAverage = (points.first.latitude() + points.second.latitude()) / 2;
    double x_coordinate_1 = kEarthRadiusInMeters * points.first.longitude() * cos(latitudeAverage * PI / 180);
    double y_coordinate_1 = kEarthRadiusInMeters * points.first.latitude();
    double x_coordinate_2 = kEarthRadiusInMeters * points.second.longitude() * cos(latitudeAverage * PI / 180);
    double y_coordinate_2 = kEarthRadiusInMeters * points.second.latitude();
    
    double x_diff = x_coordinate_2 - x_coordinate_1;
    double y_diff = y_coordinate_2 - y_coordinate_1;
    double diffSquared = pow(x_diff , 2) + pow(y_diff, 2);
    return sqrt(diffSquared);
}

double findStreetSegmentLength(StreetSegmentIdx street_segment_id){
    //load street_segment_info with getStreetSegmentInfo using street_segment_id
    //run for loop with getStreetSegmentCurvePoint by incrementing by 1 every loop
    //return the sum of the lengths
    
    double totalStreetSegmentLength = 0, firstSegmentLength = 0, lastSegmentLength = 0;
    StreetSegmentInfo street_segment = getStreetSegmentInfo(street_segment_id);
    
    if(street_segment.numCurvePoints == 0){
        LatLon posFrom = getIntersectionPosition(street_segment.from);
        LatLon posTo = getIntersectionPosition(street_segment.to);
        std::pair <LatLon, LatLon> segment (posFrom, posTo);
        totalStreetSegmentLength = findDistanceBetweenTwoPoints(segment);
    }
    
    else{
        LatLon segmentCurvePoints[street_segment.numCurvePoints];
        for(int curvePointNo = 0; curvePointNo < street_segment.numCurvePoints; curvePointNo++){
            segmentCurvePoints[curvePointNo] = getStreetSegmentCurvePoint(street_segment_id, curvePointNo);
        }
        
        //distance between from point and first curvePoint(0)
        LatLon posFrom = getIntersectionPosition(street_segment.from);
        std::pair <LatLon, LatLon> firstSegment (posFrom, segmentCurvePoints[0]);
        firstSegmentLength = findDistanceBetweenTwoPoints(firstSegment);
        totalStreetSegmentLength += firstSegmentLength;
    
        //distance between last curvePoint and to point(numCurvePoints - 1)
        LatLon posTo = getIntersectionPosition(street_segment.to);
        std::pair <LatLon, LatLon> lastSegment (segmentCurvePoints[street_segment.numCurvePoints - 1], posTo);
        lastSegmentLength = findDistanceBetweenTwoPoints(lastSegment);
        totalStreetSegmentLength += lastSegmentLength;
    
        //distance of each street segment excluding the first and the last street segment
        for(int i = 0; i < street_segment.numCurvePoints; i++){
            std::pair<LatLon, LatLon> tempSegment (segmentCurvePoints[i], segmentCurvePoints[i + 1]);
            totalStreetSegmentLength += findDistanceBetweenTwoPoints(tempSegment);
        }
    }
    
    return totalStreetSegmentLength;
}

double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id){
    //findStreetSegmentLength
    //divide by StreetSegmentInfo.speedLimit to obtain time
    //return time
    
    double streetSegmentLength = findStreetSegmentLength(street_segment_id);
    StreetSegmentInfo street_segment = getStreetSegmentInfo(street_segment_id);
    double travelTime = streetSegmentLength / (street_segment.speedLimit);
    
    return travelTime;
}

int findClosestIntersection(LatLon my_position){
    int minDist, minIndex;
    
    //loop through all intersections and find the distance from my position
    for (int i = 0; i<getNumIntersections(); i++){
        std::pair <LatLon, LatLon> positionPair (getIntersectionPosition(i), my_position);
        double dist = findDistanceBetweenTwoPoints(positionPair);
        
        //save the minimum distance and the corresponding intersection index
        if (i == 0 || dist < minDist){
            minDist = dist;
            minIndex = i;
        }
    }
    
    return minIndex;
    
    //my_position.latitude(): return latitude
    //my_position.longitude(): return longitude
    //for loop through intersections
    //run findDistanceBetweenTwoPoints(intersection_point, my_position)
    //if(currentDistance < previousDistance) use currentDistance
    //else keep previousDistance
    //return final intersection
}

std::vector<StreetSegmentIdx> findStreetSegmentsOfIntersection(IntersectionIdx intersection_id){
    //use intersection_street_segments vector created at load
    return intersection_street_segments[intersection_id];
}

std::vector<std::string> findStreetNamesOfIntersection(IntersectionIdx intersection_id){
    //vector to store names of the streets
    std::vector<std::string> streetNames;
    
    int ss_num = getNumIntersectionStreetSegment(intersection_id);
    bool duplicate = false;
    
    //loop through the segment IDs and get segment information, use them to find street name
    for (int i = 0; i < ss_num; i++){
        int ss_id = (intersection_street_segments[intersection_id])[i];
        
        StreetSegmentInfo ss_info = getStreetSegmentInfo(ss_id);
        
        std::string streetName = getStreetName(ss_info.streetID);
        
        //iterate through streetNames vector and find any duplicates
        for(std::vector<std::string>::iterator it = streetNames.begin(); it != streetNames.end(); it++){
            if (*it == streetName)
                duplicate = true;
        }           
        
        //if there are duplicates, do not push, if not, push into streetNames vector
        if (!duplicate)
            streetNames.push_back(streetName);
            
        duplicate = false;
    }
    
    return streetNames;
}

std::vector<IntersectionIdx> findAdjacentIntersections(IntersectionIdx intersection_id){
    //use function findStreetSegmentsOfIntersection
    //run a for loop to check its boolean oneWay
    //  if oneWay == false, return the adjacent intersection
    //  else oneWay == true
    //      if intersection == from, return adjacent intersection
    //      else if intersection == to, ignore
    
    std::vector<IntersectionIdx> adjacentIntersections;
    std::vector<StreetSegmentIdx> adjacentStreetSegments = findStreetSegmentsOfIntersection(intersection_id);
    
    for(std::vector<int>::iterator it = adjacentStreetSegments.begin(); it != adjacentStreetSegments.end(); it++){
        StreetSegmentInfo street_segment = getStreetSegmentInfo(intersection_id);
        if(street_segment.oneWay == false){
            if(street_segment.from == intersection_id){
                adjacentIntersections.push_back(street_segment.to);
            }
            else{
                adjacentIntersections.push_back(street_segment.from);
            }
        }
        else{
            if(street_segment.from == intersection_id){
                adjacentIntersections.push_back(street_segment.to);
            }
        }
    }
    
    return adjacentIntersections;
}

std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id){
    //vector to store intersections of a street
    std::vector <IntersectionIdx> streetIntersections; 
    
    for (std::vector<int>::iterator it = (streetID_street_segments[street_id]).begin(); it != (streetID_street_segments[street_id]).end(); it++){
        StreetSegmentInfo ss_info = getStreetSegmentInfo(*it);
        
        streetIntersections.push_back(ss_info.from);
        streetIntersections.push_back(ss_info.to);
    }
    
    std::sort (streetIntersections.begin(), streetIntersections.end());
    
    for (std::vector<int>::iterator it = streetIntersections.begin(); it != streetIntersections.end();){
        if (*it == *(it+1))
            it = streetIntersections.erase(it);
        else
            it++;
    }
    
    return streetIntersections;
}

std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(std::pair<StreetIdx, StreetIdx> street_ids){
    //create vectors for each street, use function findIntersectionsOfStreet
    //use set_intersection of the two vectors
    //assuming the vector is sorted from findIntersectionsOfStreet

    std::vector<IntersectionIdx> firstStreet = findIntersectionsOfStreet(street_ids.first);
    std::vector<IntersectionIdx> secondStreet = findIntersectionsOfStreet(street_ids.second);
    std::vector<IntersectionIdx> overlap;
    
    std::set_intersection (firstStreet.begin(), firstStreet.end(), secondStreet.begin(), secondStreet.end(), std::back_inserter(overlap));
    
    return overlap;
}

std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix){
    
    
}

double findStreetLength(StreetIdx street_id){
    //speed requirement high
    
    //for street_id == StreetSegmentInfo.streetID; (all segments of the street)
    //add each segment length to 'length' --how will we find the segment's length?
    //return length
    double StreetLength = 0;
    
    for(int i = 0; i<getNumStreetSegments(); i++){
        if(getStreetSegmentInfo(i).streetID == street_id){

            StreetLength = StreetLength + findStreetSegmentLength(i);
            
        }        
    }

    return StreetLength;
}

LatLonBounds findStreetBoundingBox(StreetIdx street_id){
// Return the smallest axis-aligned rectangle that contains all the 
// intersections and curve points of the given street (i.e. the min,max 
// lattitude and longitude bounds that can just contain all points of the 
// street).
// Speed Requirement --> none 
       
    double latMin = 0;
    double latMax = 0;
    double longMin = 0;
    double longMax = 0;
    
    for(int i = 0; i<getNumStreetSegments(); i++){
        bool firstSegment = true;
        if(getStreetSegmentInfo(i).streetID == street_id){
            
            //store the latitudes and longitudes of the segments
            //compute if the latlon of the next segments are larger or smaller
            //if smaller, skip; if larger, swap values
            
            //for the first segment:
            //xMax takes the larger latitude of from/to, xMin takes the smaller.
            //yMax takes the larger longitude of from/to, yMin takes the smaller.
            if (firstSegment){
                if(getIntersectionPosition(getStreetSegmentInfo(i).from).latitude() > getIntersectionPosition(getStreetSegmentInfo(i).to).latitude()){
                    latMax = getIntersectionPosition(getStreetSegmentInfo(i).from).latitude();
                    latMin = getIntersectionPosition(getStreetSegmentInfo(i).to).latitude();
                }
                else{
                    latMax = getIntersectionPosition(getStreetSegmentInfo(i).to).latitude();
                    latMin = getIntersectionPosition(getStreetSegmentInfo(i).from).latitude();
                }
                if(getIntersectionPosition(getStreetSegmentInfo(i).from).longitude() > getIntersectionPosition(getStreetSegmentInfo(i).to).longitude()){
                    longMax = getIntersectionPosition(getStreetSegmentInfo(i).from).longitude();
                    longMax = getIntersectionPosition(getStreetSegmentInfo(i).to).longitude();
                }
                else{
                    longMax = getIntersectionPosition(getStreetSegmentInfo(i).to).longitude();
                    longMax = getIntersectionPosition(getStreetSegmentInfo(i).from).longitude();
                }
            }
            
            else{
                
                if(getIntersectionPosition(getStreetSegmentInfo(i).to).latitude() > latMax){      //if the next segment's latitude larger than xMax, replace
                    latMax = getIntersectionPosition(getStreetSegmentInfo(i).to).latitude();
                }
                if(getIntersectionPosition(getStreetSegmentInfo(i).to).latitude() < latMin){      //if the next segment's latitude smaller than xMin, replace
                    latMin = getIntersectionPosition(getStreetSegmentInfo(i).to).latitude();
                }
                if(getIntersectionPosition(getStreetSegmentInfo(i).to).longitude() > longMax){     //if the next segment's longitude larger than yMax, replace
                    longMax = getIntersectionPosition(getStreetSegmentInfo(i).to).longitude();
                }
                if(getIntersectionPosition(getStreetSegmentInfo(i).to).longitude() < longMin){     //if the next segment's lontigude smaller than yMin, replace
                    longMin = getIntersectionPosition(getStreetSegmentInfo(i).to).longitude();
                }
                
            }
            
        }
        firstSegment = false;   //set boolean to false after first segment
    }
    
    //now all 4 values contain the end bounds of the street
    
    //now construct a latlonbounds object comprising of them and return.


    LatLon minimum(latMin, longMin);
    LatLon maximum(latMax, longMax);
    
    LatLonBounds streetBounds;
    streetBounds.max = maximum;
    streetBounds.min = minimum;
    
    return streetBounds;
    
}

POIIdx findClosestPOI(LatLon my_position, std::string POIname){
    // Returns the nearest point of interest of the given name to the given position
    // Speed Requirement --> none 
    
    // make a vector consisting of the all the poi locations
    // compare using a for loop to return the shortest one
    
    double shortestDistance = 0;
    double newDistance = 0;
    POIIdx closestPOIIdx = 0;
        
    for(int i = 0; i < getNumPointsOfInterest(); i++){  //loop through POI
        if(getPOIName(i) == POIname){                   //if poi name matches,
            
            std::pair <LatLon, LatLon> PositionPOIPair (my_position, getPOIPosition(i));

            newDistance = findDistanceBetweenTwoPoints(PositionPOIPair);   //pair of my_position and getPOIPosition
            if(shortestDistance == 0){
                shortestDistance = newDistance;
                closestPOIIdx = i;
            }
            else if(shortestDistance > newDistance){
                shortestDistance = newDistance;
                closestPOIIdx = i;
            }
        }
    }
    
    return closestPOIIdx;  
}

double findFeatureArea(FeatureIdx feature_id){
    //convert into xy coordinates
    //compute area by computing the larger area then subtracting the smaller
    double featureArea = 0;
    double y2, y1; //y = latitude
    double x2, x1; //x = longitude * cos(latitude.average)
    
    //int     getNumFeaturePoints(FeatureIdx featureIdx);
    //LatLon  getFeaturePoint(FeatureIdx featureIdx, int pointNum);
    
    
    (y2-y1) * (x2+x1) / 2;  //keep adding this, until y2-y1 is negative; when negative, start subtracting
        
    for(int i = 0; i<getNumFeaturePoints(feature_id);i++){
        
        //adding = getFeaturePoint(feature_id,i+1).latitude()-getFeaturePoint(feature_id,i).latitude();
        getFeaturePoint(feature_id,i).longitude();
                
    }
    
    //return 0 if not a closed polygon
    
    
    
}