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
#include "rectangle.hpp"

struct intersection_data{
    std::string name;
    double x = 0;
    double y = 0;
    bool highlight = false;
};

struct POI_data{
    std::string name;
    double x = 0;
    double y = 0;
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
std::vector<POI_data> POIs;


void draw_main_canvas(ezgl::renderer *g){
    g->draw_rectangle({0, 0}, {1000, 1000});

    for(int i = 0; i < intersections.size(); i++){
        float x = intersections[i].x;
        float y = intersections[i].y;
        
        //g->get_visible_world(); 
        
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
    
    ezgl::rectangle scope = g->get_visible_world();
    double scope_length = scope.m_second.x - scope.m_first.x;
    double scope_height = scope.m_second.y - scope.m_first.y;
    
    
    
    //drawing streets
    //g->set_color()
    for(int i = 0; i < getNumStreetSegments(); i++){
        
        //for each street segment, obtain its intersection IDs "from" and "to"
        //obtain each intersection ID's position via calling getIntersectionPosition (type LatLon)
        LatLon startingSeg = LatLon(getIntersectionPosition(getStreetSegmentInfo(i).from).latitude(),getIntersectionPosition(getStreetSegmentInfo(i).from).longitude());
        LatLon endingSeg = LatLon(getIntersectionPosition(getStreetSegmentInfo(i).to).latitude(),getIntersectionPosition(getStreetSegmentInfo(i).to).longitude());
        
        //convert LatLon into Cartesian coord and draw line for each segment
        double startingX = x_from_lon(startingSeg.longitude());
        double startingY = y_from_lat(startingSeg.latitude());
        double finalX = x_from_lon(endingSeg.longitude());
        double finalY = y_from_lat(endingSeg.latitude());
      
        
        g->draw_line({startingX,startingY}, {finalX,finalY});
    }
    
    //drawing features
    
    
    //drawing POIs
    for(int i = 0; i < POIs.size(); i++){
        float x = POIs[i].x;
        float y = POIs[i].y;
        
        //g->get_visible_world();
        
        float radius = 50;
        
        if(intersections[i].highlight){ 
            g->set_color(ezgl::BLUE);
        }
        else{ 
            g->set_color(ezgl::BLACK);
        }
        
        ezgl::point2d center(x, y);
        
        g->fill_elliptic_arc (center, radius, radius, 0, 360);
    }
    
    
    //writing street intersection and POI names
    
    
    //make the search box for street intersections
    
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
    
    //set intersections database
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
    
    //set POI database
    POIs.resize(getNumPointsOfInterest());
    
    for (int i = 0; i < getNumPointsOfInterest(); i++){
        POIs[i].name = getPOIName(i);
        POIs[i].x = x_from_lon(getPOIPosition(i).longitude());
        POIs[i].y = y_from_lat(getPOIPosition(i).latitude());
    }
     
    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world);
    
    application.run(nullptr, act_on_mouse_click, nullptr, nullptr);
}

