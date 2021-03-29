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

#define NO_EDGE -1

//outedges are street segments with pointers to the nodes they go to
struct outEdge{
    StreetSegmentIdx id;               
    Node* toNode;
};

//nodes have reaching edges and outgoing edges
class Node{
public:
    IntersectionIdx id;
    std::vector<outEdge> outEdges;      //outgoing segments of current node
    StreetSegmentIdx reachingEdge;      //segment used to get here
};

//waveElems have nodes with directions on how they got here
struct WaveElem{
    Node *node;     //node of wave element
    int edgeID;     //id of segment used to get here
    WaveElem (Node *n, int id){
        node = n;
        edgeID = id;
    }
};

std::vector<Node> intersection_nodes;
std::pair<LatLon,LatLon> fromToPoints;

double computePathTravelTime(const std::vector<StreetSegmentIdx>& path, const double turn_penalty){
    int pathSize = path.size();
    double totalTime = 0;
    double distance = 0;
    
    if(pathSize == 0){
        return 0;
    }
    
    for(int i=0; i<pathSize; i++){
        
        fromToPoints = std::make_pair(getIntersectionPosition(getStreetSegmentInfo(path[i]).from),getIntersectionPosition(getStreetSegmentInfo(path[i]).to));
        
        distance = findDistanceBetweenTwoPoints(fromToPoints);
                
        totalTime += (distance / getStreetSegmentInfo(path[i]).speedLimit);
        
        if(i<pathSize-1){
            if(getStreetSegmentInfo(path[i]).streetID != getStreetSegmentInfo(path[i+1]).streetID){
                totalTime += turn_penalty;
            }
        }
        
    }
    return totalTime;
    
}
//intersectionIDs are returned at (startIntersectionID, destIntersectionID)
//take startIntersectionID and destIntersectionID as start and finish

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
    std::list<WaveElem> wavefront;  //stores the next set of nodes to be sweeped
    wavefront.push_back(WaveElem(sourceNode, NO_EDGE)); //initialize with source node
    
    while(wavefront.size()!=0){
        WaveElem curr = wavefront.front();      //take the first in wavefront to be currentNode
        wavefront.pop_front();                  //remove node from wavefront
        curr.node->reachingEdge = curr.edgeID;  //segID used to get here (-1 for source node)
        
        if(curr.node->id == destID){
            return true;
        }
        
        for(int i = 0; i < curr.node->outEdges.size(); i++){
            Node* toNode = curr.node->outEdges[i].toNode;                    
            wavefront.push_back(WaveElem(toNode,curr.node->outEdges[i].id)); 
        }
    } 
    return false;
    
}

std::vector<StreetSegmentIdx> bfsTraceBack(int destID){
    //vector stores the path to destination
    std::vector<StreetSegmentIdx> pathToDest;            
    Node* currNode;
    
    //find the destinationID
    for(int i=0; i<getNumIntersections();i++){
        if(intersection_nodes[i].id == destID){
            //if match, destID = currentID
            *currNode = intersection_nodes[i];
        } 
    }
    //prevEdge stores what segment was used to get to destID
    StreetSegmentIdx prevEdge = currNode->reachingEdge;
    
    while(prevEdge!=NO_EDGE){
        //add the segment used to get to destID
        pathToDest.push_back(prevEdge);
        //find the prevNode of the prevNode
        //store that into currNode
        *currNode = intersection_nodes[getStreetSegmentInfo(prevEdge).from];
        //obtain reaching edge and insert into vector
        prevEdge = currNode->reachingEdge;
    }

    //reverse vector
    std::reverse(path.begin(),path.end());
 
    return pathToDest;
}
