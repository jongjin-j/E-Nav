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

#define NO_EDGE -1


//waveElems have nodes with directions on how they got here
struct WaveElem{
    Node *node;             //node of wave element
    int edgeID;             //id of segment used to get here
    double travelTime = 0;      //total time taken to reach node  
    double timeToDest = 0;
    double totalTime = 0;
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





//extern struct databases database;
//std::pair<LatLon,LatLon> fromToPoints;

/*Node* getNodeByID(IntersectionIdx ID){    
    auto it = intersections.find(ID);
    Node *currNode = it->second;
    return currNode;
}*/

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
    std::unordered_map<IntersectionIdx, Node*> intersections;
    
    std::vector<StreetSegmentIdx> adjacentSegments = findStreetSegmentsOfIntersection(intersect_id_start);
    std::vector<std::pair<StreetSegmentIdx, IntersectionIdx> > valid = validSegmentsAndIntersections(adjacentSegments, intersect_id_start);
    Node* sourceNode = new Node(intersect_id_start, valid, 10000000000);
    
    intersections.insert({intersect_id_start, sourceNode});
    
    bool found = bfsPath(intersections, intersect_id_start, intersect_id_destination, turn_penalty);
    
    std::vector<StreetSegmentIdx> path;
    if(found){
        path = bfsTraceBack(intersections, intersect_id_destination);
    }
    
    for (int i = 0; i < intersections.size(); i++){
        delete intersections[i];
    }
    
    //take turn penalty into account by checking street name
    return path;
}

bool bfsPath(std::unordered_map<IntersectionIdx, Node*>& intersections, int startID, int destID, double timePenalty){
    //std::list<WaveElem> wavefront;
    std::priority_queue <WaveElem, std::vector<WaveElem>, myComparator> wavefront;
    
    auto it = intersections.find(startID);
    
    std::pair<LatLon, LatLon> posPair (getIntersectionPosition(startID), getIntersectionPosition(destID));
    double distToDest = findDistanceBetweenTwoPoints(posPair);
    double timeToDest = distToDest/maxSpeed;
                
    wavefront.push(WaveElem(it->second, NO_EDGE, 0, timeToDest));
    
    while(wavefront.size()!=0){
        //make the wavefront into a heap
        //std::make_heap(wavefront.begin(), wavefront.end());
        
        //WaveElem wave = wavefront.front();      //take the first in wavefront to be currentNode
        WaveElem wave = wavefront.top();
        
        //wavefront.pop_front();                  //remove node from wavefront
        wavefront.pop();
        
        Node *currNode = wave.node;
        
        //std::cout << "id: " << currNode->id << std::endl;
        //std::cout << "time: " << currNode->bestTime << std::endl;
        //std::cout << "wave time: " << wave.travelTime << std::endl;
        
        //double turnTime = 0;
        /*
        if(currNode->turn == true){
            turnTime = timePenalty;
        }
        */
        if (wave.travelTime < currNode->bestTime) {
            // Was this a better path to this node? Update if so.
            currNode->reachingEdge = wave.edgeID;         
            currNode->bestTime = wave.travelTime;
            
            if(currNode->id == destID){
                return true;
            }
            
            for(int i = 0; i < currNode->legal.size(); i++){
                it = intersections.find(currNode->legal[i].second);
                
                if(it == intersections.end()){
                    std::vector<StreetSegmentIdx> adjacentSegments = findStreetSegmentsOfIntersection(currNode->legal[i].second);
                    std::vector<std::pair<StreetSegmentIdx, IntersectionIdx> > valid = validSegmentsAndIntersections(adjacentSegments, currNode->legal[i].second);
                    Node *toNode = new Node(currNode->legal[i].second, valid);
                   
                    
                    intersections.insert({currNode->legal[i].second, toNode});
                    //auto it = intersections.find(currNode->legal[i].second);
                }
                
                it = intersections.find(currNode->legal[i].second);

                //intersections.insert({currNode->legal[i].second, toNode});
                
                //heuristics 
                std::pair<LatLon, LatLon> posPair (getIntersectionPosition(currNode->legal[i].second), getIntersectionPosition(destID));
                double distToDest = findDistanceBetweenTwoPoints(posPair);
                double timeToDest = distToDest/maxSpeed;
                
                double travel_time = findStreetSegmentTravelTime(currNode->legal[i].first);
                          
                if (currNode->reachingEdge != NO_EDGE){
                    StreetSegmentInfo prev_segment = getStreetSegmentInfo(currNode->reachingEdge);
                    StreetSegmentInfo next_segment = getStreetSegmentInfo(currNode->legal[i].first);
                    
                    
                    if (prev_segment.streetID == next_segment.streetID){
                        wavefront.push(WaveElem(it->second, currNode->legal[i].first, currNode->bestTime + travel_time, timeToDest));
                    }
                    else{
                        wavefront.push(WaveElem(it->second, currNode->legal[i].first, currNode->bestTime + travel_time + timePenalty, timeToDest));

                    }
                }
                else{
                    wavefront.push(WaveElem(it->second, currNode->legal[i].first, currNode->bestTime + travel_time, timeToDest));                
                }
            }
        }
    } 
    return false;
}

std::vector<StreetSegmentIdx> bfsTraceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID){
    std::vector<StreetSegmentIdx> pathToDest; 
    
    auto it = intersections.find(destID);
    Node *currNode = it->second;
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
    
    std::reverse(pathToDest.begin(),pathToDest.end());
    
    return pathToDest;
}

/*std::vector<StreetSegmentIdx> bfsTraceBack(int destID){
    //vector stores the path to destination
    std::vector<StreetSegmentIdx> pathToDest; 
    
    Node* currNode = getNodeByID(destID);
    
    //prevEdge stores what segment was used to get to destID
    StreetSegmentIdx prevEdge = currNode->reachingEdge;
    //std::cout << prevEdge << std::endl;
    //return pathToDest;
    
    while(prevEdge!=NO_EDGE){
        //std::cout << prevEdge << std::endl;
        //if (prevEdge == 9043){
            //break;
        //}
        //add the segment used to get to destID
        pathToDest.push_back(prevEdge);
        //find the prevNode of the prevNode
        //store that into currNode
        StreetSegmentInfo ss_info = getStreetSegmentInfo(prevEdge);
        //std::cout << getStreetName(ss_info.streetID) << std::endl;
        
        if (currNode->id == ss_info.to){
            currNode = intersection_nodes[ss_info.from];
        }
        else if (currNode->id == ss_info.from){
            currNode = intersection_nodes[ss_info.to];
        }
        //obtain reaching edge and insert into vector
        prevEdge = currNode->reachingEdge;
    }

    //reverse vector
    std::reverse(pathToDest.begin(),pathToDest.end());
 
    return pathToDest;
}*/

