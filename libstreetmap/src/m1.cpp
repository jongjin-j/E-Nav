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

bool loadMap(std::string map_streets_database_filename) {
    bool load_successful = false; //Indicates whether the map has loaded 
                                  //successfully

    std::cout << "loadMap: " << map_streets_database_filename << std::endl;

    //
    // Load your map related data structures here.
    //
    intersection_street_segments.resize(getNumIntersections());
    
    for(int intersection = 0; intersection < getNumIntersections(); ++intersection){
        for(int i = 0; i < getNumIntersectionStreetSegment(intersection); ++i){
            int ss_id = getIntersectionStreetSegment(intersection, i);
            intersection_street_segments[intersection].push_back(ss_id);
        }
    }

    load_successful = true; //Make sure this is updated to reflect whether
                            //loading the map succeeded or failed

    return load_successful;
}

void closeMap() {
    //Clean-up your map related data structures here
    
}

double findDistanceBetweenTwoPoints(std::pair<LatLon, LatLon> points){
    //return sqrt of LatDiff^2 + LonDiff^2 of the two points
    double latitudeDiff = points.first.latitude() - points.second.latitude();
    double longitudeDiff = points.first.longitude() - points.second.longitude();
    double diffSquared = pow(latitudeDiff , 2) + pow(longitudeDiff, 2);
    return sqrt(diffSquared);
}

double findStreetSegmentLength(StreetSegmentIdx street_segment_id){
    //load street_segment_info with getStreetSegmentInfo using street_segment_id
    //run for loop with getStreetSegmentCurvePoint by incrementing by 1 every loop
    //return the sum of the lengths
    
    double totalStreetSegmentLength = 0, firstSegmentLength = 0, lastSegmentLength = 0;
    StreetSegmentInfo street_segment;
    street_segment = getStreetSegmentInfo(street_segment_id);
    
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
    
        //distance between last curvePoint and to point(numCurvePoints - 1)
        LatLon posTo = getIntersectionPosition(street_segment.to);
        std::pair <LatLon, LatLon> lastSegment (segmentCurvePoints[street_segment.numCurvePoints - 1], posTo);
        lastSegmentLength = findDistanceBetweenTwoPoints(lastSegment);
    
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
}

int findClosestIntersection(LatLon my_position){
    int minDist, minIndex;
    
    
    for (int i = 0; i<getNumIntersections(); i++){
        std::pair <LatLon, LatLon> positionPair (getIntersectionPosition(i), my_position);
        double dist = findDistanceBetweenTwoPoints(positionPair);
        
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
    return intersection_street_segments[intersection_id];
}

std::vector<std::string> findStreetNamesOfIntersection(IntersectionIdx intersection_id){
    //take in intersection_id
    //declare a string vector
    //find street segments associated with the intersection; store in a string vector with pushback
    //find the street names of the segments; print using loop
    //return the vector
    
    //might want to find ways to reduce O(n) 
}

std::vector<IntersectionIdx> findAdjacentIntersections(IntersectionIdx intersection_id){
    
}

std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id){
    //define an int array; we'll store the street intersections here
    //with the street id, find all its street segments (ie StreetSegmentInfo.streetID == street_id)
    //find the intersection points of these segments
    //store as you go, return the vector
}

std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(std::pair<StreetIdx, StreetIdx> street_ids){
    
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
            if(i==0){
                shortestDistance == newDistance;
                closestPOIIdx = i;
            }
            else if(shortestDistance > newDistance){
                shortestDistance == newDistance;
                closestPOIIdx = i;
            }
        }
    }
    
    return closestPOIIdx;  
}

double findFeatureArea(FeatureIdx feature_id){
    
}