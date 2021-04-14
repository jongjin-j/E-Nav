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



bool multiDestDijkstra(int startIdx, int startID, int numOfImportantIntersections, std::vector<std::vector<double>>& pathTimes, std::unordered_map<IntersectionIdx, int> pickupIndexes, 
std::unordered_map<IntersectionIdx, int> dropoffIndexes, std::unordered_map<IntersectionIdx, int> depotIndexes, double timePenalty);

std::vector<StreetSegmentIdx> traceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID);



std::vector<CourierSubPath> travelingCourier(const std::vector<DeliveryInf>& deliveries, const std::vector<int>& depots, const float turn_penalty){
    std::vector<CourierSubPath> travelRoute;
    int N = deliveries.size();
    int M = depots.size();
    
    //database for all path times
    std::vector<std::vector<double>> timeForAllPaths;
    timeForAllPaths.resize(2 * N + M);
    for (int i = 0; i < 2 * N + M; ++i){
        timeForAllPaths[i].resize(2 * N + M);
    }
    
    for(int i = 0; i < 2 * N + M; i++){
        for(int j = 0; j < 2 * N + M; j++){
            timeForAllPaths[i][j] = initial_bestTime;
        }
    }
    

    //store the intersection indexes of the important intersections
    std::unordered_map<IntersectionIdx, int> pickupIndexes;
    std::unordered_map<IntersectionIdx, int> dropoffIndexes;
    std::unordered_map<IntersectionIdx, int> depotIndexes;
    
    
  //set up database for important intersections
    for (int i = 0; i < deliveries.size(); i++){
        pickupIndexes[deliveries[i].pickUp] = i;
        //auto it = pickupIndexes.find(deliveries[i].pickUp);
        //std::cout << it->first << std::endl;
    }
    for (int i = 0; i < deliveries.size(); i++){
        dropoffIndexes[deliveries[i].dropOff] = i;
    }
    for (int i = 0; i < depots.size(); i++){
        depotIndexes[depots[i]] = i;
    }  
    
    //travel time using Dijkstra
    for (int i = 0; i < deliveries.size(); i++){
        multiDestDijkstra(i, deliveries[i].pickUp, 2 * N + M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
        multiDestDijkstra(i + N, deliveries[i].dropOff, 2 * N + M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
    }

    for (int i = 0; i < depots.size(); i++){
        multiDestDijkstra(i + 2 * N, depots[i], 2 * N + M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
    }
    
    std::cout << "After Dijkstra" << std::endl;
    
    for(int i = 0; i < 2 * N + M; i++){
        for(int j = 0; j < 2 * N + M; j++){
            std::cout << timeForAllPaths[i][j] << " ";
        }
        std::cout << std::endl;
    }

    
    //set all points going to the same point as a big number
    /*for(int i = 0; i < 2 * N + M; i++){
        timeForAllPaths[i][i] = initial_bestTime;
        for(int j = 0; j < 2 * N + M; j++){
            if (timeForAllPaths[i][j] == 0){
                timeForAllPaths[i][j] = initial_bestTime;
            }
        }
    }*/
    for(int i = 0; i < 2 * N + M; i++){
        std::cout << "time: " << timeForAllPaths[i][i] << std::endl;
    }
    for(int i = 0; i < 2 * N + M; i++){
        timeForAllPaths[i][i] = initial_bestTime;
    }
    
    
    
    
    //Greedy Algorithm
    //need a data structure to keep track of visited index
    //1. loop through all depots -> find closest pickup point
    //2. loop through the pickup point -> find closest pickup or dropoff
    //3. loop until no pickups / dropoffs left
    //4. find closest depot
    
    //Step 1 of Algorithm
    int shortestTimeFromDepot = initial_bestTime;
    int startDepotIndex = 0;
    int firstPickupIndex = 0;
    
    std::unordered_map<int, bool> visitedIndex;
    
    for(int i = 2 * N; i < 2 * N + depots.size(); i++){
        for(int j = 0; j < N; j++){
            if(timeForAllPaths[i][j] < shortestTimeFromDepot){
                shortestTimeFromDepot = timeForAllPaths[i][j];
                startDepotIndex = i - 2 * N;
                firstPickupIndex = j;
            }
        }
    }
    
    std::unordered_map<IntersectionIdx, Node*> initialIntersections;
    Node* startNode = new Node(depots[startDepotIndex]);
    startNode -> bestTime = initial_bestTime;
    initialIntersections.insert({depots[startDepotIndex], startNode});
 
    bool initialPathFound = AStarPath(initialIntersections, depots[startDepotIndex], deliveries[firstPickupIndex].pickUp, turn_penalty);
    
    //later use function
    CourierSubPath firstPath;
    firstPath.start_intersection = depots[startDepotIndex];
    firstPath.end_intersection = deliveries[firstPickupIndex].pickUp;
    if(initialPathFound){
        firstPath.subpath = AStarTraceBack(initialIntersections, deliveries[firstPickupIndex].pickUp);
    }
    travelRoute.push_back(firstPath);
    //visitedIndex.insert({startDepotIndex, true});
    
    std::cout << "After First Depot" << std::endl;
    
    
    
    
    //Step 2 and 3 of Algorithm
    int nextIndex = firstPickupIndex;
    int currentIndex = firstPickupIndex;
    
    visitedIndex.insert({firstPickupIndex, true});
    
    for (auto it = pickupIndexes.begin(); it != pickupIndexes.end(); it++){
        if (it->first == deliveries[firstPickupIndex].pickUp){
            visitedIndex.insert({it->second, true});
        }
    }
    
    int travelCount = 0;
    
    //another pickup point that has not been visited OR drop-off point which the corresponding index pickup happened
    while(travelCount != 2 * N - 1){
        int shortestTime = initial_bestTime;
        
        std::unordered_map<IntersectionIdx, Node*> intersections;
        
        if(currentIndex < N){
            Node* sourceNode = new Node(deliveries[currentIndex].pickUp);
            sourceNode -> bestTime = initial_bestTime;
            intersections.insert({deliveries[currentIndex].pickUp, sourceNode});
        }
        else{
            Node* sourceNode = new Node(deliveries[currentIndex - N].dropOff);
            sourceNode -> bestTime = initial_bestTime;
            intersections.insert({deliveries[currentIndex - N].dropOff, sourceNode});
        }
        
        
        //currentIndex = nextIndex;
        std::vector<int> nextIndexes;
        
        for(int i = 0; i < 2 * N; i++){
            
            //case for visiting a pickup point that has not been visited
            if(i < N && visitedIndex.find(i) == visitedIndex.end() && timeForAllPaths[currentIndex][i] < shortestTime){
                shortestTime = timeForAllPaths[currentIndex][i];
                nextIndex = i;
            }
            
            //case for visiting a drop-off point which the corresponding index pickup point has been visited
            if(i >= N && visitedIndex.find(i - N) != visitedIndex.end() && visitedIndex.find(i) == visitedIndex.end() && timeForAllPaths[currentIndex][i] < shortestTime){
                shortestTime = timeForAllPaths[currentIndex][i];
                nextIndex = i;
            }
        }
        
        
        for(int i = 0; i < 2 * N; i++){
            
            if(i < N && visitedIndex.find(i) == visitedIndex.end() && nextIndex < N && deliveries[nextIndex].pickUp == deliveries[i].pickUp){
                nextIndexes.push_back(i);
            }
            
            if(i < N && visitedIndex.find(i) == visitedIndex.end() && nextIndex >= N && deliveries[nextIndex-N].dropOff == deliveries[i].pickUp){
                nextIndexes.push_back(i);
            }
            
            if(i >= N && visitedIndex.find(i - N) != visitedIndex.end() && visitedIndex.find(i) == visitedIndex.end() && nextIndex < N && deliveries[nextIndex].pickUp == deliveries[i-N].dropOff){
                nextIndexes.push_back(i);
            }
            
            if(i >= N && visitedIndex.find(i - N) != visitedIndex.end() && visitedIndex.find(i) == visitedIndex.end() && nextIndex >= N && deliveries[nextIndex-N].dropOff == deliveries[i-N].dropOff){
                nextIndexes.push_back(i);
            }
        }
        
        for (int i = 0; i < nextIndexes.size(); i++){
            visitedIndex.insert({nextIndexes[i], true});
            travelCount++;
        }
        
        std::cout << "Count: " << travelCount << std::endl;
        
        bool pathFound = false;

        if(currentIndex < N && nextIndex < N){
            pathFound = AStarPath(intersections, deliveries[currentIndex].pickUp, deliveries[nextIndex].pickUp, turn_penalty); 
        }
        if(currentIndex < N && nextIndex >= N){
            pathFound = AStarPath(intersections, deliveries[currentIndex].pickUp, deliveries[nextIndex - N].dropOff, turn_penalty); 
        }
        if(currentIndex >= N && nextIndex < N){
            pathFound = AStarPath(intersections, deliveries[currentIndex - N].dropOff, deliveries[nextIndex].pickUp, turn_penalty); 
        }
        if(currentIndex >= N && nextIndex >= N){
            pathFound = AStarPath(intersections, deliveries[currentIndex - N].dropOff, deliveries[nextIndex - N].dropOff, turn_penalty); 
        }
        
        
        
        //create CourierSubPath object to insert into vector
        CourierSubPath middlePath;
        if(currentIndex < N){
            middlePath.start_intersection = deliveries[currentIndex].pickUp;
        }
        else{
            middlePath.start_intersection = deliveries[currentIndex - N].dropOff;
        }
        if(nextIndex < N){
            middlePath.end_intersection = deliveries[nextIndex].pickUp;
        }
        else{
            middlePath.end_intersection = deliveries[nextIndex - N].dropOff;
        }
        
        if(pathFound){
            middlePath.subpath = AStarTraceBack(intersections, middlePath.end_intersection);
        }
        
        travelRoute.push_back(middlePath);
        
        
        //std::cout << currentIndex << std::endl;
        currentIndex = nextIndex;
      
    }
    
    for (auto it = visitedIndex.begin(); it != visitedIndex.end(); it++){
        std::cout << it->first << std::endl;
    }
    
    std::cout << "Before Last Depot" << std::endl;

    
    //Step 4 of Algorithm
    //currentIndex = nextIndex;
    
    //std::cout << currentIndex << std::endl;
    int shortestTimeToDepot = initial_bestTime;
    int lastDepotIndex = 0;
    
    for(int j = 2 * N; j < 2 * N + M; j++){
        if(timeForAllPaths[currentIndex][j] < shortestTimeToDepot){
            shortestTimeToDepot = timeForAllPaths[currentIndex][j];
            lastDepotIndex = j - 2 * N;
        }
    }
    
    std::unordered_map<IntersectionIdx, Node*> finalIntersections;
    Node* lastDropOff = new Node(deliveries[currentIndex - N].dropOff);
    lastDropOff -> bestTime = initial_bestTime;
    finalIntersections.insert({deliveries[currentIndex - N].dropOff, lastDropOff});
    
    bool pathFound = false;
 
    pathFound = AStarPath(finalIntersections, deliveries[currentIndex - N].dropOff, depots[lastDepotIndex], turn_penalty);
    
    CourierSubPath lastPath;
    lastPath.start_intersection = deliveries[currentIndex - N].dropOff;
    lastPath.end_intersection = depots[lastDepotIndex];
    if(pathFound){
        lastPath.subpath = AStarTraceBack(finalIntersections, depots[lastDepotIndex]);
    }
    
    travelRoute.push_back(lastPath);
    
    std::cout << "After Last Depot" << std::endl;
    
    return travelRoute;
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

bool multiDestDijkstra(int startIdx, int startID, int numOfImportantIntersections, std::vector<std::vector<double>>& pathTimes, std::unordered_map<IntersectionIdx, int> pickupIndexes, 
        std::unordered_map<IntersectionIdx, int> dropoffIndexes, std::unordered_map<IntersectionIdx, int> depotIndexes, double timePenalty){
    
    std::unordered_map<IntersectionIdx, Node*> intersections;
    Node* sourceNode = new Node(startID);
    sourceNode -> bestTime = initial_bestTime;
    intersections.insert({startID, sourceNode});
    
    int importantIntersectionCount = 0;
    
    //set a priority queue for the wave elements in the wavefront
    std::priority_queue <WaveElem, std::vector<WaveElem>, myComparator> wavefront;
    
    auto it = intersections.find(startID);
    
    //push in the first wave element
    wavefront.push(WaveElem(it->second, NO_EDGE, 0));
    
    //checking the wavefronts until it is empty
    while(wavefront.size()!=0){
        
        //take the first in wavefront to be currentNode
        WaveElem wave = wavefront.top();  
        //remove the first element
        wavefront.pop();
        
        Node *currNode = wave.node;
        
        //check if found a better path
        if (wave.travelTime < currNode->bestTime) {
            
            //update the reaching edge and best time
            currNode->reachingEdge = wave.edgeID;         
            currNode->bestTime = wave.travelTime;
            
            
            auto pickupIt = pickupIndexes.find(currNode->id);
            auto dropoffIt = dropoffIndexes.find(currNode->id);
            auto depotIt = depotIndexes.find(currNode->id);
            bool checkPickupProcessed = false;
            bool checkDropOffProcessed = false;

            
            if(pickupIt != pickupIndexes.end() && currNode->processed == false){
                currNode->processed = true;
                //checkPickupProcessed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                pathTimes[startIdx][pickupIt->second] = time;
                importantIntersectionCount++;
            }
            if(dropoffIt != dropoffIndexes.end() && currNode->processed == false){
                currNode->processed = true;
                //checkDropOffProcessed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                pathTimes[startIdx][pickupIndexes.size()+dropoffIt->second] = time;
                importantIntersectionCount++;
            }
            if(depotIt != depotIndexes.end() && currNode->processed == false){
                currNode->processed = true;
                //checkProcessed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                pathTimes[startIdx][pickupIndexes.size()+dropoffIndexes.size()+depotIt->second] = time;
                importantIntersectionCount++;
            }
            
            /*if (checkProcessed){
                currNode->processed = true;
            }*/
            
            
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
