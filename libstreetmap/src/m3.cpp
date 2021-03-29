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
#include "m3.h"

#define NO_EDGE -1

class Node{
public:
    IntersectionIdx id;
    std::vector<StreetSegmentIdx> outgoingEdges;
    StreetSegmentIdx reachingEdge;
};

struct WaveElem{
    Node *node;
    int edgeID;
    WaveElem (Node *n, int id){
        node = n;
        edgeID = id;
    }
};

std::vector<Node> intersection_nodes;

double computePathTravelTime(const std::vector<StreetSegmentIdx>& path, const double turn_penalty){
    
}


std::vector<StreetSegmentIdx> findPathBetweenIntersections(const IntersectionIdx intersect_id_start, 
const IntersectionIdx intersect_id_destination,const double turn_penalty){
       
    //initializing database for intersection nodes
    for(int i = 0; i < getNumIntersections(); i++){
        intersection_nodes[i].id = i;
        intersection_nodes[i].outgoingEdges = findStreetSegmentsOfIntersection(i);
        
        for(int j = 0; j < intersection_nodes[i].outgoingEdges.size(); j++){
            StreetSegmentInfo street_segment = getStreetSegmentInfo(intersection_nodes[i].outgoingEdges[j]);
            if(street_segment.to == intersection_nodes[i].id && street_segment.oneWay){
                intersection_nodes[i].outgoingEdges.erase(intersection_nodes[j].outgoingEdges.begin() + j);
            }
            
        }
        
    }
    
    
}

bool bfsPath(Node* sourceNode, int destID){
    std::list<WaveElem> wavefront;
    wavefront.push_back(WaveElem(sourceNode, NO_EDGE));
    
    while(1){
        WaveElem curr = wavefront.front();
        wavefront.pop_front();
        curr.node->reachingEdge = curr.edgeID;
        
        if(curr.node->id == destID){
            return true;
        }
        
        for(int i = 0; i < curr.node->outgoingEdges.size(); i++){
            
        }
    }
    
}
