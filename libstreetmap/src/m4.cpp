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
#include "m4.h"
#include <unordered_map>
#include <queue>

#define NO_EDGE -1
//initial time has to be a large number
#define initial_bestTime 1000000000

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

bool multiDestDijkstra(std::unordered_map<IntersectionIdx, Node*>& intersections, int startID, int destID, double timePenalty);

std::vector<StreetSegmentIdx> traceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID);



std::vector<CourierSubPath> travelingCourier(const std::vector<DeliveryInf>& deliveries, const std::vector<int>& depots, const float turn_penalty){
    std::unordered_map<IntersectionIdx, int> importantIntersections;
    for (int i = 0; i < deliveries.size(); i++){
        importantIntersections[deliveries[i].pickUp] = 1;
        importantIntersections[deliveries[i].dropOff] = 1;
    }
    
    for (int i = 0; i < depots.size(); i++){
        importantIntersections[depots[i]] = 1;
    }
    
    std::unordered_map<IntersectionIdx, Node*> intersections;
    
    
    
    //vector that contains all path from each Delivery Location to another or to a Depot
    //std::vector<std::vector<std::vector<StreetSegmentIdx>>> pathFromDeliveryLoc [2 * deliveries.size()][2 * deliveries.size() + depots.size()][];
    std::unordered_map<IntersectionIdx, std::unordered_map<IntersectionIdx, std::vector<StreetSegmentIdx>>> pathFromDeliveryLoc;

    //vector that contains all path from each Depot to any Delivery Pickup Location
    //std::unordered_map<std::vector<std::vector<StreetSegmentIdx>>> pathFromDepot;
    std::unordered_map<IntersectionIdx, std::unordered_map<IntersectionIdx, std::vector<StreetSegmentIdx>>> pathFromDepot;
    
    int numOfIntersectionsOfInterest = deliveries.size()*2 + depots.size() - 1;

    for (int i = 0; i < deliveries.size(); i++){
        multiDestDijkstra(intersections, deliveries[i].pickUp, importantIntersections, pathFromDeliveryLoc, pathFromDepot, numOfIntersectionsOfInterest, turn_penalty);
        multiDestDijkstra(intersections, deliveries[i].dropOff, importantIntersections, pathFromDeliveryLoc, pathFromDepot, numOfIntersectionsOfInterest, turn_penalty);
    }
    for (int i = 0; i < depots.size(); i++){
        multiDestDijkstra(intersections, depots[i], deliveries.size(), turn_penalty);
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


bool multiDestDijkstra(std::unordered_map<IntersectionIdx, Node*>& intersections, int startID, std::unordered_map<IntersectionIdx, int> importantIntersections,std::unordered_map<IntersectionIdx, std::unordered_map<IntersectionIdx, std::vector<StreetSegmentIdx>>> pathFromDeliveryLoc,
        std::unordered_map<IntersectionIdx, std::unordered_map<IntersectionIdx, std::vector<StreetSegmentIdx>>> pathFromDepot, int numOfIntersectionsOfInterest, double timePenalty){
    
    int interestedIntersectionCount = 0;
    
    //set a priority queue for the wave elements in the wavefront
    std::priority_queue <WaveElem, std::vector<WaveElem>> wavefront;
    
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
            
            
            auto it = importantIntersections.find(currNode->id);
            if(it != importantIntersections.end() && currNode.processed == false){
                currNode.processed = true;
                interestedIntersectionCount++;
            }
            
            //check whether path reached the destination intersection
            if(interestedIntersectionCount == numOfIntersectionsOfInterest){
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

                    //find the travel time of the segment
                    double travel_time = findStreetSegmentTravelTime(adjacentStreetSegments[i]);

                    //check whether the node's previous edge is not the starting edge
                    if (currNode->reachingEdge != NO_EDGE){
                        //if the previous edge and the current edge has the same streetID, don't apply the turn penalty and add to the wavefront
                        if (database.streetSegmentID_streetID[currNode->reachingEdge] == database.streetSegmentID_streetID[adjacentStreetSegments[i]]){
                            wavefront.push(WaveElem(it->second, adjacentStreetSegments[i], currNode->bestTime + travel_time));
                        }
                        //if the previous edge and the current edge do not have the same streetID, apply the turn penalty and add to the wavefront
                        else{
                            wavefront.push(WaveElem(it->second, adjacentStreetSegments[i], currNode->bestTime + travel_time + timePenalty));
                        }
                    }
                    //if it is a starting node
                    else{
                        wavefront.push(WaveElem(it->second, adjacentStreetSegments[i], currNode->bestTime + travel_time));                
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
}*/