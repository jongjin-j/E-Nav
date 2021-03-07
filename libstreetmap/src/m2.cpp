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

struct intersection_data {
    std::string name;
    double x = 0;
    double y = 0;
    bool highlight = false;
};

struct POI_data {
    std::string name;
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

void colourWidthSetter(ezgl::renderer *x, double width, ezgl::color colorChoice){
    x->set_line_width(width);
    x->set_color(colorChoice);
}

/*void test_button(GtkWidget *widget, ezgl::application *application){
    GtkEntry* text_entry = (GtkEntry*) application->get_object("SearchBar");
    const char* text = gtk_entry_get_text(text_entry);
    
    application->update_message(text);
    std::cout << text << std::endl;

}

void searchEntry(GtkWidget *widget, ezgl::application *application){
    GtkEntry* text_entry = (GtkEntry*) application->get_object("SearchBar");
    const char* searchTerm = gtk_entry_get_text(text_entry);
    application->update_message(searchTerm);
    application->refresh_drawing();
}

g_signal_connect(
        G_OBJECT(SearchBar), "activate", G_CALLBACK(testPrint),NULL
);

void testPrint(){
    std::cout << "testing" << std::endl;
}*/


void draw_main_canvas(ezgl::renderer *g) {
    g->draw_rectangle({0, 0},
    {
        1000, 1000
    });

    ezgl::rectangle scope = g->get_visible_world();
    double scope_length = scope.m_second.x - scope.m_first.x;
    double scope_height = scope.m_second.y - scope.m_first.y;

    //drawing streets
    for (int i = 0; i < getNumStreetSegments(); i++) {

        //introduce white streets at a max scope of 12000 height & width        //suggestion. while loop?
        if (scope_length < 12000 && scope_height < 12000) {
            //helper function to set width and colour of line
            colourWidthSetter(g, 6, ezgl::WHITE);
            g->draw_line({streets[i].start_x, streets[i].start_y},
            {
                streets[i].end_x, streets[i].end_y
            });
            
            
            //introduce boredered streets if scope within approx 2000
            if (scope_length < 2000 && scope_height < 1700) {
                colourWidthSetter(g, 6, ezgl::color(130,130,130));
                g->set_line_cap(ezgl::line_cap::round);
                g->draw_line({streets[i].start_x, streets[i].start_y},
                {
                    streets[i].end_x, streets[i].end_y
                });
                //then draw the street in white
                colourWidthSetter(g, 4, ezgl::WHITE);
                g->draw_line({streets[i].start_x, streets[i].start_y},
                {
                    streets[i].end_x, streets[i].end_y
                });
            }
        }
        if (scope_length < 1500 && scope_height < 1200 && findStreetSegmentLength(i) > 70) {
            if (scope.m_first.x < streets[i].mid_x && scope.m_second.x > streets[i].mid_x && scope.m_first.y < streets[i].mid_y && scope.m_second.y > streets[i].mid_y) {
                ezgl::point2d centerPoint(streets[i].mid_x, streets[i].mid_y);
                
                //set so that street name is displayed once every 4 blocks
                if(i % 4 == 0){g->set_text_rotation(streets[i].angle);
                g->set_font_size(10);
                g->set_color(ezgl::BLACK);
                g->draw_text(centerPoint, streets[i].name);
                };
            }
        }
    }

    //drawing features
    for (int i = 0; i < getNumFeatures(); i++) {

        //if it's a closed feature
        if (getFeaturePoint(i, 0) == getFeaturePoint(i, getNumFeaturePoints(i) - 1)) {

            //declare vector of 2d points
            std::vector<ezgl::point2d> featurePoints;

            featurePoints.resize(getNumFeaturePoints(i), {
                0, 0
            });

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
                    if(scope_length < 15000 && scope_height < 15000){
                        g->set_color(chooseFeatureColour(getFeatureType(i)));
                        g->fill_poly(featurePoints);
                    }
                }
                else{g->set_color(chooseFeatureColour(getFeatureType(i)));
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
                g->set_color(chooseFeatureColour(getFeatureType(i)));
                g->set_line_cap(ezgl::line_cap::butt);
                g->draw_line({xCoord, yCoord},
                {
                    x_from_lon(getFeaturePoint(i, j + 1).longitude()), y_from_lat(getFeaturePoint(i, j + 1).latitude())
                });
            }

        }
    }

    //drawing POIs
    for (int i = 0; i < POIs.size(); i++) {
        float radius = 5;

        g->set_color(ezgl::BLUE);
        g->set_text_rotation(0);

        ezgl::point2d center(POIs[i].x, POIs[i].y);

        g->fill_elliptic_arc(center, radius, radius, 0, 360);

        ezgl::point2d center_point(POIs[i].x, POIs[i].y + 7.5);

        if (scope_length < 85 && scope_height < 70) {
            g->set_color(ezgl::BLACK);
            g->set_font_size(16);
            g->draw_text(center_point, POIs[i].name);
        } else if (scope_length < 240 && scope_height < 185) {
            g->set_color(ezgl::BLACK);
            g->set_font_size(13);
            g->draw_text(center_point, POIs[i].name);
        } else if (scope_length < 385 && scope_height < 305) {
            g->set_color(ezgl::BLACK);
            g->set_font_size(10);
            g->draw_text(center_point, POIs[i].name);
        } else if (scope_length < 650 && scope_height < 505) {
            g->set_color(ezgl::BLACK);
            g->set_font_size(8);
            g->draw_text(center_point, POIs[i].name);
        }
    }

    //drawing intersections
    for (int id = 0; id < intersections.size(); id++) {
        float x = intersections[id].x;
        float y = intersections[id].y;
        float width = 10;
        float height = width;

        if (intersections[id].highlight && scope_length < 800) {
            //print name of intersection
            ezgl::point2d center_point(x, y + 7.5);
            g->set_color(ezgl::BLACK);
            g->set_font_size(13);
            g->draw_text(center_point, intersections[id].name);
            std::cout << "Closest Intersection: " << intersections[id].name << "\n";

            //set color for intersection icon 
            g->set_color(ezgl::RED);
        } else {
            g->set_color(ezgl::GREY_55);
        }

        g->fill_rectangle({x - width / 2, y - height / 2},
        {
            x + width / 2, y + height / 2
        });
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
    }

    //
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

        double rotation = 0;

        if (streets[i].end_x == streets[i].start_x) {
            rotation = 90;
        } else {
            rotation = std::atan(abs((streets[i].end_y - streets[i].start_y) / (streets[i].end_x - streets[i].start_x))) / kDegreeToRadian;
        }

        if ((streets[i].end_x > streets[i].start_x && streets[i].end_y > streets[i].start_y) || (streets[i].end_x < streets[i].start_x && streets[i].end_y < streets[i].start_y)) {
            streets[i].angle = rotation;
        } else {
            streets[i].angle = -1 * rotation;
        }

        streets[i].name = getStreetName(getStreetSegmentInfo(i).streetID);

        //setting the types of street segments


        StreetSegmentInfo ss_info = getStreetSegmentInfo(i);

        /*
        //for (int j = 0; j < getNumberOfWays(); j++){
        bool found = false; 
        int j = 0;
        while (!found){    
            const OSMWay* e = getWayByIndex(j);
            
            OSMID id1 = ss_info.wayOSMID;
            OSMID id2 = e->id();
            if (ss_info.wayOSMID == e -> id()){
                found = true;
                int k = 1;
                
                std::string key, value;
                std::tie(key, value) = getTagPair(e, 0);

                while (key != "highway"){
                    std::tie(key,value) = getTagPair(e, k);
                    k++;
                }
                    
                streets[i].street_type = value;
            }
            
            j++;
        }
         
        
        for (int ss_num = 0; ss_num < getNumStreetSegments(); ss_num++) {
            StreetSegmentInfo ss_info = getStreetSegmentInfo(ss_num);

            for (int j = 0; j < getNumberOfWays(); j++) {
                const OSMWay* e = getWayByIndex(j);

                if (ss_info.wayOSMID == e -> id()) {
                    int k = 1;
                    std::string key, value;
                    std::tie(key, value) = getTagPair(e, 0);

                    while (key != "highway") {
                        std::tie(key, value) = getTagPair(e, k);
                        k++;
                    }

                    streets[ss_num].type = value;
                }
            }
        }
    }
         */
    }

    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)},
    {
        x_from_lon(max_lon), y_from_lat(max_lat)
    });
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world, ezgl::color(230, 230, 230));

    application.run(nullptr, act_on_mouse_click, nullptr, nullptr);
}

