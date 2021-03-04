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
    LatLon position;
    std::string name;
};

float avg_lat;

std::vector<intersection_data> intersections;

void draw_main_canvas(ezgl::renderer *g);

float x_from_lon(float lon){
    float x = kEarthRadiusInMeters * kDegreeToRadian * cos(kDegreeToRadian * avg_lat) * (lon);
    return x;
}

float y_from_lat(float lat){
    float y = kEarthRadiusInMeters * kDegreeToRadian * (lat);
    return y;
}

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
        intersections[i].position = getIntersectionPosition(i);
        intersections[i].name = getIntersectionName(i);
        
        max_lat = std::max(max_lat, intersections[i].position.latitude());
        min_lat = std::min(min_lat, intersections[i].position.latitude());
        max_lon = std::max(max_lon, intersections[i].position.longitude());
        min_lon = std::min(min_lon, intersections[i].position.longitude());
      
    } 
    
    avg_lat = (min_lat + max_lat)/2;
    
    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world);
    
    application.run(nullptr, nullptr, nullptr, nullptr);
}

void draw_main_canvas(ezgl::renderer *g){
    g->draw_rectangle({0, 0}, {1000, 1000});
    
    for(int i = 0; i < intersections.size(); i++){
        float x = x_from_lon(intersections[i].position.longitude());
        float y = y_from_lat(intersections[i].position.latitude());
        
        float width = 100;
        float height = width;
        
        g->fill_rectangle({x, y}, {x + width, y + height});
    }
}