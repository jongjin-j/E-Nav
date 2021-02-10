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
#include <string.h>
#include <math.h>
#include <algorithm>
#include <locale> 
#include <unordered_set>
#include <cctype>
#include <map>

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
std::vector<std::vector<StreetIdx>> streetID_intersections;
std::vector<std::string> simplifiedStreetNames;
std::vector<std::vector<double>> street_segment_length_and_time;
std::multimap<std::string, double> streetName_and_streetID;

bool loadMap(std::string map_streets_database_filename) {
    bool load_successful = false; //Indicates whether the map has loaded 
    //successfully

    std::cout << "loadMap: " << map_streets_database_filename << std::endl;

    //
    // Load your map related data structures here.
    //

    load_successful = loadStreetsDatabaseBIN(map_streets_database_filename);
    
    //initializing the multimap
    for(int i = 0; i < getNumStreets(); i++){
        streetName_and_streetID.insert(std::make_pair(getStreetName(i),i)); 
    }    
    
    
   
    if(load_successful == false){
        return false;
    }
    intersection_street_segments.resize(getNumIntersections());

    for (int intersection = 0; intersection < getNumIntersections(); ++intersection) {
        for (int i = 0; i < getNumIntersectionStreetSegment(intersection); ++i) {
            int ss_id = getIntersectionStreetSegment(intersection, i);
            intersection_street_segments[intersection].push_back(ss_id);
        }
    }

    streetID_street_segments.resize(getNumStreets());

    for (int i = 0; i < getNumStreetSegments(); i++) {
        StreetSegmentInfo temp_segment = getStreetSegmentInfo(i);
        int temp_street_id = temp_segment.streetID;
        streetID_street_segments[temp_street_id].push_back(i);
    }

    streetID_intersections.resize(getNumStreets());

    for (int i = 0; i < getNumStreets(); i++) {
        for (auto it = streetID_street_segments[i].begin(); it != streetID_street_segments[i].end(); it++) {
            StreetSegmentIdx ss_idx = *it;
            StreetSegmentInfo ss_info = getStreetSegmentInfo(ss_idx);
            streetID_intersections[i].push_back(ss_info.from);
            streetID_intersections[i].push_back(ss_info.to);
        }

        //erase duplicates
        std::unordered_set<int> s;
        for (auto j : streetID_intersections[i]) {
            s.insert(j);
        }
        streetID_intersections[i].assign(s.begin(), s.end());

        std::copy(s.begin(), s.end(), streetID_intersections[i].begin());
    }

    simplifiedStreetNames.resize(getNumStreets());

    for (StreetIdx i = 0; i < getNumStreets(); i++) {
        std::string streetName = getStreetName(i);

        //remove blank spaces and change to lowercase 
        streetName.erase(std::remove(streetName.begin(), streetName.end(), ' '), streetName.end()); //code snippet from https://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
        std::transform(streetName.begin(), streetName.end(), streetName.begin(), ::tolower); // code snippet from https://www.geeksforgeeks.org/conversion-whole-string-uppercase-lowercase-using-stl-c/

        simplifiedStreetNames.push_back(streetName);
    }

    street_segment_length_and_time.resize(getNumStreetSegments());

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
            LatLon *segmentCurvePoints = new LatLon [curvePoints];
            for (int i = 0; i < curvePoints; i++) {
                segmentCurvePoints[i] = getStreetSegmentCurvePoint(streetSegNum, i);
            }

            //distance between from point and first curvePoint(0)
            LatLon posFrom = getIntersectionPosition(street_segment.from);
            std::pair <LatLon, LatLon> firstSegment(posFrom, segmentCurvePoints[0]);
            firstSegmentLength = findDistanceBetweenTwoPoints(firstSegment);

            //distance between last curvePoint and to point(numCurvePoints - 1)
            LatLon posTo = getIntersectionPosition(street_segment.to);
            std::pair <LatLon, LatLon> lastSegment(segmentCurvePoints[street_segment.numCurvePoints - 1], posTo);
            lastSegmentLength = findDistanceBetweenTwoPoints(lastSegment);

            //distance of each street segment excluding the first and the last street segment
            for (int i = 0; i < curvePoints - 1; i++) {
                std::pair<LatLon, LatLon> tempSegment(segmentCurvePoints[i], segmentCurvePoints[i + 1]);
                curveSegmentLength += findDistanceBetweenTwoPoints(tempSegment);
            }
            totalStreetSegmentLength = firstSegmentLength + lastSegmentLength + curveSegmentLength;
            delete[] segmentCurvePoints;
        }
        street_segment_length_and_time[streetSegNum].push_back(totalStreetSegmentLength);
        double speed = street_segment.speedLimit;
        street_segment_length_and_time[streetSegNum].push_back(totalStreetSegmentLength / speed);
    }


    load_successful = true; //Make sure this is updated to reflect whether
    //loading the map succeeded or failed

    return load_successful;
}

void closeMap() {
    //Clean-up your map related data structures here
    //Delete the three vectors created
    std::vector<std::vector < StreetSegmentIdx >> ().swap(intersection_street_segments);
    std::vector<std::vector < StreetSegmentIdx >> ().swap(streetID_street_segments);
    std::vector<std::vector < StreetIdx >> ().swap(streetID_intersections);
    std::vector<std::string>().swap(simplifiedStreetNames);
    std::vector<std::vector<double>>().swap(street_segment_length_and_time);

    closeStreetDatabase();
}

double findDistanceBetweenTwoPoints(std::pair<LatLon, LatLon> points) {
    //return sqrt of x_difference^2 + y_difference^2 of the two points

    /*double latitudeAverage = 0.5 * kDegreeToRadian * (points.first.latitude() + points.second.latitude());
    double x_coordinate_1 = kEarthRadiusInMeters * points.first.longitude() * kDegreeToRadian * cos(latitudeAverage);
    double y_coordinate_1 = kEarthRadiusInMeters * points.first.latitude() * kDegreeToRadian;
    double x_coordinate_2 = kEarthRadiusInMeters * points.second.longitude() * kDegreeToRadian * cos(latitudeAverage);
    double y_coordinate_2 = kEarthRadiusInMeters * points.second.latitude() * kDegreeToRadian;

    double x_diff = x_coordinate_2 - x_coordinate_1;
    double y_diff = y_coordinate_2 - y_coordinate_1;
    double diffSquared = pow(x_diff, 2) + pow(y_diff, 2);
    return sqrt(diffSquared);*/
    
    //simplified without using pow
    double xDiff = kEarthRadiusInMeters * kDegreeToRadian * cos(0.5 * kDegreeToRadian * (points.first.latitude() + points.second.latitude())) * (points.second.longitude() - points.first.longitude());
    double yDiff = kEarthRadiusInMeters * kDegreeToRadian * (points.second.latitude() - points.first.latitude());
    return sqrt(xDiff * xDiff + yDiff * yDiff);
}

double findStreetSegmentLength(StreetSegmentIdx street_segment_id) {
    //load street_segment_info with getStreetSegmentInfo using street_segment_id
    //run for loop with getStreetSegmentCurvePoint by incrementing by 1 every loop
    //return the sum of the lengths

    return street_segment_length_and_time[street_segment_id][0];
}

double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id) {
    /*double streetSegmentLength = findStreetSegmentLength(street_segment_id);
    StreetSegmentInfo street_segment = getStreetSegmentInfo(street_segment_id);
    double travelTime = streetSegmentLength / (street_segment.speedLimit);*/

    return street_segment_length_and_time[street_segment_id][1];
}

int findClosestIntersection(LatLon my_position) {
    int minDist, minIndex = 0;

    //loop through all intersections and find the distance from my position
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
    return intersection_street_segments[intersection_id];
}

std::vector<std::string> findStreetNamesOfIntersection(IntersectionIdx intersection_id) {
    //vector to store names of the streets
    std::vector<std::string> streetNames;

    int ss_num = getNumIntersectionStreetSegment(intersection_id);

    //loop through the segment IDs and get segment information, use them to find street name
    for (int i = 0; i < ss_num; i++) {
        int ss_id = intersection_street_segments[intersection_id][i];

        StreetSegmentInfo ss_info = getStreetSegmentInfo(ss_id);

        std::string streetName = getStreetName(ss_info.streetID);

        streetNames.push_back(streetName);
    }

    return streetNames;
}

std::vector<IntersectionIdx> findAdjacentIntersections(IntersectionIdx intersection_id) {
    //use function findStreetSegmentsOfIntersection
    //run a for loop to check its boolean oneWay
    //  if oneWay == false, return the adjacent intersection
    //  else oneWay == true
    //      if intersection == from, return adjacent intersection
    //      else if intersection == to, ignore

    std::vector<IntersectionIdx> adjacentIntersections;
    std::vector<StreetSegmentIdx> adjacentStreetSegments = findStreetSegmentsOfIntersection(intersection_id);

    for (std::vector<int>::iterator it = adjacentStreetSegments.begin(); it != adjacentStreetSegments.end(); it++) {
        StreetSegmentInfo street_segment = getStreetSegmentInfo(*it);
        if (street_segment.oneWay == false) {
            if (street_segment.from == intersection_id) {
                adjacentIntersections.push_back(street_segment.to);
            } else {
                adjacentIntersections.push_back(street_segment.from);
            }
        } else {
            if (street_segment.from == intersection_id) {
                adjacentIntersections.push_back(street_segment.to);
            }
        }
    }

    std::unordered_set<int> s;
    for (auto i : adjacentIntersections) {
        s.insert(i);
    }
    adjacentIntersections.assign(s.begin(), s.end());

    std::copy(s.begin(), s.end(), adjacentIntersections.begin());

    return adjacentIntersections;
}

std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id) {
    return streetID_intersections[street_id];
}

std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(std::pair<StreetIdx, StreetIdx> street_ids) {
    //create vectors for each street, use function findIntersectionsOfStreet
    //use set_intersection of the two vectors
    //assuming the vector is sorted from findIntersectionsOfStreet

    std::vector<IntersectionIdx> firstStreet = findIntersectionsOfStreet(street_ids.first);
    std::vector<IntersectionIdx> secondStreet = findIntersectionsOfStreet(street_ids.second);
    std::vector<IntersectionIdx> overlap;

    std::sort(firstStreet.begin(), firstStreet.end());
    std::sort(secondStreet.begin(), secondStreet.end());

    std::set_intersection(firstStreet.begin(), firstStreet.end(), secondStreet.begin(), secondStreet.end(), std::back_inserter(overlap));

    return overlap;
}

std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix) {
    /*
    std::vector<StreetIdx> matchingStreetIds;
    std::string streetName;

    //if prefix is empty
    if (street_prefix.empty()) {
        return std::vector<StreetIdx>();
    }

    //erase all blank spaces and change street_prefix into lowercase
    street_prefix.erase(std::remove(street_prefix.begin(), street_prefix.end(), ' '), street_prefix.end()); //code snippet from https://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
    std::transform(street_prefix.begin(), street_prefix.end(), street_prefix.begin(), ::tolower); // code snippet from https://www.geeksforgeeks.org/conversion-whole-string-uppercase-lowercase-using-stl-c/

    //loop through the streets and find match
    for (StreetIdx i = 0; i < getNumStreets(); i++) {
        streetName = getStreetName(i);

        //erase all the blanks and change street name to lower case
        streetName.erase(std::remove(streetName.begin(), streetName.end(), ' '), streetName.end()); //code snippet from https://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
        std::transform(streetName.begin(), streetName.end(), streetName.begin(), ::tolower);// code snippet from https://www.geeksforgeeks.org/conversion-whole-string-uppercase-lowercase-using-stl-c/

        if ((street_prefix.compare(0, street_prefix.size(), streetName)) == 0)
            matchingStreetIds.push_back(i);
    }

    return matchingStreetIds;
     * */

    /*
    std::vector<StreetIdx> matchingStreetIds;
    
    //if prefix is empty
    if (street_prefix.empty()) {
        return std::vector<StreetIdx>();
    }
    
    for (StreetIdx i = 0; i < getNumStreets(); i++) {
        if (((simplifiedStreetNames[i]).compare(0, street_prefix.size(), street_prefix)) == 0)
            matchingStreetIds.push_back(i);
    }
    
    return matchingStreetIds;
     */


    std::string searchTerm = street_prefix;
    std::vector<StreetIdx> matchingStreets;

    //converting to nonwhitespace+lowercase
    searchTerm.erase(remove(searchTerm.begin(), searchTerm.end(), ' '), searchTerm.end());
    std::transform(searchTerm.begin(), searchTerm.end(), searchTerm.begin(), [](unsigned char c) {
        return std::tolower(c);
    }); //https://stackoverflow.com/questions/313970/how-to-convert-stdstring-to-lower-case

    for (int i = 1; i < getNumStreets(); i++) { //of large magnitude
        bool currentlyMatching = true; //initialize to true at the start of each street

        for (int j = 0; j < strlen(searchTerm.c_str()); j++) { //of small magnitude, double loop ok?

            std::string streetToCompare = getStreetName(i); //convert to string because getstreetname(i) is in bin form?

            //converting to nonwhitespace+lowercase
            streetToCompare.erase(remove(streetToCompare.begin(), streetToCompare.end(), ' '), streetToCompare.end());
            std::transform(streetToCompare.begin(), streetToCompare.end(), streetToCompare.begin(), [](unsigned char c) {
                return std::tolower(c);
            }); //https://stackoverflow.com/questions/313970/how-to-convert-stdstring-to-lower-case

            if (streetToCompare[j] == searchTerm[j]) {
                //continue if chars matched
            } else {
                //if one char doesn't match, break
                currentlyMatching = false;
                break;
            }


        }
        if (currentlyMatching) {
            matchingStreets.push_back(i);
        }

    }

    return matchingStreets;
}

double findStreetLength(StreetIdx street_id) {
    //speed requirement high

    double StreetLength = 0;

    for (auto it = 0; it < streetID_street_segments[street_id].size(); it++) {
        StreetSegmentIdx segment = streetID_street_segments[street_id][it];
        StreetLength += findStreetSegmentLength(segment);
    }

    return StreetLength;
}

LatLonBounds findStreetBoundingBox(StreetIdx street_id) {

    double latMin = 0;
    double latMax = 0;
    double lonMin = 0;
    double lonMax = 0;

    //for all the streets with streetid;
    //take each segment, record the largest/smallest lat and lon
    //as you go through new segments, update the largest/smallest

    for (int i = 0; i < getNumStreetSegments(); i++) {
        if (getStreetSegmentInfo(i).streetID == street_id) {

            double latFrom_temp = getIntersectionPosition(getStreetSegmentInfo(i).from).latitude();
            double latTo_temp = getIntersectionPosition(getStreetSegmentInfo(i).to).latitude();
            double lonFrom_temp = getIntersectionPosition(getStreetSegmentInfo(i).from).longitude();
            double lonTo_temp = getIntersectionPosition(getStreetSegmentInfo(i).to).longitude();

            if (latMax == 0) {

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

            } else {
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

    LatLonBounds streetBounds;

    LatLon minBounds(latMin, lonMin);
    LatLon maxBounds(latMax, lonMax);

    streetBounds.max = maxBounds;
    streetBounds.min = minBounds;

    return streetBounds;


}

POIIdx findClosestPOI(LatLon my_position, std::string POIname) {
    // Returns the nearest point of interest of the given name to the given position
    // Speed Requirement --> none 

    // make a vector consisting of the all the poi locations
    // compare using a for loop to return the shortest one

    double shortestDistance = 0;
    double newDistance = 0;
    POIIdx closestPOIIdx = 0;

    for (int i = 0; i < getNumPointsOfInterest(); i++) { //loop through POI
        if (getPOIName(i) == POIname) { //if poi name matches,

            std::pair <LatLon, LatLon> PositionPOIPair(my_position, getPOIPosition(i));

            newDistance = findDistanceBetweenTwoPoints(PositionPOIPair); //pair of my_position and getPOIPosition
            if (shortestDistance == 0) {
                shortestDistance = newDistance;
                closestPOIIdx = i;
            } else if (shortestDistance > newDistance) {
                shortestDistance = newDistance;
                closestPOIIdx = i;
            }
        }
    }

    return closestPOIIdx;
}

double findFeatureArea(FeatureIdx feature_id) {
    //convert into x,y coordinates
    LatLon firstPoint = getFeaturePoint(feature_id, 0);
    LatLon lastPoint = getFeaturePoint(feature_id, getNumFeaturePoints(feature_id) - 1);
    double totalArea = 0;

    if (firstPoint == lastPoint) {
        for (int i = 0; i < getNumFeaturePoints(feature_id) - 1; i++) {
            LatLon current_point = getFeaturePoint(feature_id, i);
            LatLon next_point = getFeaturePoint(feature_id, i + 1);
            double latitudeAverage = 0.5 * kDegreeToRadian * (current_point.latitude() + next_point.latitude());
            double current_point_x = kEarthRadiusInMeters * current_point.longitude() * kDegreeToRadian * cos(latitudeAverage);
            double current_point_y = kEarthRadiusInMeters * current_point.latitude() * kDegreeToRadian;
            double next_point_x = kEarthRadiusInMeters * next_point.longitude() * kDegreeToRadian * cos(latitudeAverage);
            double next_point_y = kEarthRadiusInMeters * next_point.latitude() * kDegreeToRadian;
            double x_average = 0.5 * (next_point_x + current_point_x);
            double y_diff = next_point_y - current_point_y;
            double area = abs(x_average * y_diff);

            if (next_point_y > current_point_y) {
                totalArea += area;
            } else {
                totalArea -= area;
            }
        }

        double latitude_Average = 0.5 * kDegreeToRadian * (firstPoint.latitude() + lastPoint.latitude());
        double first_point_x = kEarthRadiusInMeters * firstPoint.longitude() * kDegreeToRadian * cos(latitude_Average);
        double first_point_y = kEarthRadiusInMeters * firstPoint.latitude() * kDegreeToRadian;
        double last_point_x = kEarthRadiusInMeters * lastPoint.longitude() * kDegreeToRadian * cos(latitude_Average);
        double last_point_y = kEarthRadiusInMeters * lastPoint.latitude() * kDegreeToRadian;
        double x_average = 0.5 * (first_point_x + last_point_x);
        double y_diff = first_point_y - last_point_y;
        double area = abs(x_average * y_diff);

        if (first_point_y > last_point_y) {
            totalArea += area;
        } else {
            totalArea -= area;
        }
    } else {
        return 0;
    }

    return abs(totalArea);
}