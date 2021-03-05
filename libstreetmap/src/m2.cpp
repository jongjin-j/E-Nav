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
    double x = 0;
    double y = 0;
    bool highlight = false;
};

float avg_lat;

double x_from_lon(double lon){
    double x = kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat) * (lon);
    return x;
}

double y_from_lat(double lat){
    double y = kEarthRadiusInMeters * kDegreeToRadian * lat;
    return y;
}

double lon_from_x(double x){
    double lon = x/(kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat));
    return lon;
}

double lat_from_y(double y){
    double lat = y/(kEarthRadiusInMeters * kDegreeToRadian);
    return lat;
}

std::vector<intersection_data> intersections;


void draw_main_canvas(ezgl::renderer *g){
    g->draw_rectangle({0, 0}, {1000, 1000});

    for(int i = 0; i < intersections.size(); i++){
        float x = intersections[i].x;
        float y = intersections[i].y;
        
        float width = 100;
        float height = width;
        
        if(intersections[i].highlight){ 
            g->set_color(ezgl::RED);
        }
        else{ 
            g->set_color(ezgl::GREY_55);
        }
        
        g->fill_rectangle({x - width/2, y - height/2}, {x + width/2, y + height/2});
    }
    
    for(int i = 0; i < getNumStreets(); i++){
        
    }
}

void act_on_mouse_click(ezgl::application* app, GdkEventButton* event, double x, double y){
    std::cout << "Mouse clicked at (" << x << "," << y << ")\n";
    
    LatLon pos = LatLon(lat_from_y(y), lon_from_x(x));
    int id = findClosestIntersection(pos);
    
    std::cout << "Closest Intersection: "
              << intersections[id].name << "\n";
    intersections[id].highlight = true;
    
    app -> refresh_drawing();
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
    
    //set name in database and find the max and min lat, lon
    for(int i = 0; i < getNumIntersections(); i++){
        intersections[i].name = getIntersectionName(i);
        
        max_lat = std::max(max_lat, getIntersectionPosition(i).latitude());
        min_lat = std::min(min_lat, getIntersectionPosition(i).latitude());
        max_lon = std::max(max_lon, getIntersectionPosition(i).longitude());
        min_lon = std::min(min_lon, getIntersectionPosition(i).longitude());
    } 
    
    //average lat for cartesian transformation
    avg_lat = (min_lat + max_lat)/2;
    
    //change intersection points to cartesian coordinates
    for(int i = 0; i < getNumIntersections(); i++){
        intersections[i].x = x_from_lon(getIntersectionPosition(i).longitude());
        intersections[i].y = y_from_lat(getIntersectionPosition(i).latitude());
    }
 
    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world);
    
    application.run(nullptr, act_on_mouse_click, nullptr, nullptr);
}

