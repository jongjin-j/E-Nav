/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <iostream>
#include "m1.h"
#include "globals.h"
#include "StreetsDatabaseAPI.h"
#include <string>
#include <string.h>
#include <list>
#include "m2.h"
#include "m3.h"
#include <unordered_map>
#include <queue>
#include <bits/stdc++.h>
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"

#define NO_EDGE -1
//initial time has to be a large number
#define initial_bestTime 1000000000


//waveElems have nodes with directions on how they got here
struct WaveElem{
    Node *node;                 //node of wave element
    int edgeID;                 //id of segment used to get here
    double travelTime = 0;      //total time taken to reach node  
    double timeToDest = 0;      //estimated time to reach destination from current Node
    double totalTime = 0;       //total estimated time
    
    //constructor
    WaveElem (Node *n, int id, float travel_time, float dest_time){
        node = n;
        edgeID = id;
        travelTime = travel_time;
        timeToDest = dest_time;
        totalTime = travel_time + dest_time;
        
    }
};

//comparator for the priority queue
class myComparator
{
public:
    int operator() (const WaveElem& wave1, const WaveElem& wave2)
    {
        //compares the total time
        return wave1.totalTime > wave2.totalTime;
    }
};

//computing the total travel time
double computePathTravelTime(const std::vector<StreetSegmentIdx>& path, const double turn_penalty){
    int pathSize = path.size();
    double totalTime = 0;
    
    //if there is no path
    if(pathSize == 0){
        return 0;
    }

    //loop through the segments and add the travel time for each 
    for(int i = 0; i < pathSize; i++){
        totalTime += findStreetSegmentTravelTime(path[i]);
        
        if(i < pathSize - 1){
            //add the turn penalty if the street ID changes
            if(database.street_segments[path[i]].ID != database.street_segments[path[i + 1]].ID){
                totalTime += turn_penalty;
            }
        }
    }
    
    return totalTime;
}

//intersectionIDs are returned at (startIntersectionID, destIntersectionID)
//take startIntersectionID and destIntersectionID as start and finish

//finding the path between intersections
std::vector<StreetSegmentIdx> findPathBetweenIntersections(const IntersectionIdx intersect_id_start, 
const IntersectionIdx intersect_id_destination,const double turn_penalty){
    //create database of unordered map for intersections as nodes
    std::unordered_map<IntersectionIdx, Node*> intersections;
    
    //starting node 
    Node* sourceNode = new Node(intersect_id_start);
    sourceNode -> bestTime = initial_bestTime;
    
    //insert into the unordered map intersection database
    intersections.insert({intersect_id_start, sourceNode});
    
    //find the optimal path
    bool found = AStarPath(intersections, intersect_id_start, intersect_id_destination, turn_penalty);
    
    std::vector<StreetSegmentIdx> path;
    
    //return a path if a path exists between the intersections
    if(found){
        path = AStarTraceBack(intersections, intersect_id_destination);
    }
    
    //delete the unordered map
    for (int i = 0; i < intersections.size(); i++){
        delete intersections[i];
    }
    
    return path;
}

//find the path using A* algorithm
bool AStarPath(std::unordered_map<IntersectionIdx, Node*>& intersections, int startID, int destID, double timePenalty){
    
    //set a priority queue for the wave elements in the wavefront
    std::priority_queue <WaveElem, std::vector<WaveElem>, myComparator> wavefront;
    
    auto it = intersections.find(startID);
    
    //find the distance and the time to the destination
    std::pair<LatLon, LatLon> posPair (database.intersections[startID].pos, database.intersections[destID].pos);
    double distToDest = findDistanceBetweenTwoPoints(posPair);
    double timeToDest = distToDest/maxSpeed;
                
    //push in the first wave element
    wavefront.push(WaveElem(it->second, NO_EDGE, 0, timeToDest));
    
    //checking the wavefronts until it is empty
    while(wavefront.size()!=0){
        
        //take the first in wavefront to be currentNode
        WaveElem wave = wavefront.top();  
        //remove the first element
        wavefront.pop();
        
        //currNode points to the wave node
        Node *currNode = wave.node;
        
        //check if found a better path
        if (wave.travelTime < currNode->bestTime) {
            
            //update the reaching edge and best time
            currNode->reachingEdge = wave.edgeID;         
            currNode->bestTime = wave.travelTime;
            
            //check whether path reached the destination intersection
            if(currNode->id == destID){
                return true;
            }
            
            //vector for adjacent street segments
            std::vector<StreetSegmentIdx> adjacentStreetSegments = findStreetSegmentsOfIntersection(currNode->id);
            
            for(int i = 0; i < adjacentStreetSegments.size(); i++){
                IntersectionIdx legalIntersection;
                
                //boolean for checking if street segment is legal 
                bool legal = false;
                
                //if street segment is legal
                if(database.street_segments[adjacentStreetSegments[i]].intersection_from == currNode->id || database.street_segments[adjacentStreetSegments[i]].oneWay == false){

                    legal = true;
                    
                    //find the intersection indexes at the other end of the segment
                    if (database.street_segments[adjacentStreetSegments[i]].intersection_from == currNode->id){
                        legalIntersection = database.street_segments[adjacentStreetSegments[i]].intersection_to;
                    }

                    if (database.street_segments[adjacentStreetSegments[i]].intersection_to == currNode->id){
                        legalIntersection = database.street_segments[adjacentStreetSegments[i]].intersection_from;
                    }
                }
               
                //if the street segment was legal
                if (legal){
                    
                    it = intersections.find(legalIntersection);
                
                    //check whether the node was visited or not(exists in the database or not)
                    //add a node to the database(unordered map) if it wasn't visited
                    if(it == intersections.end()){
                        Node *toNode = new Node(legalIntersection);

                        intersections.insert({legalIntersection, toNode});
                    }

                    it = intersections.find(legalIntersection);

                    //heuristics (time taken to reach the destination from current position with euclidian distance)
                    std::pair<LatLon, LatLon> currentPosPair (database.intersections[legalIntersection].pos, database.intersections[destID].pos);
                    double currentDistToDest = findDistanceBetweenTwoPoints(currentPosPair);
                    double currentTimeToDest = currentDistToDest/maxSpeed;

                    //find the travel time of the segment
                    double travel_time = findStreetSegmentTravelTime(adjacentStreetSegments[i]);

                    //check whether the node's previous edge is not the starting edge
                    if (currNode->reachingEdge != NO_EDGE){
                        //if the previous edge and the current edge has the same streetID, don't apply the turn penalty and add to the wavefront
                        if (database.streetSegmentID_streetID[currNode->reachingEdge] == database.streetSegmentID_streetID[adjacentStreetSegments[i]]){
                            wavefront.push(WaveElem(it->second, adjacentStreetSegments[i], currNode->bestTime + travel_time, currentTimeToDest));
                        }
                        //if the previous edge and the current edge do not have the same streetID, apply the turn penalty and add to the wavefront
                        else{
                            wavefront.push(WaveElem(it->second, adjacentStreetSegments[i], currNode->bestTime + travel_time + timePenalty, currentTimeToDest));
                        }
                    }
                    //if it is a starting node
                    else{
                        wavefront.push(WaveElem(it->second, adjacentStreetSegments[i], currNode->bestTime + travel_time, currentTimeToDest));                
                    }
                }
            }
        }
    } 
    return false;
}

//trace back the path and store the segment IDs in a vector
std::vector<StreetSegmentIdx> AStarTraceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID){
    std::vector<StreetSegmentIdx> pathToDest; 
    
    //find the Node with the destination intersection ID
    auto it = intersections.find(destID);
    Node *currNode = it->second;
    StreetSegmentIdx prevEdge = currNode->reachingEdge;
    
    //while the starting node hasn't been reached
    while(prevEdge != NO_EDGE){
        
        //add the previous edge to the path
        pathToDest.push_back(prevEdge);
                
        //set the next Node to the intersection that is on the other side of the street segment
        if(currNode->id == database.street_segments[prevEdge].intersection_from){
            auto k = intersections.find(database.street_segments[prevEdge].intersection_to);
            currNode = k->second;
        }
        
        else if(currNode->id == database.street_segments[prevEdge].intersection_to){
            auto k = intersections.find(database.street_segments[prevEdge].intersection_from);
            currNode = k->second;
        }
        
        //update the previous edge to the reaching edge
        prevEdge = currNode->reachingEdge;
    }
    
    //reverse the vector since it started from the destination
    std::reverse(pathToDest.begin(),pathToDest.end());
    
    return pathToDest;
}

/*ezgl::renderer *g;
g->set_color(ezgl::BLUE);*/

/*LatLon p1(getIntersectionPosition(currNode->id).latitude(), getIntersectionPosition(currNode->id).longitude());
LatLon p2(getIntersectionPosition(legalIntersection).latitude(), getIntersectionPosition(legalIntersection).longitude());
ezgl::point2d point1(x_from_lon(p1.longitude()), y_from_lat(p1.latitude()));
ezgl::point2d point2(x_from_lon(p2.longitude()), y_from_lat(p2.latitude()));

g->draw_line(point1, point2);*/