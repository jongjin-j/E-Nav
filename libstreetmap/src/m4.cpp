/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#include <chrono>
#include <iostream>
#include "m1.h"
#include "globals.h"
#include "StreetsDatabaseAPI.h"
#include <string>
#include <string.h>
#include <list>
#include "m3.h"
#include "m4.h"
#include <unordered_map>
#include <queue>

#define NO_EDGE -1
//initial time has to be a large number
#define initial_bestTime 1000000000



bool multiDestDijkstra(int startIdx, int startID, int numOfImportantIntersections, std::vector<std::vector<int>>& pathTimes, std::unordered_map<IntersectionIdx, int> pickupIndexes, 
        std::unordered_map<IntersectionIdx, int> dropoffIndexes, std::unordered_map<IntersectionIdx, int> depotIndexes, double timePenalty);

std::vector<StreetSegmentIdx> traceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID);



std::vector<CourierSubPath> travelingCourier(const std::vector<DeliveryInf>& deliveries, const std::vector<int>& depots, const float turn_penalty){
    int N = deliveries.size();
    int M = depots.size();
    
    //database for all path times
    std::vector<std::vector<int>> timeForAllPaths;
    timeForAllPaths.resize(2*N+M);
    for (int i = 0; i < 2*N+M; ++i){
        timeForAllPaths[i].resize(2*N+M);
    }
    
    

    //store the indexes of the important intersections
    std::unordered_map<IntersectionIdx, int> pickupIndexes;
    std::unordered_map<IntersectionIdx, int> dropoffIndexes;
    std::unordered_map<IntersectionIdx, int> depotIndexes;
    
    
    //set up database for important intersections
    for (int i = 0; i < deliveries.size(); i++){
        pickupIndexes[deliveries[i].pickUp] = i;
    }
    for (int i = 0; i < deliveries.size(); i++){
        dropoffIndexes[deliveries[i].dropOff] = i;
    }
    for (int i = 0; i < depots.size(); i++){
        depotIndexes[depots[i]] = i;
    }
    
    //travel time using Dijkstra
    for (int i = 0; i < deliveries.size(); i++){
        multiDestDijkstra(i, deliveries[i].pickUp, 2*N+M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
        multiDestDijkstra(i+N, deliveries[i].dropOff, 2*N+M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
    }
    

    for (int i = 0; i < depots.size(); i++){
        multiDestDijkstra(i+2*N, depots[i], 2*N+M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
    }
}


struct WaveElem{
    Node *node;                 //node of wave element
    int edgeID;                 //id of segment used to get here
    double travelTime = 0;      //total time taken to reach node  
    double timeToDest = 0;      //estimated time to reach destination from current Node
    double totalTime = 0;       //total estimated time
    
    //constructor
    WaveElem (Node *n, int id, float travel_time){
        node = n;
        edgeID = id;
        travelTime = travel_time;
    }
};

//comparator for the priority queue
class myComparator
{
public:
    int operator() (const WaveElem& wave1, const WaveElem& wave2)
    {
        //compares the total time
        return wave1.travelTime > wave2.travelTime;
    }
};

bool multiDestDijkstra(int startIdx, int startID, int numOfImportantIntersections, std::vector<std::vector<int>>& pathTimes, std::unordered_map<IntersectionIdx, int> pickupIndexes, 
        std::unordered_map<IntersectionIdx, int> dropoffIndexes, std::unordered_map<IntersectionIdx, int> depotIndexes, double timePenalty){
    
    std::unordered_map<IntersectionIdx, Node*> intersections;
    //starting node 
    Node* sourceNode = new Node(startID);
    sourceNode -> bestTime = initial_bestTime;
    
    //insert into the unordered map intersection database
    intersections.insert({startID, sourceNode});
    
    int importantIntersectionCount = 0;
    /*int pickupCount = 0;
    int dropoffCount = 0;
    int depotCount = 0;*/
    
    //set a priority queue for the wave elements in the wavefront
    std::priority_queue <WaveElem, std::vector<WaveElem>, myComparator> wavefront;
    
    auto it = intersections.find(startID);
    
    /*//find the distance and the time to the destination
    std::pair<LatLon, LatLon> posPair (database.intersections[startID].pos, database.intersections[destID].pos);
    double distToDest = findDistanceBetweenTwoPoints(posPair);
    double timeToDest = distToDest/maxSpeed;*/
                
    //push in the first wave element
    wavefront.push(WaveElem(it->second, NO_EDGE, 0));
    
    
    
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
            
            
            auto pickupIt = pickupIndexes.find(currNode->id);
            auto dropoffIt = dropoffIndexes.find(currNode->id);
            auto depotIt = depotIndexes.find(currNode->id);
            
            if(pickupIt != pickupIndexes.end() && currNode->processed == false){
                currNode->processed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                pathTimes[startIdx][pickupIt->second] = time;
                importantIntersectionCount++;
            }
            if(dropoffIt != dropoffIndexes.end() && currNode->processed == false){
                currNode->processed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                pathTimes[startIdx][pickupIndexes.size()+dropoffIt->second] = time;
                importantIntersectionCount++;
            }
            if(depotIt != depotIndexes.end() && currNode->processed == false){
                currNode->processed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                pathTimes[startIdx][pickupIndexes.size()+dropoffIndexes.size()+depotIt->second] = time;
                importantIntersectionCount++;
            }
            
            //check whether path reached the destination intersection
            if(importantIntersectionCount == numOfImportantIntersections){
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
                    
                    auto legalIt = intersections.find(legalIntersection);
                
                    //check whether the node was visited or not(exists in the database or not)
                    //add a node to the database(unordered map) if it wasn't visited
                    if(legalIt == intersections.end()){
                        Node *toNode = new Node(legalIntersection);

                        intersections.insert({legalIntersection, toNode});
                    }

                    legalIt = intersections.find(legalIntersection);

                    //find the travel time of the segment
                    double travel_time = findStreetSegmentTravelTime(adjacentStreetSegments[i]);

                    //check whether the node's previous edge is not the starting edge
                    if (currNode->reachingEdge != NO_EDGE){
                        //if the previous edge and the current edge has the same streetID, don't apply the turn penalty and add to the wavefront
                        if (database.streetSegmentID_streetID[currNode->reachingEdge] == database.streetSegmentID_streetID[adjacentStreetSegments[i]]){
                            wavefront.push(WaveElem(legalIt->second, adjacentStreetSegments[i], currNode->bestTime + travel_time));
                        }
                        //if the previous edge and the current edge do not have the same streetID, apply the turn penalty and add to the wavefront
                        else{
                            wavefront.push(WaveElem(legalIt->second, adjacentStreetSegments[i], currNode->bestTime + travel_time + timePenalty));
                        }
                    }
                    //if it is a starting node
                    else{
                        wavefront.push(WaveElem(legalIt->second, adjacentStreetSegments[i], currNode->bestTime + travel_time));                
                    }
                }
            }
        }
    } 
    return false;
}

//trace back the path and store the segment IDs in a vector
std::vector<StreetSegmentIdx> traceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID){
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
