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


//waveElems have nodes with directions on how they got here
struct WaveElem{
    Node *node;                 //node of wave element
    int edgeID;                 //id of segment used to get here
    double travelTime = 0;      //total time taken to reach node  
    double timeToDest = 0;      //estimated time to reach destination from current Node
    double totalTime = 0;       //total estimated time
    WaveElem (Node *n, int id, float travel_time, float dest_time){
        node = n;
        edgeID = id;
        travelTime = travel_time;
        timeToDest = dest_time;
        totalTime = travel_time + dest_time;
        
    }
};

class myComparator
{
public:
    int operator() (const WaveElem& wave1, const WaveElem& wave2)
    {
        return wave1.totalTime > wave2.totalTime;
    }
};


double computePathTravelTime(const std::vector<StreetSegmentIdx>& path, const double turn_penalty){
    int pathSize = path.size();
    double totalTime = 0;
    
    if(pathSize == 0){
        return 0;
    }

    for(int i = 0; i < pathSize; i++){
        totalTime += findStreetSegmentTravelTime(path[i]);
        
        if(i < pathSize - 1){
            if(getStreetSegmentInfo(path[i]).streetID != getStreetSegmentInfo(path[i+1]).streetID){
                totalTime += turn_penalty;
            }
        }
    }
    
    return totalTime;
}
//intersectionIDs are returned at (startIntersectionID, destIntersectionID)
//take startIntersectionID and destIntersectionID as start and finish

std::vector<std::pair<StreetSegmentIdx, IntersectionIdx> > validSegmentsAndIntersections(std::vector<StreetSegmentIdx> &segments, IntersectionIdx point){
    std::vector<std::pair<StreetSegmentIdx, IntersectionIdx> > legalSegmentsandIntersections;
    
    for(int i = 0; i < segments.size(); i++){
        StreetSegmentInfo street_segment = getStreetSegmentInfo(segments[i]);
        if(street_segment.from == point || street_segment.oneWay == false){
            std::pair<StreetSegmentIdx, IntersectionIdx> pairing;
                
            if (street_segment.from == point){
                pairing.first = segments[i];
                pairing.second = street_segment.to;
            }
                
            if (street_segment.to == point){
                pairing.first = segments[i];
                pairing.second = street_segment.from;
            }
                
            legalSegmentsandIntersections.push_back(pairing);
        }
    }
    
    return legalSegmentsandIntersections;
}


std::vector<StreetSegmentIdx> findPathBetweenIntersections(const IntersectionIdx intersect_id_start, 
const IntersectionIdx intersect_id_destination,const double turn_penalty){
    //create database of unordered map for intersections as nodes
    std::unordered_map<IntersectionIdx, Node*> intersections;
    
    //find the adjacent street segments of the start intersection
    std::vector<StreetSegmentIdx> adjacentSegments = findStreetSegmentsOfIntersection(intersect_id_start);
    //check whether the street segments are legal and store it in valid
    std::vector<std::pair<StreetSegmentIdx, IntersectionIdx> > valid = validSegmentsAndIntersections(adjacentSegments, intersect_id_start);
    Node* sourceNode = new Node(intersect_id_start, valid, 10000000000);
    
    intersections.insert({intersect_id_start, sourceNode});
    
    bool found = bfsPath(intersections, intersect_id_start, intersect_id_destination, turn_penalty);
    
    std::vector<StreetSegmentIdx> path;
    //return a path if a path exists between the intersections
    if(found){
        path = bfsTraceBack(intersections, intersect_id_destination);
    }
    
    //delete the unordered map
    for (int i = 0; i < intersections.size(); i++){
        delete intersections[i];
    }
    
    return path;
}

bool bfsPath(std::unordered_map<IntersectionIdx, Node*>& intersections, int startID, int destID, double timePenalty){
    //set a priority queue for the wave elements in the wavefront
    std::priority_queue <WaveElem, std::vector<WaveElem>, myComparator> wavefront;
    
    auto it = intersections.find(startID);
    
    std::pair<LatLon, LatLon> posPair (getIntersectionPosition(startID), getIntersectionPosition(destID));
    double distToDest = findDistanceBetweenTwoPoints(posPair);
    double timeToDest = distToDest/maxSpeed;
                
    wavefront.push(WaveElem(it->second, NO_EDGE, 0, timeToDest));
    /*ezgl::renderer *g;
    g->set_color(ezgl::BLUE);*/
    
    while(wavefront.size()!=0){
        //take the first in wavefront to be currentNode
        WaveElem wave = wavefront.top();  
        //remove the first element
        wavefront.pop();
        
        Node *currNode = wave.node;
        
        /*std::cout << "id: " << currNode->id << std::endl;
        std::cout << "time: " << currNode->bestTime << std::endl;
        std::cout << "wave time: " << wave.travelTime << std::endl;*/
        
        if (wave.travelTime < currNode->bestTime) {
            //check whether the bath was better to this node
            currNode->reachingEdge = wave.edgeID;         
            currNode->bestTime = wave.travelTime;
            
            //check whether path reached the destination intersection
            if(currNode->id == destID){
                return true;
            }
            
            for(int i = 0; i < currNode->legal.size(); i++){
                /*LatLon p1(getIntersectionPosition(currNode->id).latitude(), getIntersectionPosition(currNode->id).longitude());
                LatLon p2(getIntersectionPosition(currNode->legal[i].second).latitude(), getIntersectionPosition(currNode->legal[i].second).longitude());
                ezgl::point2d point1(x_from_lon(p1.longitude()), y_from_lat(p1.latitude()));
                ezgl::point2d point2(x_from_lon(p2.longitude()), y_from_lat(p2.latitude()));
                
                g->draw_line(point1, point2);*/
                
                it = intersections.find(currNode->legal[i].second);
                
                //check whether the node was visited or not(exists in the database or not)
                //add a node to the database(unordered map) if it wasn't visited
                if(it == intersections.end()){
                    std::vector<StreetSegmentIdx> adjacentSegments = findStreetSegmentsOfIntersection(currNode->legal[i].second);
                    std::vector<std::pair<StreetSegmentIdx, IntersectionIdx> > valid = validSegmentsAndIntersections(adjacentSegments, currNode->legal[i].second);
                    Node *toNode = new Node(currNode->legal[i].second, valid);
                   
                    intersections.insert({currNode->legal[i].second, toNode});
                }
                
                it = intersections.find(currNode->legal[i].second);
                
                //heuristics 
                std::pair<LatLon, LatLon> currentPosPair (getIntersectionPosition(currNode->legal[i].second), getIntersectionPosition(destID));
                double currentDistToDest = findDistanceBetweenTwoPoints(currentPosPair);
                double currentTimeToDest = currentDistToDest/maxSpeed;
                
                double travel_time = findStreetSegmentTravelTime(currNode->legal[i].first);
                          
                //check whether the node's previous edge is not the starting edge
                if (currNode->reachingEdge != NO_EDGE){
                    //if the previous edge and the current edge has the same streetID, don't apply the turn penalty and add to the wavefront
                    if (database.streetSegmentID_streetID[currNode->reachingEdge] == database.streetSegmentID_streetID[currNode->legal[i].first]){
                        wavefront.push(WaveElem(it->second, currNode->legal[i].first, currNode->bestTime + travel_time, currentTimeToDest));
                    }
                    //if the previous edge and the current edge do not have the same streetID, apply the turn penalty and add to the wavefront
                    else{
                        wavefront.push(WaveElem(it->second, currNode->legal[i].first, currNode->bestTime + travel_time + timePenalty, currentTimeToDest));
                    }
                }
                else{
                    wavefront.push(WaveElem(it->second, currNode->legal[i].first, currNode->bestTime + travel_time, currentTimeToDest));                
                }
            }
        }
    } 
    return false;
}

std::vector<StreetSegmentIdx> bfsTraceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID){
    std::vector<StreetSegmentIdx> pathToDest; 
    
    //find the Node with the destination intersection ID
    auto it = intersections.find(destID);
    Node *currNode = it->second;
    StreetSegmentIdx prevEdge = currNode->reachingEdge;
    
    while(prevEdge != NO_EDGE){
        //add the previous edge to the path
        pathToDest.push_back(prevEdge);
        
        StreetSegmentInfo street_segment = getStreetSegmentInfo(prevEdge);
        
        //set the next Node to the intersection that is on the other side of the street segment
        if(currNode->id == street_segment.from){
            auto k = intersections.find(street_segment.to);
            currNode = k->second;
        }
        
        else if(currNode->id == street_segment.to){
            auto k = intersections.find(street_segment.from);
            currNode = k->second;
        }
        
        //update the previous edge to the reaching edge
        prevEdge = currNode->reachingEdge;
    }
    
    //reverse the vector since it started from the destination
    std::reverse(pathToDest.begin(),pathToDest.end());
    
    return pathToDest;
}