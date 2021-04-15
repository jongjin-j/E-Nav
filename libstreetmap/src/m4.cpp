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
#include <map>
#include <algorithm>
#include <chrono>

#define NO_EDGE -1
//initial time has to be a large number
#define initial_bestTime 1000000000



bool multiDestDijkstra(int startIdx, int startID, int numOfImportantIntersections, std::vector<std::vector<double>>& pathTimes, std::multimap<IntersectionIdx, int> pickupIndexes, 
std::multimap<IntersectionIdx, int> dropoffIndexes, std::multimap<IntersectionIdx, int> depotIndexes, double timePenalty);

std::vector<StreetSegmentIdx> traceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID);



std::vector<CourierSubPath> travelingCourier(const std::vector<DeliveryInf>& deliveries, const std::vector<int>& depots, const float turn_penalty){
    
    //initialize timer at function call
    auto startTime = std::chrono::high_resolution_clock::now();
    //timeout is false until T = 45s
    bool timeOut = false;
    
    std::vector<CourierSubPath> finalTravelRoute;
    int bestTime = initial_bestTime;
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
    std::multimap<IntersectionIdx, int> pickupIndexes;
    std::multimap<IntersectionIdx, int> dropoffIndexes;
    std::multimap<IntersectionIdx, int> depotIndexes;
    
    
    
    //set up database for important intersections
    for (int i = 0; i < deliveries.size(); i++){
        pickupIndexes.insert(std::pair <IntersectionIdx, int>(deliveries[i].pickUp, i));
        //auto it = pickupIndexes.find(deliveries[i].pickUp);
        //std::cout << it->first << std::endl;
    }
    for (int i = 0; i < deliveries.size(); i++){
        dropoffIndexes.insert(std::pair<IntersectionIdx, int>(deliveries[i].dropOff, i));
    }
    for (int i = 0; i < depots.size(); i++){
        depotIndexes.insert(std::pair <IntersectionIdx, int> (depots[i],i));
    }  
    
    
    //travel time using Multi-Destination Dijkstra
    #pragma omp parallel for
    for (int i = 0; i < deliveries.size(); i++){
        multiDestDijkstra(i, deliveries[i].pickUp, 2 * N + M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
        multiDestDijkstra(i + N, deliveries[i].dropOff, 2 * N + M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
    }

    #pragma omp parallel for
    for (int i = 0; i < depots.size(); i++){
        multiDestDijkstra(i + 2 * N, depots[i], 2 * N + M, timeForAllPaths, pickupIndexes, dropoffIndexes, depotIndexes, turn_penalty);
    }
    
    std::cout << "After Dijkstra" << std::endl;
    
    //set all points going to the same point as a big number
    for(int i = 0; i < 2 * N + M; i++){
        for(int j = 0; j < 2 * N + M; j++){
            if (timeForAllPaths[i][j] == 0){
                timeForAllPaths[i][j] = initial_bestTime;
            }
        }
    }
    
    
    //Greedy Algorithm
    //need a data structure to keep track of visited index
    //1. loop through all depots -> find closest pickup point
    //2. loop through the pickup point -> find closest pickup or dropoff
    //3. loop until no pickups / dropoffs left
    //4. find closest depot
    
    
    //Step 1 of Algorithm
    //loop through all depots
    
    
    
    /*for(int i = 2 * N; i < 2 * N + depots.size(); i++){
        for(int j = 0; j < N; j++){
            if(timeForAllPaths[i][j] < shortestTimeFromDepot){
                shortestTimeFromDepot = timeForAllPaths[i][j];
                startDepotIndex = i - 2 * N;
                firstPickupIndex = j;
            }
        }
    }*/
    
    //algorithm start?
    //wrap while loop around two-opt later?
    while(!timeOut){
    
    std::vector<IntersectionIdx> finalPathIntersections;
    
    #pragma omp parallel for
    for(int depotIndex = 2 * N; depotIndex < 2 * N + depots.size(); depotIndex++){
        
        std::vector<IntersectionIdx> pathIntersections;
        double currentTime = 0;
        
        int shortestTimeFromDepot = initial_bestTime;
        int startDepotIndex = 0;
        int firstPickupIndex = 0;
        std::vector<CourierSubPath> travelRoute;
        
        for(int j = 0; j < N; j++){
            if(timeForAllPaths[depotIndex][j] < shortestTimeFromDepot){
                shortestTimeFromDepot = timeForAllPaths[depotIndex][j];
                startDepotIndex = depotIndex - 2 * N;
                firstPickupIndex = j;
            }
        }
        
        pathIntersections.push_back(depots[startDepotIndex]);
        currentTime += shortestTimeFromDepot;
        
        std::unordered_map<int, bool> visitedIndex;
    
        /*std::unordered_map<IntersectionIdx, Node*> initialIntersections;
        Node* startNode = new Node(depots[startDepotIndex]);
        startNode -> bestTime = initial_bestTime;
        initialIntersections.insert({depots[startDepotIndex], startNode});*/

        //bool initialPathFound = AStarPath(initialIntersections, depots[startDepotIndex], deliveries[firstPickupIndex].pickUp, turn_penalty);

        //later use function
        /*CourierSubPath firstPath;
        firstPath.start_intersection = depots[startDepotIndex];
        firstPath.end_intersection = deliveries[firstPickupIndex].pickUp;
        if(initialPathFound){
            firstPath.subpath = AStarTraceBack(initialIntersections, deliveries[firstPickupIndex].pickUp);
        }
        travelRoute.push_back(firstPath);*/

        /*for (int mapSize = 0; mapSize < initialIntersections.size(); mapSize++){
            delete initialIntersections[mapSize];
        }*/

        std::cout << "After First Depot" << std::endl;




        //Step 2 and 3 of Algorithm
        int nextIndex = firstPickupIndex;
        int currentIndex = firstPickupIndex;

        visitedIndex.insert({firstPickupIndex, true});

        int travelCount = 1;

        for (auto it = pickupIndexes.begin(); it != pickupIndexes.end(); it++){
            if (it->first == deliveries[firstPickupIndex].pickUp){
                visitedIndex.insert({it->second, true});
                travelCount++;
            }
        }

        
        //push back first pick up index
        //pathIntersections.push_back(deliveries[firstPickupIndex].pickUp);
        
        
        
        
        //another pickup point that has not been visited OR drop-off point which the corresponding index pickup happened
        while(travelCount != 2 * N + 1){
            double shortestTime = initial_bestTime;

            /*std::unordered_map<IntersectionIdx, Node*> intersections;

            if(currentIndex < N){
                Node* sourceNode = new Node(deliveries[currentIndex].pickUp);
                sourceNode -> bestTime = initial_bestTime;
                intersections.insert({deliveries[currentIndex].pickUp, sourceNode});
            }
            else{
                Node* sourceNode = new Node(deliveries[currentIndex - N].dropOff);
                sourceNode -> bestTime = initial_bestTime;
                intersections.insert({deliveries[currentIndex - N].dropOff, sourceNode});
            }*/

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


            /*bool pathFound = false;

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
            }*/
            
            
            //update path and time
            if (currentIndex < N){
                pathIntersections.push_back(deliveries[currentIndex].pickUp);
            }
            if (currentIndex >= N){
                pathIntersections.push_back(deliveries[currentIndex-N].dropOff);
            }
            currentTime += shortestTime;



            //create CourierSubPath object to insert into vector
            /*CourierSubPath middlePath;
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

            travelRoute.push_back(middlePath);*/


            currentIndex = nextIndex;

            /*for (int i = 0; i < intersections.size(); i++){
                delete intersections[i];
            }*/

        }

        pathIntersections.push_back(deliveries[nextIndex-N].dropOff);
        
        
        /*for (auto it = visitedIndex.begin(); it != visitedIndex.end(); it++){
            std::cout << it->first << std::endl;
        }*/



        //Step 4 of Algorithm

        int shortestTimeToDepot = initial_bestTime;
        int lastDepotIndex = 0;

        for(int j = 2 * N; j < 2 * N + M; j++){
            if(timeForAllPaths[currentIndex][j] < shortestTimeToDepot){
                shortestTimeToDepot = timeForAllPaths[currentIndex][j];
                lastDepotIndex = j - 2 * N;
            }
        }

        
        pathIntersections.push_back(depots[lastDepotIndex]);
        currentTime += shortestTimeToDepot;
        
        /*std::unordered_map<IntersectionIdx, Node*> finalIntersections;
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

        travelRoute.push_back(lastPath);*/

        /*for (int mapSize = 0; mapSize < finalIntersections.size(); mapSize++){
            delete finalIntersections[mapSize];
        }*/
        
        
        
        #pragma omp critical
        if(currentTime < bestTime){
            //finalPathIntersections.clear();
            finalPathIntersections = pathIntersections;
            bestTime = currentTime;
            
            //finalTravelRoute = travelRoute;
        }
        
    }
    //implement two opt
    //end of algorithm
    //close bracket for timeOut
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto wallClock = std::chrono_duration_cast<std::chrono::seconds>(currentTime - startTime);
        if(wallClock.count() > 45){
            timeOut = true;
        }
    }
    
    
    //update the best time - using critical
    
    
    
    std::cout << "After Last Depot" << std::endl;
    
    for (int i = 0; i < finalPathIntersections.size() - 1; i++){
        CourierSubPath path;
        path.start_intersection = finalPathIntersections[i];
        path.end_intersection = finalPathIntersections[i+1];
        
        std::unordered_map<IntersectionIdx, Node*> intersections;
        Node* lastDropOff = new Node(finalPathIntersections[i]);
        lastDropOff -> bestTime = initial_bestTime;
        intersections.insert({finalPathIntersections[i], lastDropOff});

        AStarPath(intersections, finalPathIntersections[i], finalPathIntersections[i+1], turn_penalty);
        path.subpath = AStarTraceBack(intersections, finalPathIntersections[i+1]);
        finalTravelRoute.push_back(path);
        
        for (int j = 0; j < intersections.size(); j++){
            delete intersections[j];
        }
    }
    
    
        
    //return travelRoute;
    return finalTravelRoute;
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
class myComparator2
{
public:
    int operator() (const WaveElem& wave1, const WaveElem& wave2)
    {
        //compares the total time
        return wave1.travelTime > wave2.travelTime;
    }
};

bool multiDestDijkstra(int startIdx, int startID, int numOfImportantIntersections, std::vector<std::vector<double>>& pathTimes, std::multimap<IntersectionIdx, int> pickupIndexes, 
        std::multimap<IntersectionIdx, int> dropoffIndexes, std::multimap<IntersectionIdx, int> depotIndexes, double timePenalty){
    
    std::unordered_map<IntersectionIdx, Node*> intersections;
    Node* sourceNode = new Node(startID);
    sourceNode -> bestTime = initial_bestTime;
    intersections.insert({startID, sourceNode});
    
    int importantIntersectionCount = 0;
    
    //set a priority queue for the wave elements in the wavefront
    std::priority_queue <WaveElem, std::vector<WaveElem>, myComparator2> wavefront;
    
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
            bool checkProcessed = false;

            typedef std::multimap<IntersectionIdx, int>::iterator map_iterator;
            
            if(pickupIt != pickupIndexes.end() && currNode->processed == false){
                checkProcessed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                //pathTimes[startIdx][pickupIt->second] = time;
                
                std::pair<map_iterator, map_iterator> result = pickupIndexes.equal_range(currNode->id);
                for (map_iterator map_it = result.first; map_it != result.second; map_it++){
                    pathTimes[startIdx][map_it->second] = time;
                    importantIntersectionCount++;
                }
                    
                //importantIntersectionCount++;
            }
            if(dropoffIt != dropoffIndexes.end() && currNode->processed == false){
                checkProcessed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                //pathTimes[startIdx][pickupIndexes.size()+dropoffIt->second] = time;
                
                std::pair<map_iterator, map_iterator> result = dropoffIndexes.equal_range(currNode->id);
                for (map_iterator map_it = result.first; map_it != result.second; map_it++){
                    pathTimes[startIdx][pickupIndexes.size()+map_it->second] = time;
                    importantIntersectionCount++;
                }
                
                //importantIntersectionCount++;
            }
            if(depotIt != depotIndexes.end() && currNode->processed == false){
                currNode->processed = true;
                //checkProcessed = true;
                
                //move the path vector into the 3d database
                std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                double time = computePathTravelTime(path, timePenalty);
                //pathTimes[startIdx][pickupIndexes.size()+dropoffIndexes.size()+depotIt->second] = time;
                
                std::pair<map_iterator, map_iterator> result = depotIndexes.equal_range(currNode->id);
                for (map_iterator map_it = result.first; map_it != result.second; map_it++){
                    pathTimes[startIdx][pickupIndexes.size()+dropoffIndexes.size()+map_it->second] = time;
                    importantIntersectionCount++;
                }
                
                //importantIntersectionCount++;
            }
            
            if (checkProcessed){
                currNode->processed = true;
            }
            
            
            //check whether path reached the destination intersection
            if(importantIntersectionCount == numOfImportantIntersections){
                for(int i = 0; i < intersections.size(); i++){
                    delete intersections[i];
                }
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
    if (pathToDest.size() != 0){
        std::reverse(pathToDest.begin(),pathToDest.end());
        return pathToDest;
    }
    
    return std::vector<StreetSegmentIdx>();
}
