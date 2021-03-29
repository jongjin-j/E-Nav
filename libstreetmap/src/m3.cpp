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
class Node;

struct outEdge{
    StreetSegmentIdx id;
    Node* toNode;
};

class Node{
public:
    IntersectionIdx id;
    std::vector<outEdge> outEdges;
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
    
    //loop through the intersections
    for(int i = 0; i < getNumIntersections(); i++){
        intersection_nodes[i].id = i;
        
        //loop through the street segments of the intersection
        for (int j = 0; j < getNumIntersectionStreetSegment(i); j++){
            
            StreetSegmentInfo street_segment = getStreetSegmentInfo(getIntersectionStreetSegment(i,j));
            
            if(street_segment.from == intersection_nodes[i].id || street_segment.oneWay == false){
                struct outEdge edge;
                
                edge.id = getIntersectionStreetSegment(i, j);
                
                if (street_segment.from == intersection_nodes[i].id){
                    edge.toNode->id = street_segment.to;
                }
                
                if (street_segment.to == intersection_nodes[i].id){
                    edge.toNode->id = street_segment.from;
                }
                
                intersection_nodes[i].outEdges.push_back(edge);
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
        
        for(int i = 0; i < curr.node->outEdges.size(); i++){
            
        }
    }
    
}
