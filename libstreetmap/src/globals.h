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

#include <unordered_map>
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"

//initial best time must be a large number
#define initial_bestTime 1000000000

extern struct databases database;

extern int cityNums;

extern IntersectionIdx startIntersectionID, destIntersectionID;

struct intersection_data {
    std::string name;
    LatLon pos;
    double x = 0;
    double y = 0;
    bool highlight = false;
    bool start = false;
    bool dest = false;
};

struct POI_data {
    std::string name;
    std::string type;
    OSMID id;
    double x = 0;
    double y = 0;
};

struct street_data {
    std::string name;
    IntersectionIdx intersection_from;
    IntersectionIdx intersection_to;
    double start_x = 0;
    double start_y = 0;
    double end_x = 0;
    double end_y = 0;
    double angle = 0;
    double mid_x = 0;
    double mid_y = 0;
    bool oneWay;
    bool reverse = false;
    std::string street_type;
    StreetIdx ID;
};


struct vector{
    double x;
    double y;
    vector(){
        x = 0;
        y = 0;
    }
};

struct databases {
    std::vector<std::vector<StreetSegmentIdx>> intersection_street_segments;
    std::vector<std::vector<StreetSegmentIdx>> streetID_street_segments;
    std::vector<std::vector<StreetIdx>> streetID_intersections;
    std::vector<double> street_segment_travelTime;
    std::multimap<std::string, StreetIdx> streetName_and_streetID;

    std::vector<intersection_data> intersections;
    std::vector<POI_data> POIs;
    std::vector<std::vector<StreetSegmentIdx>> streetSegments;
    std::vector<street_data> street_segments;
    std::vector<StreetIdx> results1; //stores the results from user search street 1
    std::vector<StreetIdx> results2; //stores the results from user search street 2
    std::unordered_map<OSMID, std::string> OSMID_wayType;
    std::vector<StreetIdx>streetSegmentID_streetID;
};

extern float avg_lat;
extern double min_lat;
extern double max_lat;
extern double min_lon;
extern double max_lon;
extern double maxSpeed;
extern std::vector<std::string> fileNames;


//m3.cpp function declaration

//class for a node (intersection)
class Node{
public:
    IntersectionIdx id;
    StreetSegmentIdx reachingEdge;
    double bestTime = initial_bestTime;
    bool turn = false;
    bool processed = false;
    Node(int inter_id){
        id = inter_id;
    }
};

void validSegmentsAndIntersections(std::vector<std::pair<StreetSegmentIdx, IntersectionIdx>>& valid, std::vector<StreetSegmentIdx> &segments, IntersectionIdx point);

bool AStarPath(std::unordered_map<IntersectionIdx, Node*>& intersections, int startID, int destID, double turnPenalty);
std::vector<StreetSegmentIdx> AStarTraceBack(std::unordered_map<IntersectionIdx, Node*>& intersections, int destID);

//m2.cpp function declaration
const ezgl::color chooseFeatureColour(FeatureType x); 
void colourWidthSetter(ezgl::renderer *x, double width, ezgl::color colorChoice);
void searchFirstStreet(GtkWidget *widget, ezgl::application *application);
void searchSecondStreet(GtkWidget *widget, ezgl::application *application);
void displayIntersections(GtkWidget *widget, ezgl::application *application);
void initial_setup(ezgl::application *application, bool /*new_window*/);
void draw_POI_function(ezgl::renderer *g, ezgl::point2d center_point, double font, ezgl::surface *p, std::string name, double scope_length, double scope_height);
void draw_important_POIs(ezgl::renderer *g, int i, double font);
void draw_POIs(ezgl::renderer *g, int i, double font);
void drawSegment(ezgl::renderer *g, StreetSegmentInfo tempInfo, int i, ezgl::color colorChoice);
void writeStreetName(ezgl::renderer *g, ezgl::point2d center, StreetSegmentInfo segInfo, std::string name, int i);
void draw_main_canvas(ezgl::renderer *g);
void act_on_mouse_click(ezgl::application* app, GdkEventButton* event, double x, double y);

//m1.cpp function declaration
double x_from_lon(double lon);
double y_from_lat(double lat);
double lon_from_x(double x);
double lat_from_y(double y);
void simplifiedStreetNames_streetIdx ();
void intersections_streetSegments ();
void streetID_streetSegments();
void streetID_Intersections();
void streetSegment_travelTime();
void set_MaxMinLatLon_avgLat();
void intersections_database();
void POI_database();
void streets_database();
void OSMID_wayValue();
void POIDatabase_nonAmenity();
int createFileList(std::string directoryPath);
void createNodesfromIntersections();
void street_SegmentID_streetID();
void findMaxSpeed();

extern std::vector<int> weatherData;

#endif /* GLOBALS_H */

