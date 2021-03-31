/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


/*#include <iostream>
#include "m1.h"
#include "globals.h"
#include "StreetsDatabaseAPI.h"
#include <string>
#include <string.h>
#include <list>
#include "m2.h"
#include "m3.h"

#define NO_EDGE -1


class Nodes{
public:
    IntersectionIdx id;
    std::vector<std::pair<StreetSegmentIdx, IntersectionIdx>> legal;
    Nodes(int inter_id, std::vector<std::pair<int, int>> vec){
        id = inter_id;
        legal = vec;
    }
    StreetSegmentIdx reachingEdge;
    double bestTime = 10000000;
};

struct WaveElem{
    Nodes* node;  
    int edgeID;             
    double travelTime = 0;         
    WaveElem(Nodes* n, int id, float time){
        node = n;
        edgeID = id;
        travelTime = time;
    }
};


double computPathTravelTime(const std::vector<StreetSegmentIdx>& path, const double turn_penalty){
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

std::vector<std::pair<StreetSegmentIdx, IntersectionIdx>> validSegmentsAndIntersections(std::vector<StreetSegmentIdx> segments, IntersectionIdx point){
    std::vector<std::pair<StreetSegmentIdx, IntersectionIdx>> legalSegmentsandIntersections;
    
    for(int i = 0; i < segments.size(); i++){
        StreetSegmentInfo street_segment = getStreetSegmentInfo(segments[i]);
        if(street_segment.from == point || street_segment.oneWay == false){
            std::pair<StreetSegmentIdx, IntersectionIdx> pairing;
                
            if (street_segment.from == i){
                pairing.first = segments[i];
                pairing.second = street_segment.to;
            }
                
            if (street_segment.to == i){
                pairing.first = segments[i];
                pairing.second = street_segment.from;
            }
                
            legalSegmentsandIntersections.push_back(pairing);
        }
    }
    
    return legalSegmentsandIntersections;
}

std::map<IntersectionIdx, Nodes*> intersections;
bool bfsPaths(int startID, int destID);
std::vector<StreetSegmentIdx> TraceBack(int destID);

std::vector<StreetSegmentIdx> findPathBetweenIntersection(const IntersectionIdx intersect_id_start, 
const IntersectionIdx intersect_id_destination,const double turn_penalty){

    std::vector<StreetSegmentIdx> adjacentSegments = findStreetSegmentsOfIntersection(intersect_id_start);
    std::vector<std::pair<StreetSegmentIdx, IntersectionIdx>> valid = validSegmentsAndIntersections(adjacentSegments, intersect_id_start);
    Nodes* sourceNode;
    sourceNode->id = intersect_id_start;
    sourceNode->legal = valid;
    intersections.insert({intersect_id_start, sourceNode});
    
    bool found = bfsPaths(intersect_id_start, intersect_id_destination);
    
    std::vector<StreetSegmentIdx> path;
    if(found){
        path = TraceBack(intersect_id_destination);
    }
    
    //take turn penalty into account by checking street name
    return path;
}


bool bfsPaths(int startID, int destID){
    std::list<WaveElem> wavefront;
    auto it = intersections.find(startID);
    wavefront.push_back(WaveElem(it->second, NO_EDGE, 20)); 
    
    while(wavefront.size()!=0){
        WaveElem wave = wavefront.front();  
        wavefront.pop_front();   
        
        Nodes* currNode = wave.node;
        
        if (wave.travelTime < currNode->bestTime) {
            currNode->reachingEdge = wave.edgeID;         
            currNode->bestTime = wave.travelTime;
            
            if(currNode->id == destID){
                return true;
            }   
            
            for(int i = 0; i < currNode->legal.size(); i++){
                std::vector<StreetSegmentIdx> adjacentSegments = findStreetSegmentsOfIntersection(currNode->id);
                std::vector<std::pair<StreetSegmentIdx, IntersectionIdx>> valid = validSegmentsAndIntersections(adjacentSegments, currNode->id);
                Nodes *toNode;
                toNode->id = currNode->legal[i].second;
                toNode->legal = valid;             
                wavefront.push_back(WaveElem(toNode, currNode->legal[i].second, currNode->bestTime + findStreetSegmentTravelTime(currNode->legal[i].first)));
            }
        }
    } 
    return false;
}

std::vector<StreetSegmentIdx> TraceBack(int destID){
    std::vector<StreetSegmentIdx> pathToDest; 
    
    auto it = intersections.find(destID);
    Nodes *currNode = it->second;
    StreetSegmentIdx prevEdge = currNode->reachingEdge;
    
    while(prevEdge != NO_EDGE){
        pathToDest.push_back(prevEdge);
        
        StreetSegmentInfo street_segment = getStreetSegmentInfo(prevEdge);
        
        if(currNode->id == street_segment.from){
            auto k = intersections.find(street_segment.to);
            currNode = k->second;
        }
        
        else if(currNode->id == street_segment.to){
            auto k = intersections.find(street_segment.from);
            currNode = k->second;
        }
        
        prevEdge = currNode->reachingEdge;
    }
    
    return pathToDest;
}*/