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

struct intersection_data {
    std::string name;
    double x = 0;
    double y = 0;
    bool highlight = false;
};

struct POI_data {
    std::string name;
    OSMID id;
    double x = 0;
    double y = 0;
};

struct street_data {
    std::string name;
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
};

float avg_lat;

double x_from_lon(double lon) {
    double x = kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat) * (lon);
    return x;
}

double y_from_lat(double lat) {
    double y = kEarthRadiusInMeters * kDegreeToRadian * lat;
    return y;
}

double lon_from_x(double x) {
    double lon = x / (kEarthRadiusInMeters * kDegreeToRadian * std::cos(kDegreeToRadian * avg_lat));
    return lon;
}

double lat_from_y(double y) {
    double lat = y / (kEarthRadiusInMeters * kDegreeToRadian);
    return lat;
}

//helper function to choose colour from feature type

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

std::vector<intersection_data> intersections;
std::vector<POI_data> POIs;
std::vector<std::vector<StreetSegmentIdx>> streetSegments;
std::vector<street_data> streets;
std::vector<StreetIdx> results1; //stores the results from user search street 1
std::vector<StreetIdx> results2; //stores the results from user search street 2
std::unordered_map<OSMID, std::string> OSMID_wayType;
std::unordered_map<OSMID, std::string> OSMID_nodeType;


void colourWidthSetter(ezgl::renderer *x, double width, ezgl::color colorChoice){
    x->set_line_width(width);
    x->set_color(colorChoice);
}

/*void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data){

    std::cout << "response is ";
        switch(response_id) {
            case GTK_RESPONSE_ACCEPT:
                std::cout << "GTK_RESPONSE_ACCEPT ";
                break;
            case GTK_RESPONSE_DELETE_EVENT:
                std::cout << "GTK_RESPONSE_DELETE_EVENT (i.e. ’X’ button) ";
                break;
            case GTK_RESPONSE_REJECT:
                std::cout << "GTK_RESPONSE_REJECT ";
                break;
            default:
                std::cout << "UNKNOWN ";
                break;
        }
    std::cout << "(" << response_id << ")\n";
    gtk_widget_destroy(GTK_WIDGET (dialog));
    
}*/

//std pair to store the two chosen streets; this is passed onto the findIntersections function
std::pair<StreetIdx, StreetIdx> resultStreets;

//callback function of searching the first street
void searchFirstStreet(GtkWidget *widget, ezgl::application *application){
    
    //street1 will hold what the user inputs
    const char* street1 = gtk_entry_get_text((GtkEntry*) application -> get_object("SearchStreet1"));
    
    //results1 is the vector of matching streets
    results1 = findStreetIdsFromPartialStreetName(street1);
    
    //check if vector empty
    if(results1.size() == 0){
        std::cout << "No matching results found" << std::endl;
    }else{
        //else, give the list of results for the user to choose from
        /*
        for(int i = 0; i < results1.size(); i++){
            std::cout << getStreetName(results1[i]) << std::endl;
        }
         */
        //test code, ignore
        resultStreets.first = results1[0];
        std::cout << results1[0] << std::endl;
        
        
        //save the user's choice into the pair
        //chosen = resultStreets.first (type StreetIdx)
    }
    
    /*GObject *window; // the parent window over which to add the dialog
    GtkWidget *content_area; // the content area of the dialog
    GtkWidget *label; // the label we will create to display a message in the content area
    GtkWidget *dialog; // the dialog box we will create

    window = application->get_object(application->get_main_window_id().c_str());

    dialog = gtk_dialog_new_with_buttons(
            "Search Results",
            (GtkWindow*) window,
            GTK_DIALOG_MODAL,
            ("OK"),
            GTK_RESPONSE_ACCEPT,
            ("CANCEL"),
            GTK_RESPONSE_REJECT,
            NULL
            );

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    label = gtk_label_new("Prototype; display search results here");
    gtk_container_add(GTK_CONTAINER(content_area), label);

    gtk_widget_show_all(dialog);
    
    g_signal_connect(
        GTK_DIALOG(dialog),
        "response",
        G_CALLBACK(on_dialog_response),
        NULL
    );*/
}

//callback function of searching the second street
void searchSecondStreet(GtkWidget *widget, ezgl::application *application){
    
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
        /*
        for(int i = 0; i < results2.size(); i++){
            std::cout << getStreetName(results2[i]) << std::endl;
        }
        */
        //test code, ignore
        resultStreets.second = results2[0];
        std::cout << results2[0] << std::endl;
        
        std:: cout << resultStreets.second << std::endl;
        
        //std::cout << findIntersectionsOfTwoStreets(resultStreets)[0] << std::endl;
                
        
        //save the user's choice into the pair
        //chosen = resultStreets.first (type StreetIdx)
    }
    //application->refresh_drawing();
}

void displayIntersections(GtkWidget *widget, ezgl::application *application){
    
    //consider case where pressed without a valid pair
    //if valid pair, display the intersections
    std::cout << "Find button clicked" << std::endl;
    std:: cout << "The pair of streetIds are: " << resultStreets.first << " and " << resultStreets.second << std::endl;
    
}


void initial_setup(ezgl::application *application, bool /*new_window*/){
    g_signal_connect(application->get_object("SearchStreet1"), "activate", G_CALLBACK(searchFirstStreet), application);
    g_signal_connect(application->get_object("SearchStreet2"), "activate", G_CALLBACK(searchSecondStreet), application);
    g_signal_connect(application->get_object("FindButton"), "clicked", G_CALLBACK(displayIntersections), application);
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


void draw_important_POIs(ezgl::renderer *g, int i, double font){
    g->set_text_rotation(0);
        
    ezgl::rectangle scope = g->get_visible_world();
    double scope_min_x = scope.m_first.x;
    double scope_max_x = scope.m_second.x;
    double scope_min_y = scope.m_first.y;
    double scope_max_y = scope.m_second.y;

    ezgl::point2d center_point(POIs[i].x, POIs[i].y + 2);
    bool include = false;
        
    if(POIs[i].x > scope_min_x  && POIs[i].x < scope_max_x && POIs[i].y > scope_min_y  && POIs[i].y < scope_max_y){
        include = true;
    }
        
    std::unordered_map<OSMID, std::string>::const_iterator it = OSMID_nodeType.find(POIs[i].id);
    std::unordered_map<OSMID, std::string>::const_iterator it2 = OSMID_wayType.find(POIs[i].id);
    
    if(include){
        if (it != OSMID_nodeType.end() && (it->second == "hospital")){
             ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/hospital.png");
            draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
        }
        if (it2 != OSMID_wayType.end() && (it2->second == "aerodrome")){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/aerodrome.png");
            draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
        }
        if (it2 != OSMID_wayType.end() && (it2->second == "helipad")){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/helipad.png");
            draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
        }
        if (it2 != OSMID_wayType.end() && (it2->second == "subway_entrance")){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/subway_entrance.png");
            draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
        }
    }
}

//function to draw POIs
void draw_POIs(ezgl::renderer *g, int i, double font){
        float radius = 3;

        g->set_color(ezgl::BLUE);
        g->set_text_rotation(0);
        
        ezgl::rectangle scope = g->get_visible_world();
        double scope_min_x = scope.m_first.x;
        double scope_max_x = scope.m_second.x;
        double scope_min_y = scope.m_first.y;
        double scope_max_y = scope.m_second.y;

        ezgl::point2d center_point(POIs[i].x, POIs[i].y + 2);
        bool include = false;
        
        if(POIs[i] .x > scope_min_x  && POIs[i].x < scope_max_x && POIs[i].y > scope_min_y  && POIs[i].y < scope_max_y){
            include = true;
        }
        
        std::unordered_map<OSMID, std::string>::const_iterator it = OSMID_nodeType.find(POIs[i].id);
        std::unordered_map<OSMID, std::string>::const_iterator it2 = OSMID_wayType.find(POIs[i].id);
        
        if(include){
            if (it != OSMID_nodeType.end() && it->second == "restaurant"){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/restaurant.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it != OSMID_nodeType.end() && (it->second == "school")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/school.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it != OSMID_nodeType.end() && (it->second == "cafe")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/cafe.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it != OSMID_nodeType.end() && (it->second == "bank")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bank.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it != OSMID_nodeType.end() && (it->second == "bus_station")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bus_station.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it != OSMID_nodeType.end() && (it->second == "supermarket")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/supermarket.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it != OSMID_nodeType.end() && (it->second == "hospital")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/hospital.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it2 != OSMID_wayType.end() && (it2->second == "aerodrome")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/aerodrome.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it2 != OSMID_wayType.end() && (it2->second == "helipad")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/helipad.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
            if (it2 != OSMID_wayType.end() && (it2->second == "subway_entrance")){
                ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/subway_entrance.png");
                draw_POI_function(g, center_point, font, png_surface, POIs[i].name);
            }
        }
        
}

void drawSegment(ezgl::renderer *g, StreetSegmentInfo tempInfo, int i){
    if(tempInfo.numCurvePoints == 0){
         g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
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
    if(segInfo.numCurvePoints == 0){
        g->draw_text(center, name);
    }
    else{
        int midPoint = segInfo.numCurvePoints / 2;
        LatLon point;
        if(midPoint % 2 == 0){
            point = getStreetSegmentCurvePoint(i, midPoint);
        }
        else{
            point = getStreetSegmentCurvePoint(i, midPoint - 1);
        }
        double mid_x = x_from_lon(point.longitude());
        double mid_y = y_from_lat(point.latitude());
        ezgl::point2d middlePoint(mid_x, mid_y);
        g->draw_text(middlePoint, name);
    }
}

void draw_main_canvas(ezgl::renderer *g) {
    g->draw_rectangle({0, 0}, {000, 1000});

    ezgl::rectangle scope = g->get_visible_world();
    double scope_length = scope.m_second.x - scope.m_first.x;
    double scope_height = scope.m_second.y - scope.m_first.y;
    //std::cout << scope_length << "  " << scope_height << std::endl;
     
    //drawing streets
    for (int i = 0; i < getNumStreetSegments(); i++) {

        //introduce white streets at a max scope of 12000 height & width        //suggestion. while loop?
        if (scope_length < 7000 && scope_height < 7000) {
            //helper function to set width and color of line
            colourWidthSetter(g, 6, ezgl::WHITE);
            StreetSegmentInfo tempInfo = getStreetSegmentInfo(i);
            //g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
            drawSegment(g, tempInfo, i);
            
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
                drawSegment(g,tempInfo, i);
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
                drawSegment(g, tempInfo, i);
            }  
        }
        
        if (scope_length < 65000 && scope_height < 60000){
            StreetSegmentInfo tempInfo = getStreetSegmentInfo(i);
            std::unordered_map<OSMID, std::string>::const_iterator it = OSMID_wayType.find(tempInfo.wayOSMID);
            if (it != OSMID_wayType.end() && it->second == "motorway"){
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
                g->set_color(ezgl::ORANGE);
                //g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
                drawSegment(g, tempInfo, i);
            }
            if (it != OSMID_wayType.end() && (it->second == "primary" || it->second == "secondary")){
                g->set_line_width(1.5);
                g->set_color(ezgl::WHITE);
                //g->draw_line({streets[i].start_x, streets[i].start_y}, {streets[i].end_x, streets[i].end_y});
                drawSegment(g, tempInfo, i);
            }
        }
        
        if (scope_length < 1500 && scope_height < 1200) {
            if (findStreetSegmentLength(i) > 70 && scope.m_first.x < streets[i].mid_x && scope.m_second.x > streets[i].mid_x && scope.m_first.y < streets[i].mid_y && scope.m_second.y > streets[i].mid_y) {
                ezgl::point2d centerPoint(streets[i].mid_x, streets[i].mid_y);
                StreetSegmentInfo tempInfo = getStreetSegmentInfo(i);
                
                //display the street names
                g->set_text_rotation(streets[i].angle);
                g->set_font_size(8);
                g->set_color(ezgl::BLACK);
                
                if(scope_length < 1000){
                    g->set_font_size(12);
                }
                
                if (streets[i].oneWay){
                    if (streets[i].reverse){
                       //g->draw_text(centerPoint, "< " + streets[i].name + " <");
                       writeStreetName(g, centerPoint, tempInfo, "< " + streets[i].name + " <", i);
                    }
                    else {
                       //g->draw_text(centerPoint, "> " + streets[i].name + " >");
                       writeStreetName(g, centerPoint, tempInfo, "> " + streets[i].name + " >", i);
                    }
                }
                else {
                    //g->draw_text(centerPoint, streets[i].name);
                    writeStreetName(g, centerPoint, tempInfo, streets[i].name, i);
                    
                }
            }
        }
    }
    
                

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

    //drawing POIs
    for (int i = 0; i < POIs.size(); i++) {

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
        /*else{
            draw_important_POIs(g, i, 10);
        }*/
    }

    //drawing intersections
    for (int id = 0; id < intersections.size(); id++) {
        float x = intersections[id].x;
        float y = intersections[id].y;
        float radius = 1;
        if(scope_length < 1800 && scope_length > 800){
            radius = 1.5;
        }

        if (intersections[id].highlight && scope_length < 800) {
            //print name of intersection
            ezgl::point2d center_point(x, y + 5);
            g->set_color(ezgl::BLACK);
            g->set_font_size(13);
            g->draw_text(center_point, intersections[id].name);
            std::cout << "Closest Intersection: " << intersections[id].name << "\n";

            //set color for intersection icon 
            g->set_color(ezgl::RED);
            ezgl::point2d center(x, y);
            g->fill_arc(center, radius, 0, 360);
        } 
        else if(intersections[id].highlight == false && scope_length < 800){
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

    if (intersections[id].highlight == true)
        intersections[id].highlight = false;
    else
        intersections[id].highlight = true;

    app -> refresh_drawing();
}


void drawMap() {
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
    for (int i = 0; i < getNumIntersections(); i++) {
        intersections[i].name = getIntersectionName(i);

        max_lat = std::max(max_lat, getIntersectionPosition(i).latitude());
        min_lat = std::min(min_lat, getIntersectionPosition(i).latitude());
        max_lon = std::max(max_lon, getIntersectionPosition(i).longitude());
        min_lon = std::min(min_lon, getIntersectionPosition(i).longitude());
    }

    //average lat for Cartesian transformation
    avg_lat = (min_lat + max_lat) / 2;

    //change intersection points to Cartesian coordinates
    for (int i = 0; i < getNumIntersections(); i++) {
        intersections[i].x = x_from_lon(getIntersectionPosition(i).longitude());
        intersections[i].y = y_from_lat(getIntersectionPosition(i).latitude());
    }

    
    //set POI database
    POIs.resize(getNumPointsOfInterest());

    for (int i = 0; i < getNumPointsOfInterest(); i++) {
        POIs[i].name = getPOIName(i);
        POIs[i].x = x_from_lon(getPOIPosition(i).longitude());
        POIs[i].y = y_from_lat(getPOIPosition(i).latitude());
        POIs[i].id = getPOIOSMNodeID(i);
    }

    
    //set streets database
    streets.resize(getNumStreetSegments());

    for (int i = 0; i < getNumStreetSegments(); i++) {

        //for each street segment, obtain its intersection IDs "from" and "to"
        //obtain each intersection ID's position via calling getIntersectionPosition (type LatLon)
        LatLon startingSeg = LatLon(getIntersectionPosition(getStreetSegmentInfo(i).from).latitude(), getIntersectionPosition(getStreetSegmentInfo(i).from).longitude());
        LatLon endingSeg = LatLon(getIntersectionPosition(getStreetSegmentInfo(i).to).latitude(), getIntersectionPosition(getStreetSegmentInfo(i).to).longitude());

        //convert LatLon into Cartesian coord and draw line for each segment
        streets[i].start_x = x_from_lon(startingSeg.longitude());
        streets[i].start_y = y_from_lat(startingSeg.latitude());
        streets[i].end_x = x_from_lon(endingSeg.longitude());
        streets[i].end_y = y_from_lat(endingSeg.latitude());
        streets[i].mid_x = 0.5 * (streets[i].start_x + streets[i].end_x);
        streets[i].mid_y = 0.5 * (streets[i].start_y + streets[i].end_y);

        
        double start_x = streets[i].start_x, start_y = streets[i].start_y;
        double end_x = streets[i].end_x, end_y = streets[i].end_y;
        
        //set rotation of names
        double rotation = 0;

        //if the x coordinates of start and end are equal
        if (end_x == start_x) {
            rotation = 90;
            
            //if end y and start y are in reverse
            if (end_y < start_y){
                streets[i].reverse = true;
            }
        } 
        else {
            rotation = std::atan(abs((end_y - start_y) / (end_x - start_x))) / kDegreeToRadian;
        }
        
        //setting the correct rotations of the names and figuring out the directions of the ways
        //when from is seen as the origin and to is in first quadrant 
        if (end_x > start_x && end_y > start_y) {
            streets[i].angle = rotation;
        } 
        
        //to is in second quadrant
        if (end_x < start_x && end_y > start_y){
            streets[i].angle = -1 * rotation;
            streets[i].reverse = true;
        }
        
        //to is in third quadrant
        if (end_x < start_x && end_y < start_y){
            streets[i].angle = rotation;
            streets[i].reverse = true;
        }
        
        //to is in fourth quadrant
        if (end_x > start_x && end_y < start_y){
            streets[i].angle = -1 * rotation;
        }
        
        //to is on positive x axis
        if (end_x > start_x && end_y == start_y){
            streets[i].angle = rotation;
        }
        
        //to is on negative x axis
        if (end_x < start_x && end_y == start_y){
            streets[i].angle = rotation;
            streets[i].reverse = true;
        }
        
        //set name of the street
        streets[i].name = getStreetName(getStreetSegmentInfo(i).streetID);
        
        //set one way boolean
        streets[i].oneWay = getStreetSegmentInfo(i).oneWay;
    }
    
    
    //creating an unordered map for OSMID and way pointer         
    for (int i = 0; i < getNumberOfWays(); i++){
            
        //get the way pointer and OSMID
        const OSMWay* OSMWay_ptr = getWayByIndex (i);
        OSMID WayID = OSMWay_ptr->id();
        
        std::string key, value;

        //loop through the tags and push into unordered map when key is highway
        for (int j = 0; j < getTagCount(OSMWay_ptr); j++) {
            
            std::tie(key, value) = getTagPair(OSMWay_ptr, j);
            
            if (key == "highway" || key == "aeroway" || key == "railway")
                OSMID_wayType[WayID] = value;
        }
    }
    
    for (int i = 0; i < getNumberOfNodes(); i++){
            
        //get the node pointer and OSMID
        const OSMNode* OSMNode_ptr = getNodeByIndex(i);
        OSMID NodeID = OSMNode_ptr->id();
        
        std::string key, value;

        //loop through the tags and push into unordered map when key is amenity or shop
        for (int j = 0; j < getTagCount(OSMNode_ptr); j++) {
            
            std::tie(key, value) = getTagPair(OSMNode_ptr, j);
            
            if(key == "amenity" || key == "shop"){
                OSMID_nodeType[NodeID] = value;
            }
        }
    }

    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)},
    {
        x_from_lon(max_lon), y_from_lat(max_lat)
    });
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world, ezgl::color(230, 230, 230));

    application.run(initial_setup, act_on_mouse_click, nullptr, nullptr);
}

