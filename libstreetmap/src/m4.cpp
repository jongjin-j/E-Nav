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
#include <stdlib.h>
#include <time.h>


#define NO_EDGE -1
//initial time has to be a large number
#define initial_bestTime 1000000000



bool multiDestDijkstra(int startIdx, int startID, int numOfImportantIntersections, std::vector<std::vector<double>>& pathTimes, std::multimap<IntersectionIdx, int> pickupIndexes, 
std::multimap<IntersectionIdx, int> dropoffIndexes, std::multimap<IntersectionIdx, int> depotIndexes, double timePenalty);

std::vector<StreetSegmentIdx> traceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID);

bool pathLegal(std::vector<int> allPathIndex);
void swapThreeOrder(std::vector<int>& vec1, std::vector<int>& vec2, std::vector<int>& vec3, std::vector<int>& finalPath, int order);
void reversePartialVector(std::vector<int>& vec1, std::vector<int>& vec2, std::vector<int>& vec3, int order);
double findPathTravelTime(std::vector<int> intersections, std::vector<std::vector<double>> timeForPaths);
void swapFourOrder(std::vector<int>& vec1, std::vector<int>& vec2, std::vector<int>& vec3, std::vector<int>& vec4, std::vector<int>& finalPath, int order);
void reversePartialVector3(std::vector<int>& vec1, std::vector<int>& vec2, std::vector<int>& vec3, std::vector<int>& vec4, int order);

std::vector<CourierSubPath> travelingCourier(const std::vector<DeliveryInf>& deliveries, const std::vector<int>& depots, const float turn_penalty){
    
    srand(time(NULL));
    
    
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
    
    
    //std::cout << "After Dijkstra" << std::endl;
    //auto endTime1 = std::chrono::high_resolution_clock::now();
    //auto elapsedTime1 = std::chrono::duration_cast<std::chrono::seconds>(endTime1-startTime).count();
    //std::cout << elapsedTime1 << std::endl;
    
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
    //while(!timeOut){
    
    std::vector<IntersectionIdx> finalPathIntersections;
    std::vector<int> finalPathIndexes;
    
    
    double finalTimeFromDepot = initial_bestTime;
    double finalTimeToDepot = initial_bestTime;
    
    #pragma omp parallel for
    for(int pickUpIndex = 0; pickUpIndex < N; pickUpIndex++){
        
        //std::vector<IntersectionIdx> pathIntersections;
        std::vector<int> pathIndexes;
        double currentTime = 0;
        
        /*int shortestTimeFromDepot = initial_bestTime;
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
        
        //pathIntersections.push_back(depots[startDepotIndex]);
        currentTime += shortestTimeFromDepot;*/
        
        std::unordered_map<int, bool> visitedIndex;
    
        //std::cout << "After First Depot" << std::endl;




        //Step 2 and 3 of Algorithm
        int nextIndex = pickUpIndex;
        int currentIndex = pickUpIndex;

        visitedIndex.insert({pickUpIndex, true});
        pathIndexes.push_back(pickUpIndex);
        

        int travelCount = 1;

        for (auto it = pickupIndexes.begin(); it != pickupIndexes.end(); it++){
            if (it->first == deliveries[pickUpIndex].pickUp){
                visitedIndex.insert({it->second, true});
                travelCount++;
            }
        }

        
        //another pickup point that has not been visited OR drop-off point which the corresponding index pickup happened
        while(travelCount != 2 * N + 1){
            double shortestTime = initial_bestTime;
            
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
                pathIndexes.push_back(nextIndexes[i]);
                travelCount++;
            }

            //update path and time
            /*if (currentIndex < N){
                pathIntersections.push_back(deliveries[currentIndex].pickUp);
            }
            if (currentIndex >= N){
                pathIntersections.push_back(deliveries[currentIndex-N].dropOff);
            }*/
            currentTime += shortestTime;

            //pathIndexes.push_back(currentIndex);

            currentIndex = nextIndex;
        }

        //pathIntersections.push_back(deliveries[nextIndex-N].dropOff);
        //pathIndexes.push_back(nextIndex);
        
        
        /*for (auto it = visitedIndex.begin(); it != visitedIndex.end(); it++){
            std::cout << it->first << std::endl;
        }*/



        //Step 4 of Algorithm

        /*int shortestTimeToDepot = initial_bestTime;
        int lastDepotIndex = 0;

        for(int j = 2 * N; j < 2 * N + M; j++){
            if(timeForAllPaths[currentIndex][j] < shortestTimeToDepot){
                shortestTimeToDepot = timeForAllPaths[currentIndex][j];
                lastDepotIndex = j - 2 * N;
            }
        }*/
            
        
        
        //pathIntersections.push_back(depots[lastDepotIndex]);
        //currentTime += shortestTimeToDepot;
        
        
        #pragma omp critical
        if(currentTime < bestTime){
            //finalPathIntersections = pathIntersections;
            //finalTimeFromDepot = shortestTimeFromDepot;
            //finalTimeToDepot = shortestTimeToDepot;
            bestTime = currentTime;
            finalPathIndexes = pathIndexes;
        }
        
    }
    
    //std::cout << "end of greedy" << std::endl;
    //auto endTime2 = std::chrono::high_resolution_clock::now();
    //auto elapsedTime2 = std::chrono::duration_cast<std::chrono::seconds>(endTime2-startTime).count();
    //std::cout << elapsedTime2 << std::endl;
    
    //implement two opt
    //end of algorithm
    //close bracket for timeOut
    
    /*auto currentTime = std::chrono::high_resolution_clock::now();
    auto wallClock = std::chrono_duration_cast<std::chrono::seconds>(currentTime - startTime);
        if(wallClock.count() > 45){
            timeOut = true;
        }
    }*/
    
    
    //update the best time - using critical
    
    /*for(int j = 0; j < finalPathIndexes.size(); j++){
        std::cout << finalPathIndexes[j] << std::endl;
    }*/
    
    //we already have bestTime, can compare to this variable
    
    int pathIndexesSize = finalPathIndexes.size();
    
    bool endOfTwoOpt = false;
    
    
    //double T = 300;
    //int x = 15000;
    double currBestTime = initial_bestTime;
    
    
    while(!timeOut && !endOfTwoOpt){
        currBestTime = bestTime;
        //std::cout << "count " << bestTime << std::endl;
        #pragma omp parallel for
        for(int i = 1; i < pathIndexesSize - 1; i++){
            if (!timeOut){
                for(int j = i + 1; j < pathIndexesSize; j++){
                    if (!timeOut){
                    
                        ////ERROR FREE
                        std::vector<int> indexesOfFirstSegment;
                        std::vector<int> indexesOfMidSegment;
                        std::vector<int> indexesOfLastSegment;

                        indexesOfFirstSegment.resize(i);
                        indexesOfMidSegment.resize(j-i);
                        indexesOfLastSegment.resize(pathIndexesSize-j);



                        for (int k = 0; k < i; k++){
                            indexesOfFirstSegment[k] = finalPathIndexes[k];
                        }
                        for (int n = 0; n < j-i; n++){
                            indexesOfMidSegment[n] = finalPathIndexes[n+i];
                        }
                        for (int l = 0; l < pathIndexesSize-j; l++){
                            indexesOfLastSegment[l] = finalPathIndexes[l+j];
                        }



                        for(int reverse = 0; reverse < 8; reverse++){
                            for(int swap = 0; swap < 6; swap++){
                                std::vector<int> vec1 = indexesOfFirstSegment;
                                std::vector<int> vec2 = indexesOfMidSegment;
                                std::vector<int> vec3 = indexesOfLastSegment;
                                std::vector<int> path;

                                reversePartialVector(vec1, vec2, vec3, reverse);
                                swapThreeOrder(vec1, vec2, vec3, path, swap);

                                #pragma omp critical
                                if(pathLegal(path)){
                                    double curTime = findPathTravelTime(path, timeForAllPaths);

                                    //double random = (rand()%100);
                                    //double randNum = random/100;
                                    double timeDiff = curTime - bestTime;
                                    //#pragma omp critical
                                    if(timeDiff < 0 /*|| randNum < exp(-1 * timeDiff / T)*/){
                                        finalPathIndexes = path;
                                        bestTime = curTime;
                                    }

                                    //reduce T /////////////////////////////////////
                                    //auto currTime = std::chrono::high_resolution_clock::now();
                                    //auto elapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(currTime-startTime).count();
                                    //T = T - (2*T/x)*((43.5 - elapsedTime) / 100);
                                    //std::cout << "time: " << elapsedTime << std::endl;
                                    //std::cout << "Temp: " << T << " " << bestTime << " " << elapsedTime << std::endl;
                                }
                            }
                        }
                    }
                    auto endTime = std::chrono::high_resolution_clock::now();
                    auto elapsedTime2 = std::chrono::duration_cast<std::chrono::seconds>(endTime-startTime).count();
                    //std::cout << elapsedTime2 << std::endl;
                    if (elapsedTime2 > 43.5){
                        timeOut = true;
                        break;
                    }
                }
            }
            if (currBestTime == bestTime){
                endOfTwoOpt = true;
            }
        }
    }
    
    
        
    /*for(int j = 0; j < finalPathIndexes.size(); j++){
        std::cout << finalPathIndexes[j] << std::endl;
    }*/
    
    /*auto endTwoOptTime = std::chrono::high_resolution_clock::now();
    auto elapsedTwoOptTime = std::chrono::duration_cast<std::chrono::double>(endTwoOptTime-startTime).count();
    if (elapsedTwoOptTime > 43.5){
        timeOut = true;
    }*/
    //timeOut = true;
    /*if (timeOut){
        std::cout << "timeout2" << std::endl;
    }*/
    
    
    
    
    //3-opt
    bool endOfThreeOpt = false;
    double currBestTimeThreeOpt = initial_bestTime;
    
    while(!timeOut && !endOfThreeOpt){
        currBestTimeThreeOpt = bestTime;
        //std::cout << "count 3: " << bestTime << std::endl;
        #pragma omp parallel for
        for(int i = 1; i < pathIndexesSize - 2; i++){
            if (!timeOut){
                for(int j = i + 1; j < pathIndexesSize - 1; j++){
                    if (!timeOut){
                        for(int k = j + 1; k < pathIndexesSize; k++){
                            if (!timeOut){
                                ////ERROR FREE
                                std::vector<int> indexesOfFirstSegment;
                                std::vector<int> indexesOfSecondSegment;
                                std::vector<int> indexesOfThirdSegment;
                                std::vector<int> indexesOfLastSegment;

                                indexesOfFirstSegment.resize(i);
                                indexesOfSecondSegment.resize(j-i);
                                indexesOfThirdSegment.resize(k-j);
                                indexesOfLastSegment.resize(pathIndexesSize-k);



                                for (int m = 0; m < i; m++){
                                    indexesOfFirstSegment[m] = finalPathIndexes[m];
                                }
                                for (int n = 0; n < j-i; n++){
                                    indexesOfSecondSegment[n] = finalPathIndexes[n+i];
                                }
                                for (int l = 0; l < k-j; l++){
                                    indexesOfThirdSegment[l] = finalPathIndexes[l+j];
                                }
                                for (int w = 0; w < pathIndexesSize-k; w++){
                                    indexesOfLastSegment[w] = finalPathIndexes[w+k];
                                }


                                for(int reverse = 0; reverse < 16; reverse++){
                                    for(int swap = 0; swap < 24; swap++){
                                        std::vector<int> vec1 = indexesOfFirstSegment;
                                        std::vector<int> vec2 = indexesOfSecondSegment;
                                        std::vector<int> vec3 = indexesOfThirdSegment;
                                        std::vector<int> vec4 = indexesOfLastSegment;
                                        std::vector<int> path;

                                        reversePartialVector3(vec1, vec2, vec3, vec4, reverse);
                                        swapFourOrder(vec1, vec2, vec3, vec4, path, swap);

                                        #pragma omp critical
                                        if(pathLegal(path)){
                                            double curTime = findPathTravelTime(path, timeForAllPaths);


                                            double timeDiff = curTime - bestTime;
                                            //#pragma omp critical
                                            if(timeDiff < 0){
                                                finalPathIndexes = path;
                                                bestTime = curTime;
                                            }
                                        }
                                    }
                                }
                            }
                            auto endTime3 = std::chrono::high_resolution_clock::now();
                            auto elapsedTime3 = std::chrono::duration_cast<std::chrono::seconds>(endTime3-startTime).count();
                            //std::cout << elapsedTime3 << std::endl;
                            if (elapsedTime3 > 44){
                                timeOut = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (currBestTimeThreeOpt == bestTime){
            endOfThreeOpt = true;
        }
    }

    
    
    
    
    
    
    int shortestTimeFromDepot = initial_bestTime;
    int fromDepotIntersection;
    
    
    
    
    for(int j = 2 * N; j < 2 * N + M; j++){
        if(timeForAllPaths[j][finalPathIndexes[0]] < shortestTimeFromDepot){
            shortestTimeFromDepot = timeForAllPaths[j][finalPathIndexes[0]];
            fromDepotIntersection = depots[j - 2 * N];
        }
    }
    
    
    
    int shortestTimeToDepot = initial_bestTime;
    int toDepotIntersection;
    int finalIntersection = finalPathIndexes[finalPathIndexes.size() - 1];
    
    
    
    for(int j = 2 * N; j < 2 * N + M; j++){
        if(timeForAllPaths[finalIntersection][j] < shortestTimeToDepot){
            shortestTimeToDepot = timeForAllPaths[finalIntersection][j];
            toDepotIntersection = depots[j - 2 * N];
        }
    }
    
    
    
    std::vector<IntersectionIdx> finalRouteIntersections;
    finalRouteIntersections.push_back(fromDepotIntersection);
    for(int i = 0; i < finalPathIndexes.size(); i++){
        if(finalPathIndexes[i] < N){
            finalRouteIntersections.push_back(deliveries[finalPathIndexes[i]].pickUp);
        }
        if(finalPathIndexes[i] >= N){
            finalRouteIntersections.push_back(deliveries[finalPathIndexes[i] - N].dropOff);
        }
    }
    finalRouteIntersections.push_back(toDepotIntersection);
    
   
    //std::cout << "STOP" << std::endl;
    
    
    for (int i = 0; i < finalRouteIntersections.size() - 1; i++){
        
        if(finalRouteIntersections[i] != finalRouteIntersections[i + 1]){
            CourierSubPath path;
            path.start_intersection = finalRouteIntersections[i];
            path.end_intersection = finalRouteIntersections[i + 1];

            std::unordered_map<IntersectionIdx, Node*> intersections;
            Node* lastDropOff = new Node(finalRouteIntersections[i]);
            lastDropOff -> bestTime = initial_bestTime;
            intersections.insert({finalRouteIntersections[i], lastDropOff});

            AStarPath(intersections, finalRouteIntersections[i], finalRouteIntersections[i + 1], turn_penalty);
            path.subpath = AStarTraceBack(intersections, finalRouteIntersections[i + 1]);
            finalTravelRoute.push_back(path);

            for (int j = 0; j < intersections.size(); j++){
                delete intersections[j];
            }
        }
        
    }
    
    //corresponding intersection in finalPathIntersections has index + 1
    
    
    //std::cout << "After Last Depot" << std::endl;
    
    /*for (int i = 0; i < finalPathIntersections.size() - 1; i++){
        CourierSubPath path;
        path.start_intersection = finalPathIntersections[i];
        path.end_intersection = finalPathIntersections[i + 1];
        
        std::unordered_map<IntersectionIdx, Node*> intersections;
        Node* lastDropOff = new Node(finalPathIntersections[i]);
        lastDropOff -> bestTime = initial_bestTime;
        intersections.insert({finalPathIntersections[i], lastDropOff});

        AStarPath(intersections, finalPathIntersections[i], finalPathIntersections[i + 1], turn_penalty);
        path.subpath = AStarTraceBack(intersections, finalPathIntersections[i + 1]);
        finalTravelRoute.push_back(path);
        
        for (int j = 0; j < intersections.size(); j++){
            delete intersections[j];
        }
    }*/
    
    
       
    //auto const endTime = std::chrono::high_resolution_clock::now();
    //auto const elapsed = std::chrono::duration<double>(endTime - startTime);

    //std::cout << elapsed.count() << std::endl;
    //return travelRoute;
    return finalTravelRoute;
    //}
    
}


double findPathTravelTime(std::vector<int> intersections, std::vector<std::vector<double>> timeForPaths){
    double totalTime = 0;
    for(int i = 0; i < intersections.size() - 1; i++){
        totalTime += timeForPaths[intersections[i]][intersections[i + 1]];
    }
    
    return totalTime;
}



bool pathLegal(std::vector<int> allPathIndex){
    int midPoint = allPathIndex.size() / 2;
    
    //map to store processed indexes
    std::unordered_map<int, bool> currentExistingIndexes;
    
    for(int i = 0; i < allPathIndex.size(); i++){
        currentExistingIndexes.insert({allPathIndex[i], true});
        
        if(allPathIndex[i] >= midPoint && currentExistingIndexes.find(allPathIndex[i] - midPoint) == currentExistingIndexes.end()){
            return false;
        }
    }
    
    return true;
}


void swapThreeOrder(std::vector<int>& vec1, std::vector<int>& vec2, std::vector<int>& vec3, std::vector<int>& finalPath, int order){
    switch(order){
        case 0:
            finalPath = vec1;
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            break;
        case 1:
            finalPath = vec1;
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            break;
        case 2:
            finalPath = vec2;
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            break;
        case 3:
            finalPath = vec2;
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            break;
        case 4:
            finalPath = vec3;
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            break;
        case 5:
            finalPath = vec3;
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            break;
        default: 
            //std::cout << "Invalid Order Input" << std::endl;
            break;
    }
}

void swapFourOrder(std::vector<int>& vec1, std::vector<int>& vec2, std::vector<int>& vec3, std::vector<int>& vec4, std::vector<int>& finalPath, int order){
    switch(order){
        case 0:
            finalPath = vec1;
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            break;
        case 1:
            finalPath = vec1;
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            break;
        case 2:
            finalPath = vec1;
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            break;
        case 3:
            finalPath = vec1;
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            break;
        case 4:
            finalPath = vec1;
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            break;
        case 5:
            finalPath = vec1;
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            break;
        case 6:
            finalPath = vec2;
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            break;
        case 7:
            finalPath = vec2;
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            break;
        case 8:
            finalPath = vec2;
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            break;
        case 9:
            finalPath = vec2;
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            break;
        case 10:
            finalPath = vec2;
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            break;
        case 11:
            finalPath = vec2;
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            break;
        case 12:
            finalPath = vec3;
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            break;
        case 13:
            finalPath = vec3;
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            break;
        case 14:
            finalPath = vec3;
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            break;
        case 15:
            finalPath = vec3;
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            break;
        case 16:
            finalPath = vec3;
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            break;
        case 17:
            finalPath = vec3;
            finalPath.insert(std::end(finalPath), std::begin(vec4), std::end(vec4));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            break;
        case 18:
            finalPath = vec4;
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            break;
        case 19:
            finalPath = vec4;
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            break;
        case 20:
            finalPath = vec4;
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            break;
        case 21:
            finalPath = vec4;
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            break;
        case 22:
            finalPath = vec4;
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            break;
        case 23:
            finalPath = vec4;
            finalPath.insert(std::end(finalPath), std::begin(vec3), std::end(vec3));
            finalPath.insert(std::end(finalPath), std::begin(vec2), std::end(vec2));
            finalPath.insert(std::end(finalPath), std::begin(vec1), std::end(vec1));
            break;
        default: 
            //std::cout << "Invalid Order Input" << std::endl;
            break;
    }
}



void reversePartialVector(std::vector<int>& vec1, std::vector<int>& vec2, std::vector<int>& vec3, int order){
    switch(order){
        case 0:
            break;
        case 1:
            std::reverse(vec1.begin(),vec1.end());
            break;
        case 2:
            std::reverse(vec2.begin(),vec2.end());
            break;
        case 3:
            std::reverse(vec3.begin(),vec3.end());
            break;
        case 4:
            std::reverse(vec1.begin(),vec1.end());
            std::reverse(vec2.begin(),vec2.end());
            break;
        case 5:
            std::reverse(vec2.begin(),vec2.end());
            std::reverse(vec3.begin(),vec3.end());
            break;
        case 6:
            std::reverse(vec3.begin(),vec3.end());
            std::reverse(vec1.begin(),vec1.end());
            break;
        case 7:
            std::reverse(vec1.begin(),vec1.end());
            std::reverse(vec2.begin(),vec2.end());
            std::reverse(vec3.begin(),vec3.end());
            break;
        default: 
            //std::cout << "Invalid Order Input" << std::endl;
            break;
    }
}

void reversePartialVector3(std::vector<int>& vec1, std::vector<int>& vec2, std::vector<int>& vec3, std::vector<int>& vec4, int order){
    switch(order){
        case 0:
            break;
        case 1:
            std::reverse(vec1.begin(),vec1.end());
            break;
        case 2:
            std::reverse(vec2.begin(),vec2.end());
            break;
        case 3:
            std::reverse(vec3.begin(),vec3.end());
            break;
        case 4:
            std::reverse(vec1.begin(),vec1.end());
            std::reverse(vec2.begin(),vec2.end());
            break;
        case 5:
            std::reverse(vec2.begin(),vec2.end());
            std::reverse(vec3.begin(),vec3.end());
            break;
        case 6:
            std::reverse(vec3.begin(),vec3.end());
            std::reverse(vec1.begin(),vec1.end());
            break;
        case 7:
            std::reverse(vec1.begin(),vec1.end());
            std::reverse(vec2.begin(),vec2.end());
            std::reverse(vec3.begin(),vec3.end());
            break;
        case 8:
            std::reverse(vec4.begin(),vec4.end());
            break;
        case 9:
            std::reverse(vec1.begin(),vec1.end());
            std::reverse(vec4.begin(),vec4.end());
            break;
        case 10:
            std::reverse(vec2.begin(),vec2.end());
            std::reverse(vec4.begin(),vec4.end());
            break;
        case 11:
            std::reverse(vec3.begin(),vec3.end());
            std::reverse(vec4.begin(),vec4.end());
            break;
        case 12:
            std::reverse(vec1.begin(),vec1.end());
            std::reverse(vec2.begin(),vec2.end());
            std::reverse(vec4.begin(),vec4.end());
            break;
        case 13:
            std::reverse(vec2.begin(),vec2.end());
            std::reverse(vec3.begin(),vec3.end());
            std::reverse(vec4.begin(),vec4.end());
            break;
        case 14:
            std::reverse(vec3.begin(),vec3.end());
            std::reverse(vec1.begin(),vec1.end());
            std::reverse(vec4.begin(),vec4.end());
            break;
        case 15:
            std::reverse(vec1.begin(),vec1.end());
            std::reverse(vec2.begin(),vec2.end());
            std::reverse(vec3.begin(),vec3.end());
            std::reverse(vec4.begin(),vec4.end());
            break;
        default: 
            //std::cout << "Invalid Order Input" << std::endl;
            break;
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
                //std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                //double time = computePathTravelTime(path, timePenalty);
                //double time = currNode->bestTime;
                //pathTimes[startIdx][pickupIt->second] = time;
                
                std::pair<map_iterator, map_iterator> result = pickupIndexes.equal_range(currNode->id);
                for (map_iterator map_it = result.first; map_it != result.second; map_it++){
                    pathTimes[startIdx][map_it->second] = currNode->bestTime;
                    importantIntersectionCount++;
                }
                    
                //importantIntersectionCount++;
            }
            if(dropoffIt != dropoffIndexes.end() && currNode->processed == false){
                checkProcessed = true;
                
                //move the path vector into the 3d database
                //std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                //double time = computePathTravelTime(path, timePenalty);
                //pathTimes[startIdx][pickupIndexes.size()+dropoffIt->second] = time;
                
                std::pair<map_iterator, map_iterator> result = dropoffIndexes.equal_range(currNode->id);
                for (map_iterator map_it = result.first; map_it != result.second; map_it++){
                    pathTimes[startIdx][pickupIndexes.size()+map_it->second] = currNode->bestTime;
                    importantIntersectionCount++;
                }
                
                //importantIntersectionCount++;
            }
            if(depotIt != depotIndexes.end() && currNode->processed == false){
                currNode->processed = true;
                //checkProcessed = true;
                
                //move the path vector into the 3d database
                //std::vector<StreetSegmentIdx> path = traceBack(intersections, currNode->id);
                //double time = computePathTravelTime(path, timePenalty);
                //pathTimes[startIdx][pickupIndexes.size()+dropoffIndexes.size()+depotIt->second] = time;
                
                std::pair<map_iterator, map_iterator> result = depotIndexes.equal_range(currNode->id);
                for (map_iterator map_it = result.first; map_it != result.second; map_it++){
                    pathTimes[startIdx][pickupIndexes.size()+dropoffIndexes.size()+map_it->second] = currNode->bestTime;
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
