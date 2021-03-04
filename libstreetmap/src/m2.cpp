/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "m1.h"
#include "m2.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"

struct intersection_data{
    std::string name;
    float x = 0;
    float y = 0;
};

float avg_lat = 0;

void draw_main_canvas(ezgl::renderer *g);

std::vector<intersection_data> intersections;

void drawMap(){
    ezgl::application::settings settings;
    settings.main_ui_resource = "libstreetmap/resources/main.ui";
    settings.window_identifier = "MainWindow";
    settings.canvas_identifier = "MainCanvas";
    
    ezgl::application application(settings);
    
    double max_lat = getIntersectionPosition(0).latitude();
    double min_lat = max_lat;
    double max_lon = getIntersectionPosition(0).longitude();
    double min_lon = max_lon;
    
    intersections.resize(getNumIntersections());
    
    for(int i = 0; i < getNumIntersections(); i++){
        intersections[i].name = getIntersectionName(i);
        intersections[i].x = kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat) * (getIntersectionPosition(i).longitude());
        intersections[i].y = kEarthRadiusInMeters * kDegreeToRadian * (getIntersectionPosition(i).latitude());
        
        max_lat = std::max(max_lat, getIntersectionPosition(i).latitude());
        min_lat = std::min(min_lat, getIntersectionPosition(i).latitude());
        max_lon = std::max(max_lon, getIntersectionPosition(i).longitude());
        min_lon = std::min(min_lon, getIntersectionPosition(i).longitude());
    } 
    
    avg_lat = (min_lat + max_lat)/2;
    
    double minX = kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat) * (min_lon);
    double maxX = kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat) * (max_lon);
    double minY = kEarthRadiusInMeters * kDegreeToRadian * (min_lat);
    double maxY = kEarthRadiusInMeters * kDegreeToRadian * (max_lat);
 
    ezgl::rectangle initial_world({minX, minY}, {maxX, maxY});
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world);
    
    application.run(nullptr, nullptr, nullptr, nullptr);
}

void draw_main_canvas(ezgl::renderer *g){
    g->draw_rectangle({0, 0}, {1000, 1000});
    
    for(int i = 0; i < intersections.size(); i++){
        float x = intersections[i].x;
        float y = intersections[i].y;
        
        float width = 100;
        float height = width;
        
        g->fill_rectangle({x, y}, {x + width, y + height});
    }
}