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
#include "OSMDatabaseAPI.h"
#include "rectangle.hpp"
#include <math.h>
#include "globals.h"
#include <typeinfo>
#include <string>
#include <unordered_map>
#include "libcurl.h"


extern struct databases database;
std::vector<std::string> fileNames;

//helper function to choose colour from feature type
bool darkMode = false;

const ezgl::color chooseFeatureColour(FeatureType x) {

    if (x == UNKNOWN) {
        //unknown returns default grey
        return ezgl::GREY_55;
    } else if (x == PARK) {
        //parks return olive green
        return ezgl::color(166, 218, 75);
    } else if (x == BEACH) {
        //beaches return beige
        return ezgl::color(220, 201, 75);
    } else if (x == LAKE) {
        //lakes return sky blue
        return ezgl::color(135, 206, 250);
    } else if (x == RIVER) {
        //rivers return sky blue
        return ezgl::color(0, 177, 235);
    } else if (x == ISLAND) {
        //islands return green
        return ezgl::color(60, 179, 113);
    } else if (x == BUILDING) {
        //buildings return dark grey
            return ezgl::color(169, 169, 169);
    } else if (x == GREENSPACE) {
        //greenspace returns light green
        return ezgl::color(154, 250, 50);
    } else if (x == GOLFCOURSE) {
        //golfcourse returns light green
        return ezgl::color(154, 250, 50);
    } else if (x == STREAM) {
        //streams return stronger blue (steel blue)
        return ezgl::color(100, 149, 237);
    }

}

void colourWidthSetter(ezgl::renderer *x, double width, ezgl::color colorChoice){
    x->set_line_width(width);
    x->set_color(colorChoice);
}

std::vector<StreetIdx> results1; //stores the results from user search street 1
std::vector<StreetIdx> results2; //stores the results from user search street 2
std::pair<StreetIdx, StreetIdx> resultStreets; //std pair to store the two chosen streets; this is passed onto the findIntersections function

//callback function of searching the first street
void searchFirstStreet(GtkWidget *, ezgl::application *application){
    
    //street1 will hold what the user inputs
    const char* street1 = gtk_entry_get_text((GtkEntry*) application -> get_object("SearchStreet1"));
    
    //results1 is the vector of matching streets
    results1 = findStreetIdsFromPartialStreetName(street1);
    
    //check if vector empty
    if(results1.size() == 0){
        std::cout << "No matching results found" << std::endl;
    }else{
        //else, give the list of results for the user to choose from
        for(int i = 0; i < results1.size(); i++){
            std::cout << getStreetName(results1[i]) << std::endl;
            //display all these into a list
            
            
        }
    }
}

//callback function of searching the second street
void searchSecondStreet(GtkWidget*, ezgl::application *application){
    
    //street2 will hold what the user inputs
    const char* street2 = gtk_entry_get_text((GtkEntry*) application -> get_object("SearchStreet2"));
    
    //results2 stores the vector of results
    results2 = findStreetIdsFromPartialStreetName(street2);
    
    //check if vector empty
    if(results2.size() == 0){
        //display error message in results box
        std::cout << "No matching results found" << std::endl;
    }else{
        //else, give the list of results for the user to choose from
        
        for(int i = 0; i < results2.size(); i++){
            std::cout << getStreetName(results2[i]) << std::endl;
        }
                //resultStreets.second = results2[0];        
        //std:: cout << resultStreets.second << std::endl;
        
        //std::cout << findIntersectionsOfTwoStreets(resultStreets)[0] << std::endl;
                
        
        //save the user's choice into the pair
        //chosen = resultStreets.first (type StreetIdx)
    }
}

void displayIntersections(GtkWidget*, ezgl::application *application){
    
    //check for boundary conditions
    if(findIntersectionsOfTwoStreets(resultStreets).size() == 0){
    std::cout << "No intersections found between the streets" << std::endl;
    }else if(getStreetName(resultStreets.first) == "<unknown>" || getStreetName(resultStreets.second) == "<unknown>"){
        //consider case when only one street is entered
        std::cout << "Please enter two valid streets" << std::endl;
    }else{
            //if valid pair, display the intersections
            std::cout << "The pair of streetIds are: " << resultStreets.first << " and " << resultStreets.second << std::endl;
            std::cout << "The street names are: " << getStreetName(resultStreets.first) << " and " << getStreetName(resultStreets.second) << std::endl; 
            std::cout << findIntersectionsOfTwoStreets(resultStreets).size() << std::endl;      //of type IntersectionIdx vector

            for(int i = 0; i < findIntersectionsOfTwoStreets(resultStreets).size(); i++){
                database.intersections[findIntersectionsOfTwoStreets(resultStreets)[i]].highlight = 1;
            }
            
             //set new scope
             //get the point of the intersection and set new scope with that as center
            
            
             //ezgl::rectangle scope = g->get_visible_world();
             //double scope_length = scope.m_second.x - scope.m_first.x;
             //double scope_height = scope.m_second.y - scope.m_first.y;
             //std::cout << scope_length << "  " << scope_height << std::endl;  
    }
    application->refresh_drawing();
}

void resetIntersections(GtkWidget*, ezgl::application *application){
    
    for(int i = 0; i < getNumIntersections(); i++){
        if(database.intersections[i].highlight == 1){
            database.intersections[i].highlight = 0;
        }
    }
    application->refresh_drawing();
}

//GtkListStore* resultList = gtk_list_store_new(1, G_TYPE_STRING);

void reloadMap(GtkWidget*, ezgl::application *application){
    std::cout << "Map reloaded" << std::endl;
}

void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data){
    gtk_widget_destroy(GTK_WIDGET (dialog));
}

void displayWeather(GtkWidget*, ezgl::application *application){
    
    std::cout<<"Displaying Weather" << std::endl;
    
    GObject *window;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget* dialog;
    
    window = application -> get_object(application->get_main_window_id().c_str());
    
    dialog = gtk_dialog_new_with_buttons(
            "Weather Conditions",
            (GtkWindow*) window,
            GTK_DIALOG_MODAL,
            ("Close"),
            GTK_RESPONSE_ACCEPT
            );
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    label = gtk_label_new("Temperature:\nFeels like:\nHumidity:\nPressure:\n");
    gtk_container_add(GTK_CONTAINER(content_area), label);
    
    gtk_widget_show_all(dialog);
    
    g_signal_connect(
        GTK_DIALOG(dialog),
        "response",
        G_CALLBACK(on_dialog_response),
        NULL
    );
    application -> refresh_drawing();
    
}

void displayWindow(GtkWidget*, ezgl::application *application){
    
    GtkWidget* dialog = gtk_dialog_new();
    
    gtk_widget_show(dialog);
}


void initial_setup(ezgl::application *application, bool /*new_window*/){
    g_signal_connect(application->get_object("SearchStreet1"), "changed", G_CALLBACK(searchFirstStreet), application);
    
    g_signal_connect(application->get_object("SearchStreet1"), "activate", G_CALLBACK(displayWindow), application);

    g_signal_connect(application->get_object("SearchStreet2"), "changed", G_CALLBACK(searchSecondStreet), application);
    
    g_signal_connect(application->get_object("FindButton"), "clicked", G_CALLBACK(displayIntersections), application);
    g_signal_connect(application->get_object("ResetButton"), "clicked", G_CALLBACK(resetIntersections), application);
    //g_signal_connect(selectCity)--used to select the city to reload
    g_signal_connect(application->get_object("GoButton"), "clicked", G_CALLBACK(reloadMap), application);
    g_signal_connect(application->get_object("WeatherButton"), "clicked", G_CALLBACK(displayWeather), application);
    
    //g_signal_connect(application->get_object(),"",G_CALLBACK(),application);
}


//function to draw a POI

void draw_POI_function(ezgl::renderer *g, ezgl::point2d center_point, double font, ezgl::surface *p, std::string name){
    g->set_text_rotation(0);
    g->draw_surface(p, {center_point.x - 1, center_point.y - 2});
    ezgl::renderer::free_surface(p);
    g->set_color(ezgl::BLACK);
    g->set_font_size(font);
    g->draw_text(center_point, name);
}


//function to draw POIs
void draw_POIs(ezgl::renderer *g, int i, double font){

    //g->set_color(ezgl::BLUE);
    //g->set_text_rotation(0);
        
    ezgl::rectangle scope = g->get_visible_world();
    double scope_min_x = scope.m_first.x;
    double scope_max_x = scope.m_second.x;
    double scope_min_y = scope.m_first.y;
    double scope_max_y = scope.m_second.y;

    ezgl::point2d center_point(database.POIs[i].x, database.POIs[i].y + 2);
    bool include = false;
        
    if(database.POIs[i].x > scope_min_x  && database.POIs[i].x < scope_max_x && database.POIs[i].y > scope_min_y  && database.POIs[i].y < scope_max_y){
        include = true;
    }
        
    //std::unordered_map<OSMID, std::string>::const_iterator it = database.OSMID_nodeType.find(database.POIs[i].id);
    //std::unordered_map<OSMID, std::string>::const_iterator it2 = database.OSMID_wayType.find(database.POIs[i].id);
        
    if(include){
        if (database.POIs[i].type == "restaurant"){
        ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/restaurant.png");
        draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "school"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/school.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "cafe"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/cafe.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "bank"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bank.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "hospital" || database.POIs[i].type == "clinic"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/hospital.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "subway_entrance"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/subway_entrance.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "bus_stop"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bus_station.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "aerodrome"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/aerodrome.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "supermarket"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/supermarket.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "wholesale"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/supermarket.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
    }
}

void draw_important_POIs(ezgl::renderer *g, int i, double font){
    g->set_text_rotation(0);
        
    ezgl::rectangle scope = g->get_visible_world();
    double scope_min_x = scope.m_first.x;
    double scope_max_x = scope.m_second.x;
    double scope_min_y = scope.m_first.y;
    double scope_max_y = scope.m_second.y;

    ezgl::point2d center_point((database.POIs[i]).x, (database.POIs[i]).y + 2);
    bool include = false;
        
    if((database.POIs[i]).x > scope_min_x  && (database.POIs[i]).x < scope_max_x && (database.POIs[i]).y > scope_min_y  && (database.POIs[i]).y < scope_max_y){
        include = true;
    }
        
    //std::unordered_map<OSMID, std::string>::const_iterator it = (database.OSMID_nodeType).find((database.POIs[i]).id);
    //std::unordered_map<OSMID, std::string>::const_iterator it2 = (database.OSMID_wayType).find((database.POIs[i]).id);
    
    if(include){
        if (database.POIs[i].type == "hospital" || database.POIs[i].type == "clinic"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/hospital.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
        if (database.POIs[i].type == "bus_station"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bus_station.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name);
        }
    }
}


void drawSegment(ezgl::renderer *g, StreetSegmentInfo tempInfo, int i, ezgl::color colorChoice){
    g->set_color(colorChoice);
    if(tempInfo.numCurvePoints == 0){
         g->draw_line({database.streets[i].start_x, database.streets[i].start_y}, {database.streets[i].end_x, database.streets[i].end_y});
    }   
    else{
            int curvePoints = tempInfo.numCurvePoints;
            
            //create a dynamic array to store the curve points in the street segment
            LatLon *segmentCurvePoints = new LatLon [curvePoints + 2];
            for (int j = 1; j < curvePoints + 1; j++) {
                segmentCurvePoints[j] = getStreetSegmentCurvePoint(i, j - 1);
            }

            segmentCurvePoints[0] = getIntersectionPosition(tempInfo.from);
            segmentCurvePoints[curvePoints + 1] = getIntersectionPosition(tempInfo.to);

            //compute distance of each street segment excluding the first and the last street segment
            for (int j = 0; j < curvePoints + 1; j++) {
                double firstCurve_x = x_from_lon((segmentCurvePoints[j]).longitude());
                double firstCurve_y = y_from_lat((segmentCurvePoints[j]).latitude());
                double lastCurve_x = x_from_lon((segmentCurvePoints[j + 1]).longitude());
                double lastCurve_y = y_from_lat((segmentCurvePoints[j + 1]).latitude());
                g->draw_line({firstCurve_x, firstCurve_y}, {lastCurve_x, lastCurve_y});
            }
            delete[] segmentCurvePoints;
    }
}

void writeStreetName(ezgl::renderer *g, ezgl::point2d center, StreetSegmentInfo segInfo, std::string name, int i){
    if (name != "<unknown>"){
        if (name == "> <unknown> >" || name == "< <unknown> <"){
            name.erase (name.begin()+1, name.end()-1);
        } 
        
        if(segInfo.numCurvePoints == 0){
            g->draw_text(center, name);
        }
        else{
            int midPoint = segInfo.numCurvePoints / 2;
            LatLon point;
            LatLon point2;
            if(midPoint % 2 == 0){
                //access curvePoint from 0 ~ numCurvePoints - 1
                point = getStreetSegmentCurvePoint(i, midPoint);
                double mid_x = x_from_lon(point.longitude());
                double mid_y = y_from_lat(point.latitude());
                ezgl::point2d middlePoint(mid_x, mid_y);
                g->draw_text(middlePoint, name);
            }
            else{
                point = getStreetSegmentCurvePoint(i, midPoint - 1);
                point2 = getStreetSegmentCurvePoint(i, midPoint);
                double mid_x = 0.5 * (x_from_lon(point.longitude()) + x_from_lon(point2.longitude()));
                double mid_y = 0.5 * (y_from_lat(point.latitude()) + y_from_lat(point2.latitude()));
                ezgl::point2d middlePoint(mid_x, mid_y);
                g->draw_text(middlePoint, name);
            }
        }
    }
}

void draw_main_canvas(ezgl::renderer *g) {
    g->draw_rectangle({0, 0}, {1000, 1000});

    ezgl::rectangle scope = g->get_visible_world();
    double scope_length = scope.m_second.x - scope.m_first.x;
    double scope_height = scope.m_second.y - scope.m_first.y;
    //std::cout << scope_length << "  " << scope_height << std::endl;
     
    //drawing features
    for (int i = 0; i < getNumFeatures(); i++) {

        //if it's a closed feature
        if (getFeaturePoint(i, 0) == getFeaturePoint(i, getNumFeaturePoints(i) - 1)) {

            //declare vector of 2d points
            std::vector<ezgl::point2d> featurePoints;

            featurePoints.resize(getNumFeaturePoints(i), {0, 0});

            //loop through # feature points and add to vector of 2d points
            for (int j = 0; j < getNumFeaturePoints(i); j++) {
                double xCoord = x_from_lon(getFeaturePoint(i, j).longitude());
                double yCoord = y_from_lat(getFeaturePoint(i, j).latitude());

                featurePoints[j] = {xCoord, yCoord};
            }
            //fill poly only if feature is 2D
            if (featurePoints.size() > 1) {
                //exception for buildings (crowdedness) to draw only at scope level of 6000
                if(getFeatureType(i) == BUILDING){
                    if(scope_length < 5000 && scope_height < 5000){
                        g->set_line_width(1);
                        g->set_color(chooseFeatureColour(getFeatureType(i)));
                        g->fill_poly(featurePoints);
                    }
                }
                else{
                    g->set_line_width(1);
                    g->set_color(chooseFeatureColour(getFeatureType(i)));
                    g->fill_poly(featurePoints);
                }
            }

        } else {
            //open feature
            //draw with open lines
            for (int j = 0; j < getNumFeaturePoints(i) - 1; j++) {

                double xCoord = x_from_lon(getFeaturePoint(i, j).longitude());
                double yCoord = y_from_lat(getFeaturePoint(i, j).latitude());

                //choose colour depending on feature type
                colourWidthSetter(g, 1, chooseFeatureColour(getFeatureType(i)));
                g->set_line_cap(ezgl::line_cap::butt);
                    
                if(getFeatureType(i) == STREAM){
                    
                    if(scope_length < 15000 && scope_height < 15000){
                      g->draw_line({xCoord, yCoord},
                    {x_from_lon(getFeaturePoint(i, j + 1).longitude()), y_from_lat(getFeaturePoint(i, j + 1).latitude())});  
                    }
                    
                }else{
                    
                    g->draw_line({xCoord, yCoord},
                    {x_from_lon(getFeaturePoint(i, j + 1).longitude()), y_from_lat(getFeaturePoint(i, j + 1).latitude())});
                }
                
                
            }

        }
    }
    
    //drawing streets
    for (int i = 0; i < getNumStreetSegments(); i++) {

        //introduce white streets at a max scope of 12000 height & width        //suggestion. while loop?
        if (scope_length < 7000 && scope_height < 7000) {
            //helper function to set width and color of line
            colourWidthSetter(g, 6, ezgl::WHITE);
            StreetSegmentInfo tempInfo = getStreetSegmentInfo(i);
            //g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
            drawSegment(g, tempInfo, i, ezgl::WHITE);
            
            //introduce bordered streets if scope within approx 2000
            if (scope_length < 2000 && scope_height < 1700) {
                colourWidthSetter(g, 6, ezgl::color(130,130,130));
                g->set_line_cap(ezgl::line_cap::round);
                if(scope_length < 1000){
                    g->set_line_width(9);
                }
                if(scope_length < 600){
                    g->set_line_width(12);
                }
                if(scope_length < 400){
                    g->set_line_width(15);
                }
                if(scope_length < 200){
                    g->set_line_width(18);
                }
                //g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
                drawSegment(g,tempInfo, i, ezgl::WHITE);
                //then draw the street in white
                colourWidthSetter(g, 4, ezgl::WHITE);
                if(scope_length < 1000){
                    g->set_line_width(8);
                }
                if(scope_length < 600){
                    g->set_line_width(11);
                }
                if(scope_length < 400){
                    g->set_line_width(14);
                }
                if(scope_length < 200){
                    g->set_line_width(17);
                }
                //g->draw_line({streets[i].start_x, streets[i].start_y},{streets[i].end_x, streets[i].end_y});
                drawSegment(g, tempInfo, i, ezgl::WHITE);
            }  
        }
        
        if (scope_length < 100000){
            StreetSegmentInfo tempInfo = getStreetSegmentInfo(i);
            std::unordered_map<OSMID, std::string>::const_iterator it = database.OSMID_wayType.find(tempInfo.wayOSMID);
            if (it != database.OSMID_wayType.end() && (it->second == "motorway" || it->second == "motorway_link")){
                g->set_line_width(4);
                if(scope_length < 2700){
                    g->set_line_width(8);
                } 
                if(scope_length < 1700){
                    g->set_line_width(12);
                } 
                if(scope_length < 1000){
                    g->set_line_width(16);
                }
                if(scope_length < 600){
                    g->set_line_width(24);
                }
                //g->set_color(ezgl::ORANGE);
                //g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
                drawSegment(g, tempInfo, i, ezgl::ORANGE);
            }
            if (it != database.OSMID_wayType.end() && (it->second == "primary" || it->second == "secondary")){
                g->set_line_width(1.5);
                if(scope_length > 5000){
                    drawSegment(g, tempInfo, i, ezgl::WHITE);
                }
                else{
                    g->set_line_width(4);
                    drawSegment(g, tempInfo, i, ezgl::YELLOW);
                }
                //g->set_color(ezgl::WHITE);
                //g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
            }
        }
        
        if (scope_length < 1500 && scope_height < 1200) {
            if (findStreetSegmentLength(i) > 70 && scope.m_first.x < database.streets[i].mid_x && scope.m_second.x > database.streets[i].mid_x && scope.m_first.y < database.streets[i].mid_y && scope.m_second.y > database.streets[i].mid_y) {
                ezgl::point2d centerPoint(database.streets[i].mid_x, database.streets[i].mid_y);
                StreetSegmentInfo tempInfo = getStreetSegmentInfo(i);
                
                //display the street names
                g->set_text_rotation(database.streets[i].angle);
                g->set_font_size(8);
                g->set_color(ezgl::BLACK);
                
                if(scope_length < 1000){
                    g->set_font_size(12);
                }
                
                if (database.streets[i].oneWay){
                    if (database.streets[i].reverse){
                       //g->draw_text(centerPoint, "< " + streets[i].name + " <");
                       writeStreetName(g, centerPoint, tempInfo, "< " + database.streets[i].name + " <", i);
                    }
                    else {
                       //g->draw_text(centerPoint, "> " + streets[i].name + " >");
                       writeStreetName(g, centerPoint, tempInfo, "> " + database.streets[i].name + " >", i);
                    }
                }
                else {
                    //g->draw_text(centerPoint, streets[i].name);
                    writeStreetName(g, centerPoint, tempInfo, database.streets[i].name, i);
                    
                }
            }
        }
    }
    
    //drawing POIs
    for (int i = 0; i < database.POIs.size(); i++) {
        if (scope_length < 85 && scope_height < 70) {
            draw_POIs(g, i, 16);
        }
        else if (scope_length < 240 && scope_height < 185) {
            draw_POIs(g, i, 13);
        }
        else if (scope_length < 385 && scope_height < 305) {
            draw_POIs(g, i, 10);
        }
        else if(scope_length < 4200 && scope_height < 3000){
            draw_important_POIs(g, i, 10);
        }
        else{
            //draw_important_POIs(g, i, 10);
        }
    }
    
    
    //drawing intersections
    for (int id = 0; id < database.intersections.size(); id++) {
        float x = database.intersections[id].x;
        float y = database.intersections[id].y;
        float radius = 1;
        if(scope_length < 1800 && scope_length > 800){
            radius = 1.5;
        }

        if (database.intersections[id].highlight) {

            ezgl::point2d center(x, y);
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/redpin.png");
            //g->draw_surface(png_surface, {center.x - 5, center.y + 16});
            double scopeRatioX = 0.00928098;
            double scopeRatioY = 0.05407086;
            g->draw_surface(png_surface, {center.x - scopeRatioX * scope_length, center.y + scopeRatioY * scope_height});
            ezgl::renderer::free_surface(png_surface);
            
            //print name of intersection
            ezgl::point2d center_point(x, y + scopeRatioY* (1.15) * scope_height);
            g->set_color(ezgl::BLACK);
            g->set_font_size(13);
            g->draw_text(center_point, database.intersections[id].name);
            std::cout << "Closest Intersection: " << database.intersections[id].name << std::endl;
            
            
        } 
        else if(database.intersections[id].highlight == false && scope_length < 800){
            g->set_color(ezgl::GREY_55);
            ezgl::point2d center(x, y);
            g->fill_arc(center, radius, 0, 360);
        }
    }

    //make the search box for street intersections

}

void act_on_mouse_click(ezgl::application* app, GdkEventButton* event, double x, double y) {

    LatLon pos = LatLon(lat_from_y(y), lon_from_x(x));
    int id = findClosestIntersection(pos);

    if (database.intersections[id].highlight == true)
        database.intersections[id].highlight = false;
    else
        database.intersections[id].highlight = true;

    app -> refresh_drawing();
}


void drawMap() {
    loadCityWeatherData();
    
    for (int i=0; i<fileNames.size(); i++){
        std::cout << (fileNames[i]) << std::endl;
    }
    
    
    ezgl::application::settings settings;
    settings.main_ui_resource = "libstreetmap/resources/main.ui";
    settings.window_identifier = "MainWindow";
    settings.canvas_identifier = "MainCanvas";

    ezgl::application application(settings);

    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)},
    {
        x_from_lon(max_lon), y_from_lat(max_lat)
    });
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world, ezgl::color(230,230,230,230));

    application.run(initial_setup, act_on_mouse_click, nullptr, nullptr);
}

void changeMap(ezgl::application* app){
    closeMap();
    
    //clear the global variables (done in close map?)
    
    
    std::string cityName;
    std::cin >> cityName;
    cityName.erase(std::remove(cityName.begin(), cityName.end(), ' '), cityName.end()); //code snippet from https://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
    std::transform(cityName.begin(), cityName.end(), cityName.begin(), ::tolower); // code snippet from https://www.geeksforgeeks.org/conversion-whole-string-uppercase-lowercase-using-stl-c/
            
    //load OSM database
    for (int i=0; fileNames.size(); i++){
        if (fileNames[i].find(cityName) != std::string::npos && fileNames[i].find("osm") != std::string::npos){
            loadOSMDatabaseBIN(fileNames[i]);
        }
    }
    
    //load streets database
    for (int i=0; fileNames.size(); i++){
        if (fileNames[i].find(cityName) != std::string::npos && fileNames[i].find("streets") != std::string::npos){
            loadOSMDatabaseBIN(fileNames[i]);
        }
    }
    
    /*
    //parse the map path
    std::string filename = "something";
    int lastSlash = filename.find_last_of("/");
    filename.erase(0, lastSlash);
    
    int firstPeriod = filename.find_first_of(".");
    filename.erase(firstPeriod, filename.length() - 1);
    */
   
    app -> refresh_drawing();
}


//path: /cad2/ece297s/public/maps
