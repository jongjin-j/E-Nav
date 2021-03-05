/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   globals.h
 * Author: chungh44
 *
 * Created on March 5, 2021, 4:43 PM
 */

#ifndef GLOBALS_H
#define GLOBALS_H

extern std::vector<std::vector<StreetSegmentIdx>> intersection_street_segments;
extern std::vector<std::vector<StreetSegmentIdx>> streetID_street_segments;
extern std::vector<std::vector<StreetIdx>> streetID_intersections;
extern std::vector<double> street_segment_travelTime;
extern std::multimap<std::string, StreetIdx> streetName_and_streetID;


#endif /* GLOBALS_H */

