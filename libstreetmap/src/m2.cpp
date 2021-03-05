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
#include <math.h>
#include "globals.h"

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

struct street_data{
    std::string name;
    double start_x = 0;
    double start_y = 0;
    double end_x = 0;
    double end_y = 0;
    double angle = 0;
    double mid_x = 0;
    double mid_y = 0;
    bool oneWay;
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
std::vector<std::vector<StreetSegmentIdx>> streetSegments;
std::vector<street_data> streets;

void draw_main_canvas(ezgl::renderer *g){
    g->draw_rectangle({0, 0}, {1000, 1000});

    for(int id = 0; id < intersections.size(); id++){
        float x = intersections[id].x;
        float y = intersections[id].y;
        
        //g->get_visible_world(); 
        
        float width = 10;
        float height = width;
        
        if (intersections[id].highlight){
            //print name of intersection
            ezgl::point2d center_point(x,y+7.5);
            g->set_color(ezgl::BLACK);
            g->set_font_size(13);
            g->draw_text(center_point, intersections[id].name);
            
            //set color for intersection icon 
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
        g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
        
        if (scope_length < 500 && scope_height < 400){
            ezgl::point2d centerPoint (streets[i].mid_x, streets[i].mid_y);
            g->set_text_rotation(streets[i].angle);
            g->set_font_size(20);
            g->draw_text(centerPoint, streets[i].name);
        }
    }
    
    //drawing features
    for(int i = 0; i < getNumFeatures(); i++){

        //if it's a closed feature
        if(getFeaturePoint(i,0) == getFeaturePoint(i, getNumFeaturePoints(i)-1)){
            
            std::vector<ezgl::point2d> featurePoints;
            //featurePoints.resize(getNumFeaturePoints(i));

            
            /*for(int j = 0; j < getNumFeaturePoints(i); j++){
                double xCoord = x_from_lon(getFeaturePoint(i,j).longitude());
                double yCoord = y_from_lat(getFeaturePoint(i,j).latitude());
                featurePoints.push_back({xCoord,yCoord});
            }
            g->fill_poly(featurePoints);*/
     
        }
        else{
            //not a closed feature
            //draw with open lines
            for(int j = 0; j < getNumFeaturePoints(i); j++){
                      
                double xCoord = x_from_lon(getFeaturePoint(i,j).longitude());
                double yCoord = y_from_lat(getFeaturePoint(i,j).latitude());
                                   
                //choose colour depending on feature type
                /*g->set_color(ezgl::RED);
                
                if(getFeatureType)*/
                
                g->draw_line({xCoord,yCoord},{x_from_lon(getFeaturePoint(i,j).longitude()),y_from_lat(getFeaturePoint(i,j).latitude())});
            }

        }
    }
    
    
    //drawing POIs
    
    for(int i = 0; i < POIs.size(); i++){
        //g->get_visible_world();
        
        float radius = 5;
        
        g->set_color(ezgl::BLUE);
        g->set_text_rotation(0);
        
        ezgl::point2d center(POIs[i].x, POIs[i].y);
        
        g->fill_elliptic_arc (center, radius, radius, 0, 360);
        
        ezgl::point2d center_point(POIs[i].x, POIs[i].y + 7.5);

        if(scope_length < 85 && scope_height < 70){
            g->set_color(ezgl::BLACK);
            g->set_font_size(16);
            g->draw_text(center_point, POIs[i].name);
        }
        
        else if(scope_length < 240 && scope_height < 185){
            g->set_color(ezgl::BLACK);
            g->set_font_size(13);
            g->draw_text(center_point, POIs[i].name);
        }
        
        else if(scope_length < 385 && scope_height < 305){
            g->set_color(ezgl::BLACK);
            g->set_font_size(10);
            g->draw_text(center_point, POIs[i].name);
        }
        
        else if(scope_length < 650 && scope_height < 505){
            g->set_color(ezgl::BLACK);
            g->set_font_size(8);
            g->draw_text(center_point, POIs[i].name);
        }
    }
    
    
    //writing street intersection and POI names
    
    /*for(int i = 0; i < getNumStreets(); i++){
        if (scope_length < 85 && scope_height < 70){
            double midPointX = 0.5 * (streets[i].end_x + streets[i].start_x);
            double midPointY = 0.5 * (streets[i].end_y + streets[i].start_y);
            ezgl::point2d centerPoint (midPointX, midPointY);
            g->set_text_rotation(streets[i].angle);
            g->set_font_size(10);
            g->draw_text(centerPoint, streets[i].name);
        }
    } */

    
    //make the search box for street intersections
    
}

void act_on_mouse_click(ezgl::application* app, GdkEventButton* event, double x, double y){
    std::cout << "Mouse clicked at (" << x << "," << y << ")\n";
    
    LatLon pos = LatLon(lat_from_y(y), lon_from_x(x));
    int id = findClosestIntersection(pos);
    //std::vector<std::string> intersectingStreets = 
    
    std::cout << "Closest Intersection: "
              << intersections[id].name << "\n";
    
    if (intersections[id].highlight == true)
        intersections[id].highlight = false;
    else
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
    
    //
    streets.resize(getNumStreetSegments());
    
    for(int i = 0; i < getNumStreetSegments(); i++){
        
        //for each street segment, obtain its intersection IDs "from" and "to"
        //obtain each intersection ID's position via calling getIntersectionPosition (type LatLon)
        LatLon startingSeg = LatLon(getIntersectionPosition(getStreetSegmentInfo(i).from).latitude(),getIntersectionPosition(getStreetSegmentInfo(i).from).longitude());
        LatLon endingSeg = LatLon(getIntersectionPosition(getStreetSegmentInfo(i).to).latitude(),getIntersectionPosition(getStreetSegmentInfo(i).to).longitude());
        
        //convert LatLon into Cartesian coord and draw line for each segment
        streets[i].start_x = x_from_lon(startingSeg.longitude());
        streets[i].start_y = y_from_lat(startingSeg.latitude());
        streets[i].end_x = x_from_lon(endingSeg.longitude());
        streets[i].end_y = y_from_lat(endingSeg.latitude());
        streets[i].mid_x = 0.5 * (streets[i].start_x + streets[i].end_x);
        streets[i].mid_y = 0.5 * (streets[i].start_y + streets[i].end_y);

        double rotation = 0;
            
        if(streets[i].end_x == streets[i].start_x){
            rotation = 90;
        }
            
        else{
            rotation = std::atan(abs((streets[i].end_y - streets[i].start_y) / (streets[i].end_x - streets[i].start_x))) / kDegreeToRadian;
        }
        
        streets[i].angle = rotation;
        streets[i].name = getStreetName(getStreetSegmentInfo(i).streetID);
    }
    
    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)}, {x_from_lon(max_lon), y_from_lat(max_lat)});
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world);
    
    application.run(nullptr, act_on_mouse_click, nullptr, nullptr);
}

