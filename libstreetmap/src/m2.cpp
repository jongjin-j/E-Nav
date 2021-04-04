/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include "rectangle.hpp"
#include <math.h>
#include "globals.h"
#include <typeinfo>
#include <string>
#include <string.h>
#include <unordered_map>
#include "libcurl.h"
#include <iomanip>
#include <camera.hpp>
#include <cassert>

#define maxScopeAdjust 1.0001
#define minScopeAdjustLon 1.00001
#define minScopeAdjustLat 0.99993        

extern struct databases database;
std::vector<std::string> fileNames;
std::vector<StreetSegmentIdx> travelPath;    
std::string stringOfDirections;


//number of cities loaded so far (needed in libcurl)
int cityNums = 0;

IntersectionIdx startIntersectionID = 0;
IntersectionIdx destIntersectionID = 0;
bool foundStreet1, foundStreet2;

void setFirstStreet(GtkWidget*, ezgl::application *application);
void setSecondStreet(GtkWidget*, ezgl::application *application);
void resetIntersections(GtkWidget*, ezgl::application *application);
void on_dialog_response(GtkDialog *dialog);
void reloadMap(GtkWidget*, ezgl::application *application);
void displayWeather(GtkWidget*, ezgl::application *application);
void directionPrinter(std::vector<StreetSegmentIdx> pathForDirections);
void displayPath(GtkWidget*, ezgl::application *application);
void selectFrom(GtkWidget*, ezgl::application *application);
void selectTo(GtkWidget*, ezgl::application *application);
std::string cardinalDirections(StreetSegmentIdx curr);
double angleBetweenVectors(vector a, vector b);
std::string leftOrRight(StreetSegmentIdx current, StreetSegmentIdx next);
void displayPathUI(GtkWidget*, ezgl::application *application);
void displayHelp(GtkWidget*, ezgl::application *application);
void displayErrorMessage(GtkWidget*, ezgl::application *application);

int integerRound(int x);

double toKM(double x);

std::string cardinalDirections(StreetSegmentIdx from, StreetSegmentIdx to);

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
        //islands return grey
        return ezgl::color(230,230,230);
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
    return ezgl::color(0,0,0,0);
}

//sets the width and colour for the next line
void colourWidthSetter(ezgl::renderer *x, double width, ezgl::color colorChoice){
    x->set_line_width(width);
    x->set_color(colorChoice);
}
std::vector<StreetIdx> firstSearchResults, secondSearchResults; //stores the user's search results from the first and second search boxes
std::pair<StreetIdx, StreetIdx> resultStreets; //std pair to store the two final chosen streets; this is passed onto the findIntersections function

//callback function of searching the first street
//this is executed each time the signal changes ie. a keyboard input
void searchFirstStreet(GtkWidget *, ezgl::application *application){
    
    //street1 will hold what the user has input so far
    const char* street1 = gtk_entry_get_text((GtkEntry*) application -> get_object("SearchStreet1"));
    
    //firstSearchResults will hold the matching streets so far
    firstSearchResults = findStreetIdsFromPartialStreetName(street1);

    //initialize resultsList for the list store
    auto resultsList = (GtkListStore *)(application->get_object("ResultsList"));
    //reset the list
    gtk_list_store_clear(resultsList);
    //iterator to go over the list
    GtkTreeIter iter;
    
    //give the list of results for the user to choose from
    //display only up to 20 results
    for(int i = 0; i < firstSearchResults.size() && i < 20; i++){
        ///append to the list as you go
        gtk_list_store_append(resultsList, &iter);
        //set the new line by feeding values from firstSearchResults
        gtk_list_store_set(resultsList, &iter, 
                            0, getStreetName(firstSearchResults[i]).c_str(),
                            -1);
        
    }
}

//callback function of searching the first street
//this is executed each time the signal changes ie. a keyboard input
void searchSecondStreet(GtkWidget*, ezgl::application *application){
    
    //street2 will hold what the user has input so far
    const char* street2 = gtk_entry_get_text((GtkEntry*) application -> get_object("SearchStreet2"));
    
    //secondSearchResults will hold the matching streets so far
    secondSearchResults = findStreetIdsFromPartialStreetName(street2);
    
    //initialize resultsList for the list store
    auto resultsList = (GtkListStore *)(application->get_object("ResultsList"));
    //reset the list
    gtk_list_store_clear(resultsList);
    //iterator to go over the list
    GtkTreeIter iter;
    
    //give the list of results for the user to choose from
    //display only up to 20 results
    for(int i = 0; i < secondSearchResults.size() && i < 20; i++){
        //append to the list as you go
        gtk_list_store_append(resultsList, &iter);
        //set the new line by feeding values from firstSearchResults
        gtk_list_store_set(resultsList, &iter, 
                            0, getStreetName(secondSearchResults[i]).c_str(),
                            -1); 
    }
}

//this is called when "enter" is pressed in the search bar, to check if the input yields any matches
void setFirstStreet(GtkWidget*, ezgl::application *application){
    //this executes when the user hits enter; hence check if it's a valid street
    //otherwise prompt the user for a complete input
    
    //take the input
    std::string firstStreet = gtk_entry_get_text((GtkEntry*) application -> get_object("SearchStreet1"));
    
    //initialize bool value to false, set streetID to garbage value so it ensures a correct value must be set
    foundStreet1 = false;
    int matchingStreetID = -1;
    
    for(int i = 0; i < getNumStreets(); i++){
        //if no streets found, break without action
        if(findStreetIdsFromPartialStreetName(firstStreet).size() == 0){
            break;
        }else if(firstStreet == getStreetName(i) || getStreetName(findStreetIdsFromPartialStreetName(firstStreet)[0]) == getStreetName(i)){
            //if street exactly matches OR, if there is at least 1 match, take the first result
            matchingStreetID = i;
            foundStreet1 = true;
            break;
        }
    }
    if(foundStreet1 == false){
        //if the for loop didn't find any matches
        std::cout << "The street does not exist" << std::endl;
    }else if(foundStreet1 == true){
        //if matched, update values
        resultStreets.first = matchingStreetID;
        std::cout << "Street 1 is " << getStreetName(resultStreets.first) << "." << std::endl;
    }
}

//identical function as setFirstStreet, but with the second street
void setSecondStreet(GtkWidget*, ezgl::application *application){
    //this executes when the user hits enter; hence check if it's a valid street
    //otherwise prompt the user for a complete input
    std::string secondStreet = gtk_entry_get_text((GtkEntry*) application -> get_object("SearchStreet2"));
    
    foundStreet2 = false;
    int matchingStreetID = -1;
    
    for(int i = 0; i < getNumStreets(); i++){
        if(findStreetIdsFromPartialStreetName(secondStreet).size() == 0){
            break;
        }else if(secondStreet == getStreetName(i) || getStreetName(findStreetIdsFromPartialStreetName(secondStreet)[0]) == getStreetName(i)){
            matchingStreetID = i;
            foundStreet2 = true;
            break;
        }
    }
    if(foundStreet2 == false){
        std::cout << "The street does not exist" << std::endl;
    }else if(foundStreet2 == true){
        resultStreets.second = matchingStreetID;
        std::cout << "Street 2 is " << getStreetName(resultStreets.second) << "." << std::endl;
    }

}

//called when Find Intersections button is clicked
void displayIntersections(GtkWidget*, ezgl::application *application){
    //stringOfDirections
    //deal with different cases
    if(findIntersectionsOfTwoStreets(resultStreets).size() == 0){
        //if no intersections found
        std::cout << "No intersections found between the streets "<< std::endl;
    }else if(getStreetName(resultStreets.first) == "<unknown>" || getStreetName(resultStreets.second) == "<unknown>"){
        //corner case where either street is unknown
        std::cout << "One or more inputs are unknown streets, please try again." << std::endl;
    }else if(findIntersectionsOfTwoStreets(resultStreets).size() > 0){
        //if intersections found, highlight those intersections
        for(int i = 0; i < findIntersectionsOfTwoStreets(resultStreets).size(); i++){
            database.intersections[findIntersectionsOfTwoStreets(resultStreets)[i]].highlight = 1;
        }
        //just for clarity
        std::cout << "Total number of intersections: " << findIntersectionsOfTwoStreets(resultStreets).size() << std::endl;
    }
 
    application->refresh_drawing();
}

//for closing the dialog which pops up when there's an erroneous load map name
void on_dialog_response(GtkDialog *dialog) {
gtk_widget_destroy(GTK_WIDGET(dialog));
}

//reload the map to show other cities
void reloadMap(GtkWidget*, ezgl::application *application){
    //true if city name is found in the directory
    bool foundMatch = false;
    
    //cityName is a char array that holds user input --> can use to call a new map
    //should check for invalid inputs
    const char* cityNameChar = gtk_entry_get_text((GtkEntry*) application -> get_object("LoadCity"));
   
    std::string cityName(cityNameChar);
        
    //make the input lowercase and erase white spaces
    cityName.erase(std::remove(cityName.begin(), cityName.end(), ' '), cityName.end()); //code snippet from https://stackoverflow.com/questions/20326356/how-to-remove-all-the-occurrences-of-a-char-in-c-string
    std::transform(cityName.begin(), cityName.end(), cityName.begin(), ::tolower); // code snippet from https://www.geeksforgeeks.org/conversion-whole-string-uppercase-lowercase-using-stl-c/
            
    /*
    //load OSM database
    for (int i = 0; i < fileNames.size(); i++){
        
        //parse file name
        std::string fileName = fileNames[i];
        
        //if file is osm.bin
        if (fileName.find("osm.bin") != std::string::npos){
            
            //if _ is found (not singapore, iceland)
            if (fileName.find("_") != std::string::npos){
                    fileName.erase(fileName.find("_"), fileName.length() - fileName.find("_"));
            }
            else {
                    fileName.erase(fileName.find("."), fileName.length() - fileName.find("."));
            }
        }
        
        
        //if found a matching city name
        if (cityName == fileName){
            foundMatch = true;
            
            //close current map
            closeOSMDatabase();
            closeMap();
            
            //load OSMDatabase
            bool loadSuccess = loadOSMDatabaseBIN("/cad2/ece297s/public/maps/" + fileNames[i]);
            if (loadSuccess) {
                std::cout << "successfully loaded OSM database" << std::endl; 
            }
            break;
        }
    }
    */
    
    //load streets database
    for (int i = 0; i < fileNames.size(); i++){
        
        //parse file name
        std::string fileName = fileNames[i];
        
        
        //if file is streets.bin
        if (fileName.find("streets.bin") != std::string::npos){
            
            //if _ is found (not singapore, iceland)
            if (fileName.find("_") != std::string::npos){
                fileName.erase(fileName.find("_"), fileName.length() - fileName.find("_"));
            }
            else {
                fileName.erase(fileName.find("."), fileName.length() - fileName.find("."));
            }
        }
        
        //if found a matching city name 
        if (cityName == fileName){
            foundMatch = true;
            
            //close current map
            //closeOSMDatabase();
            closeMap();
            
            //load the new map
            bool loadMapSuccess = loadMap("/cad2/ece297s/public/maps/" + fileNames[i]);
            if (loadMapSuccess) {
                std::cout << "successfully loaded map" << std::endl; 
                
                //load weather
                cityNums += 6;
                loadCityWeatherData(fileNames[i]);
            }
            break;
        }
    }
    
    //if the input name is valid
    if (foundMatch){
        
        //set the new dimensions
        ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)},
        {
        x_from_lon(max_lon), y_from_lat(max_lat)
        });
    
        application->change_canvas_world_coordinates("MainCanvas", initial_world);

        //refresh the drawing
        application -> refresh_drawing();
        
       
        cityName = gtk_entry_get_text((GtkEntry*) application -> get_object("LoadCity"));
        //std::cout << cityName << std::endl;
    }
    else{
        
        GObject *window = application->get_object(application->get_main_window_id().c_str()); //pointer to main application window
        GtkWidget *content_area; // the content area of the dialog
        GtkWidget *label; // writing inside the box
        GtkWidget *dialog; // dialog box to be created
                
        //create new dialog, customized
        dialog = gtk_dialog_new_with_buttons(
                "Load Error",
                (GtkWindow*) window,
                GTK_DIALOG_MODAL,
                NULL,
                //added to supress warnings
                GTK_RESPONSE_ACCEPT,
                NULL,
                GTK_RESPONSE_REJECT,
                NULL
                );
        content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        label = gtk_label_new("Invalid map name entered. Please exit and try again.");
        gtk_container_add(GTK_CONTAINER(content_area), label);
        
        gtk_widget_show_all(dialog);
        
        //connect the dialog for when OK is clicked
        g_signal_connect(
                GTK_DIALOG(dialog),
                "response",
                G_CALLBACK(on_dialog_response),
                NULL
                );
        
    }
}

//called when weather button clicked
void displayWeather(GtkWidget*, ezgl::application *application){
        
    //define variables to be used
    
    GObject *window;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget* dialog;
    
    //pointer to main winddow
    window = application -> get_object(application->get_main_window_id().c_str());
    
    dialog = gtk_dialog_new_with_buttons(
            "Weather Conditions",
            (GtkWindow*) window,
            GTK_DIALOG_MODAL,
            ("Close"),
            //added to supress warnings
            GTK_RESPONSE_ACCEPT,
            NULL,
            GTK_RESPONSE_REJECT,
            NULL
            );
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    //string to display on the dialog
    //convert to char array
    std::string line1 = "Temperature: " + std::to_string(weatherData[0+cityNums]) + " (°C)\nFeels Like: " + std::to_string(weatherData[1+cityNums]) + " (°C)\nPressure: " + std::to_string(weatherData[2+cityNums]) + " (hPA)\nHumidity: "; 
    std::string line2 = std::to_string(weatherData[3+cityNums]) + " (Rh)\nWind Speed: " + std::to_string(weatherData[4+cityNums]) + " (m/s)\nWind Direction: " + std::to_string(weatherData[5+cityNums]) + " (°)";
    std::string displayText = line1 + line2;
    char *displayCharArray = new char[displayText.length()+1];
    strcpy(displayCharArray,displayText.c_str());
    
    //input char array into the label
    label = gtk_label_new(displayCharArray);
    //temperature (celsius) 0, feels like (celsius) 1, pressure (hPa) 2, humidity (g/m^3) 3, wind speed (m/s) 4, wind degrees (deg) 5
    
    gtk_container_add(GTK_CONTAINER(content_area), label);
    
    gtk_widget_show_all(dialog);
    
    //connect the OK button
    g_signal_connect(
        GTK_DIALOG(dialog),
        "response",
        G_CALLBACK(on_dialog_response),
        NULL
    );
    application -> refresh_drawing();
    delete[]displayCharArray;
}


//declaring the intersection as the starting point
void selectFrom(GtkWidget*, ezgl::application *application){
    
    //count variable to ensure only one intersection is highlighted
    int count = 0;
    int i;
    //intersection name
    std::string intName;
    for(i = 0; i<getNumIntersections(); i++){
        if((database.intersections[i].highlight == true)){
            if(database.intersections[i].dest == 0){
             intName = database.intersections[i].name;      //store name of intersection
             startIntersectionID = i;                       //store intersection ID
             database.intersections[i].start = 1;           //set boolean start
             count++;                                       //prevent further intersections from being used
            }
             
        }
    }
    //if only one highlighted
    if(count==1){
      std::cout << "Starting intersection is " << intName << std::endl;      
      std::cout<< startIntersectionID << std::endl;
      assert (count == 1);
      //save the destination intersection ID in startIntersectionID
      //later, add visuals to indicate destination
           
      application->refresh_drawing();
     
      //error cases
    }else if(count>1){
        std::cout << "Too many input arguments" << std::endl;
    }else{
        std::cout << "Please select a starting point" << std::endl;
    }
}

//declare the intersection as the ending point (same as selectFrom)
void selectTo(GtkWidget*, ezgl::application *application){
    //make sure only one intersection is highlighted
    //return the id of that intersection
    int count = 0;
    std::string intName;
    for(int i = 0; i<getNumIntersections(); i++){
        if((database.intersections[i].highlight == true)){
            if(database.intersections[i].start == 0){
               intName = database.intersections[i].name;
               destIntersectionID = i;                    //store intersection ID
               database.intersections[i].dest = 1;
               count++; 
            }
        }
    }
    if(count==1){
        assert (count == 1);
      std::cout << "Destination intersection is " << intName << std::endl;
      std::cout<< destIntersectionID << std::endl;
      
      //save the destination intersection ID
      //later, add visuals to indicate destination
      application->refresh_drawing();
      
    }else if(count>1){
        std::cout << "Too many input arguments" << std::endl;
    }else{
        std::cout << "Please select a destination point" << std::endl;
    }   
}

//highlights the path in blue
void displayPath(GtkWidget*, ezgl::application *application){
    int twoValidIDs = 0;
    //if there isn't a start and end, return
    for(int i=0; i<getNumIntersections(); i++){
        if(database.intersections[i].start){
            twoValidIDs += 1;
        }else if(database.intersections[i].dest){
            twoValidIDs +=1;
        }
    }
    if(twoValidIDs!=2){
        return;
    }
    assert(twoValidIDs == 2);
        //call the pathfinder and insert result into the example path
        travelPath = findPathBetweenIntersections(startIntersectionID,destIntersectionID,0);
        assert (travelPath.size() != 0);
        
        if((travelPath.size()==0)|| (travelPath.size()==1)){
            return;
        }

        std::cout << startIntersectionID << " "  << destIntersectionID << std::endl;
        std::cout << "Total time in seconds is " << computePathTravelTime(travelPath,0) << std::endl;

        //adjusting the scope based on the path size
        std::vector<double> latIntersections;
        std::vector<double> lonIntersections;

        for(int i=0; i < travelPath.size();i++){

            lonIntersections.push_back(getIntersectionPosition(getStreetSegmentInfo(travelPath[i]).to).longitude());
            latIntersections.push_back(getIntersectionPosition(getStreetSegmentInfo(travelPath[i]).to).latitude());

        }
        double maxLon = *max_element(lonIntersections.begin(),lonIntersections.end())*(maxScopeAdjust);
        double minLon = *min_element(lonIntersections.begin(),lonIntersections.end())*(minScopeAdjustLon);
        double maxLat = *max_element(latIntersections.begin(),latIntersections.end())*(maxScopeAdjust);
        double minLat = *min_element(latIntersections.begin(),latIntersections.end())*(minScopeAdjustLat);

        ezgl::point2d first = ezgl::point2d(x_from_lon(minLon), y_from_lat(minLat)); //x0,y0
        ezgl::point2d second = ezgl::point2d(x_from_lon(maxLon), y_from_lat(maxLat)); //x1,y1

        ezgl::rectangle r = {first,second};

        application->get_renderer()->set_visible_world(r);
        application->refresh_drawing();

        directionPrinter(travelPath);
    
}

//called by the reset button, removes all highlights from intersections
void resetIntersections(GtkWidget*, ezgl::application *application){
    
    //if highlighted, undo highlight; if not highlighted, move on
    for(int i = 0; i < getNumIntersections(); i++){
        if(database.intersections[i].highlight == 1){
            database.intersections[i].highlight = 0;
        }
        if((database.intersections[i].start == 1) || (database.intersections[i].dest == 1)){
            database.intersections[i].start = 0;
            database.intersections[i].dest = 0;
        }
    }
    
    if(travelPath.size()!=0){
        travelPath.clear();
    }
    //reset the from/to highlights
    startIntersectionID = -1;
    destIntersectionID = -1;
    
    stringOfDirections = "";

    application->refresh_drawing();
}

//rounds the input to 0 at the ones digit
int integerRound(int x){
    if((x%100)==0){
        x-=(x%10);
        return x;
    }
    return x;
}

//converts into km (for large travel distances)
double toKM(double x){
    x = round(x/100)/10;
    return x;
}

//returns a direction (N,S,W,E)
std::string cardinalDirections(StreetSegmentIdx curr){
    assert (curr>-1);
    std::string northSouth, westEast;
    LatLon from = getIntersectionPosition(getStreetSegmentInfo(curr).from);
    LatLon to = getIntersectionPosition(getStreetSegmentInfo(curr).to);

    //you have the current segment; compare from/to to see which is the one at the other end
    
    //if seg.from is start
    if(from == getIntersectionPosition(startIntersectionID)){
        
        if(x_from_lon(from.longitude()) > x_from_lon(to.longitude())){
            westEast = "west";
        }else{
            westEast = "east";
        }
        if(y_from_lat(from.latitude()) < y_from_lat(to.latitude())){
            northSouth = "North";
        }else{
            northSouth = "South";
        }
        
    }else if(to == getIntersectionPosition(startIntersectionID)){
        
        if(x_from_lon(from.longitude()) > x_from_lon(to.longitude())){
            westEast = "east";
        }else{
            westEast = "west";
        }
        if(y_from_lat(from.latitude()) < y_from_lat(to.latitude())){
            northSouth = "South";
        }else{
            northSouth = "North";
        }
        
    }
    
    return northSouth + westEast;
}

//computes cosine of vectors and returns angle
double angleBetweenVectors(vector a, vector b){
    
    double dotProduct = a.x*b.x + a.y*b.y;
    double determinant = a.x*b.y - b.x*a.y;
    double angle = (atan2(dotProduct,determinant) / kDegreeToRadian)+90;
    //returns angle in degrees
    return angle;
}

//determines turn directions
std::string leftOrRight(StreetSegmentIdx current, StreetSegmentIdx next){

    //if any of these two are equal, that is the meeting point
    //new variable names to simplify code
    IntersectionIdx currFrom = getStreetSegmentInfo(current).from;
    LatLon currFromPos = getIntersectionPosition(currFrom);
    
    IntersectionIdx currTo = getStreetSegmentInfo(current).to;
    LatLon currToPos = getIntersectionPosition(currTo);
            
    IntersectionIdx nextFrom = getStreetSegmentInfo(next).from;
    LatLon nextFromPos = getIntersectionPosition(nextFrom);
    
    IntersectionIdx nextTo = getStreetSegmentInfo(next).to;
    LatLon nextToPos = getIntersectionPosition(nextTo);
    
    vector currSeg, nextSeg;


    currSeg.x = x_from_lon(currToPos.longitude() - currFromPos.longitude());
    currSeg.y = y_from_lat(currToPos.latitude() - currFromPos.latitude());
    nextSeg.x = x_from_lon(nextToPos.longitude() - nextFromPos.longitude());
    nextSeg.y = y_from_lat(nextToPos.latitude() - nextFromPos.latitude());
    
    //orientation possibility 1
    //intersection in common is curr.to and next.from
    if(currTo == nextFrom){
        //initialized values already correspond to this case, move on
    }
    //orientation possibility 2
    //intersection in common is curr.from and next.from
    else if(currFrom == nextFrom){
        //reverse currSeg vector
        currSeg.x = -(currSeg.x);
        currSeg.y = -(currSeg.y);
    }
    //orientation possibility 3
    //intersection in common is curr.to and next.to
    else if(currTo == nextTo){
        //reverse nextSeg vector
        nextSeg.x = -(nextSeg.x);
        nextSeg.y = -(nextSeg.y);
    }
    //orientation possibility 4
    //intersection in commin is curr.to and next.from
    else if(currFrom == nextTo){
        //reverse both vectors
        currSeg.x = -(currSeg.x);
        currSeg.y = -(currSeg.y);
        nextSeg.x = -(nextSeg.x);
        nextSeg.y = -(nextSeg.y);
    }
    
    //two vectors currSeg and nextSeg now oriented to direction of travel
    //declare angle between vectors
    double vectorAngle = angleBetweenVectors(currSeg, nextSeg);
    assert (vectorAngle != 0);
    
    //returns the angle of the vector, measured from currSeg to nextSeg, CCW
    if(vectorAngle > 0){
        assert (vectorAngle < 360);
        if(vectorAngle < 160){
            return "Make a Left";
        }else if(vectorAngle > 200){
            return "Make a Right";
        }else{
            return "Continue straight";
        }
        
    }else if(vectorAngle < 0){
        assert (vectorAngle > -360);
        if(vectorAngle > -160){
            return "Turn right";
        }else if(vectorAngle < -200){
            return "Turn left";
        }else{
            return "Continue straight";
        }
    }
    return "";
}

//prints out directions
void directionPrinter(std::vector<StreetSegmentIdx> pathForDirections){

    double tempDistance = 0;
    std::string currentStreet, nextStreet;
    stringOfDirections = "";
    
    if(pathForDirections.size()==0){
        std::cout << "Error: no input path detected\n";
        stringOfDirections += "Error: no input path detected\n";
    }
    
    else if(pathForDirections.size()==1){
        tempDistance = findStreetSegmentLength(pathForDirections[0]);
        std::string streetName = getStreetName(getStreetSegmentInfo(pathForDirections[0]).streetID);
        if(streetName == "<unknown>"){
            streetName = "a local street";
            assert (streetName!="<unknown>");
        }
        std::cout << "You are currently on " << streetName << "\n";
        std::cout << "Travel " << std::to_string(integerRound((int)tempDistance)) << "m on " << getStreetName(getStreetSegmentInfo(pathForDirections[0]).streetID) << " towards your destination\n ";
        stringOfDirections += "You are currently on " + streetName + "\n";
        stringOfDirections += "Travel " + std::to_string(integerRound((int)tempDistance)) + "m on " + getStreetName(getStreetSegmentInfo(pathForDirections[0]).streetID) + " towards your destination\n ";
        }
    
    else if(pathForDirections.size()>1){
        for(int i=0; i < pathForDirections.size(); i++){
            currentStreet = getStreetName(getStreetSegmentInfo(pathForDirections[i]).streetID);

            if(i!=pathForDirections.size()-1){
                        nextStreet = getStreetName(getStreetSegmentInfo(pathForDirections[i+1]).streetID);
                    }

            tempDistance += findStreetSegmentLength(pathForDirections[i]);
            assert (tempDistance != 0);

            if(i==0){
                if(currentStreet == "<unknown>"){
                    currentStreet = "a local street";
                }
                std::cout << "Head " << cardinalDirections(pathForDirections[i]) << " on " << currentStreet << "\n";
                stringOfDirections += "Head " + cardinalDirections(pathForDirections[i]) + " on " + currentStreet + "\n";
            }

            if(i != pathForDirections.size()-1){
                if(currentStreet != nextStreet){
                    assert (tempDistance != 0);
                    //make a stringstream to set precision of the input
                    if(tempDistance<1000){
                        std::cout<< ">> Travel " << std::to_string(integerRound((int)tempDistance)) << "m\n\n";
                        stringOfDirections += ">> Travel " + std::to_string(integerRound((int)tempDistance)) + "m\n\n";
                    }else if(tempDistance>999){
                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(1) << toKM(tempDistance);
                    std::cout << ">> Travel " << ss.str() << "km\n\n";
                    stringOfDirections += ">> Travel " + ss.str() + "km\n\n";
                    }
                    
                    if(nextStreet == "<unknown>"){
                        nextStreet = "a local street";
                    }
                    std::cout << leftOrRight(pathForDirections[i],pathForDirections[i+1]) << " onto " << nextStreet << "\n";
                    stringOfDirections += leftOrRight(pathForDirections[i],pathForDirections[i+1]) + " onto " + nextStreet + "\n";
                    tempDistance = 0;
                }
            }else if(i == pathForDirections.size()-1){
                if(tempDistance<1000){
                    std::cout << ">> Travel " << std::to_string(integerRound((int)tempDistance)) << "m";
                    stringOfDirections += ">> Travel " + std::to_string(integerRound((int)tempDistance)) + "m";
                }else if(tempDistance>999){
                    std::stringstream mySS;
                    mySS << std::fixed << std::setprecision(1) << toKM(tempDistance);
                    std::cout << ">> Travel " << mySS.str() << "km\n\n";
                    stringOfDirections += ">> Travel " + mySS.str() + "km\n\n";
                }
                std::cout << " to arrive at your destination\n";
                    stringOfDirections += " to arrive at your destination\n";
            }
        } 
    }
}



void displayPathUI(GtkWidget*, ezgl::application *application){
    application->get_renderer();
    if(travelPath.size()==0){
        return;
    }
    assert (travelPath.size()>0);
    
    char *displayCharArray = new char[stringOfDirections.length()+1];
    strcpy(displayCharArray,stringOfDirections.c_str());
    //define variables to be used   
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *scrolledwindow = gtk_scrolled_window_new(NULL, NULL); 
    GtkWidget *label = gtk_label_new(displayCharArray);
 
    gtk_scrolled_window_set_min_content_width
                               ((GtkScrolledWindow*)scrolledwindow,
                                305);
    
    gtk_scrolled_window_set_min_content_height
                               ((GtkScrolledWindow*)scrolledwindow,
                                350);

    gtk_container_add(GTK_CONTAINER(scrolledwindow), label);
    gtk_container_add(GTK_CONTAINER(window), scrolledwindow);
    gtk_widget_show_all(window);
    
    delete[]displayCharArray;
    
}

//displays intructions
void displayHelp(GtkWidget*, ezgl::application *application){
        
    //define variables to be used
    GObject *window;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget* dialog;
    
    //pointer to main winddow
    window = application -> get_object(application->get_main_window_id().c_str());
    
    dialog = gtk_dialog_new_with_buttons(
            "Using the map",
            (GtkWindow*) window,
            GTK_DIALOG_MODAL,
            ("Close"),
            //added to suppress warnings
            GTK_RESPONSE_ACCEPT,
            NULL,
            GTK_RESPONSE_REJECT,
            NULL
            );
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    //string to display on the dialog
    //convert to char array
    std::string row1 = "FINDING INTERSECTIONS:\n1. Enter Street1 and Street2. Make sure you press the enter key.\n";
    std::string row2 = "2. Hit 'Find Intersections'. Corresponding intersections (if any) will be highlighted.";
    std::string row3 = "\n\nFINDING DIRECTIONS:\n1. Highlight your starting intersection and press 'From'.\n2. Highlight your destination intersection and press 'To.";
    std::string row4 = "\n3. Hit 'Go!'.\n4. The path will be highlighted and directions will be displayed.\n\n";
    std::string row5 = "RESETTING:\n1. To reset all intersections and paths, click on 'Reset'.\n2. To reset a specific intersection, click on it.\n";

    std::string displayText = row1 + row2 + row3 + row4 + row5;
    char *displayCharArray = new char[displayText.length()+1];
    strcpy(displayCharArray,displayText.c_str());
    
    //input char array into the label
    label = gtk_label_new(displayCharArray);
    
    gtk_container_add(GTK_CONTAINER(content_area), label);
    
    gtk_widget_show_all(dialog);
    
    //connect the OK button
    g_signal_connect(
        GTK_DIALOG(dialog),
        "response",
        G_CALLBACK(on_dialog_response),
        NULL
    );
    application -> refresh_drawing();
    delete[]displayCharArray;
}



void displayErrorMessage(GtkWidget*, ezgl::application *application){
    
if(foundStreet1 == 1 || foundStreet2 == 1){
    return;
}    
    
    //define variables to be used
    GObject *window;
    GtkWidget *content_area;
    GtkWidget *label;
    GtkWidget* dialog;
    
    //pointer to main winddow
    window = application -> get_object(application->get_main_window_id().c_str());
    
    dialog = gtk_dialog_new_with_buttons(
            "Using the map",
            (GtkWindow*) window,
            GTK_DIALOG_MODAL,
            ("Close"),
            //added to suppress warnings
            GTK_RESPONSE_ACCEPT,
            NULL,
            GTK_RESPONSE_REJECT,
            NULL
            );
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    //string to display on the dialog
    //convert to char array
    std::string row1 = "ERROR:\nNo matching streets found. Try again.\n";

    std::string displayText = row1;
    char *displayCharArray = new char[displayText.length()+1];
    strcpy(displayCharArray,displayText.c_str());
    
    //input char array into the label
    label = gtk_label_new(displayCharArray);
    
    gtk_container_add(GTK_CONTAINER(content_area), label);
    
    gtk_widget_show_all(dialog);
    
    //connect the OK button
    g_signal_connect(
        GTK_DIALOG(dialog),
        "response",
        G_CALLBACK(on_dialog_response),
        NULL
    );
    application -> refresh_drawing();
    delete[]displayCharArray;
}



//initial setup, makes all the connections needed
void initial_setup(ezgl::application *application, bool /*new_window*/){
    //searching street 1 and street 2
    g_signal_connect(application->get_object("SearchStreet1"), "changed", G_CALLBACK(searchFirstStreet), application);
    g_signal_connect(application->get_object("SearchStreet2"), "changed", G_CALLBACK(searchSecondStreet), application);
    //asserting search term by pressing enter
    g_signal_connect(application->get_object("SearchStreet1"), "activate", G_CALLBACK(setFirstStreet), application);
    g_signal_connect(application->get_object("SearchStreet2"), "activate", G_CALLBACK(setSecondStreet), application);
    
    g_signal_connect(application->get_object("SearchStreet1"), "activate", G_CALLBACK(displayErrorMessage), application);
    g_signal_connect(application->get_object("SearchStreet2"), "activate", G_CALLBACK(displayErrorMessage), application);
    //find intersection button
    g_signal_connect(application->get_object("FindButton"), "clicked", G_CALLBACK(displayIntersections), application);
    //reset intersection button
    g_signal_connect(application->get_object("ResetButton"), "clicked", G_CALLBACK(resetIntersections), application);
    //loading new city button
    g_signal_connect(application->get_object("LoadCity"), "activate", G_CALLBACK(reloadMap), application);
    //display weather button
    g_signal_connect(application->get_object("WeatherButton"), "clicked", G_CALLBACK(displayWeather), application);
    g_signal_connect(application->get_object("HelpButton"), "clicked", G_CALLBACK(displayHelp), application);
    //select FROM intersection
    g_signal_connect(application->get_object("FromButton"), "clicked", G_CALLBACK(selectFrom), application);
    //select TO intersection
    g_signal_connect(application->get_object("ToButton"), "clicked", G_CALLBACK(selectTo), application);
    //find path button
    g_signal_connect(application->get_object("findPathButton"), "clicked", G_CALLBACK(displayPath), application);
    
    g_signal_connect(application->get_object("findPathButton"), "clicked", G_CALLBACK(displayPathUI), application);
}

//function to draw a POI

void draw_POI_function(ezgl::renderer *g, ezgl::point2d center_point, double font, ezgl::surface *p, std::string name, double scope_length, double scope_height){
    double x_scope = 0.00632422;
    double y_scope = 0.00938729;
    g->set_text_rotation(0);
    g->draw_surface(p, {center_point.x - x_scope * scope_length, center_point.y});
    ezgl::renderer::free_surface(p);
    g->set_color(ezgl::BLACK);
    g->set_font_size(font);
    ezgl::point2d center(center_point.x, center_point.y + y_scope * scope_height);
    g->draw_text(center, name);
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
    double scope_length = scope_max_x - scope_min_x;
    double scope_height = scope_max_y - scope_min_y;

    ezgl::point2d center_point(database.POIs[i].x, database.POIs[i].y);
    bool include = false;
        
    if(database.POIs[i].x > scope_min_x  && database.POIs[i].x < scope_max_x && database.POIs[i].y > scope_min_y  && database.POIs[i].y < scope_max_y){
        include = true;
    }
        
    //std::unordered_map<OSMID, std::string>::const_iterator it = database.OSMID_nodeType.find(database.POIs[i].id);
    //std::unordered_map<OSMID, std::string>::const_iterator it2 = database.OSMID_wayType.find(database.POIs[i].id);
        
    if(include){
        if (database.POIs[i].type == "restaurant" || database.POIs[i].type == "fast_food" || database.POIs[i].type == "bar"){
        ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/restaurant.png");
        draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "school"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/school.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "toilets"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/toilet.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "post_office"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/post_office.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "police"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/police.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "parking"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/parking.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "fuel"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/fuel.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "cinema"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/cinema.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "cafe"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/cafe.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "bank" || database.POIs[i].type == "atm"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bank.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "hospital" || database.POIs[i].type == "clinic"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/hospital.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "subway_entrance"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/subway_entrance.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "bus_station"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bus_station.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "bus_stop"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bus_station.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "aerodrome"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/aerodrome.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "supermarket"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/supermarket.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else if (database.POIs[i].type == "wholesale"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/supermarket.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        else {
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/BlackPin.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
    }
}

//function to draw more significant POIs
void draw_important_POIs(ezgl::renderer *g, int i, double font){
    g->set_text_rotation(0);
        
    ezgl::rectangle scope = g->get_visible_world();
    double scope_min_x = scope.m_first.x;
    double scope_max_x = scope.m_second.x;
    double scope_min_y = scope.m_first.y;
    double scope_max_y = scope.m_second.y;
    double scope_length = scope_max_x - scope_min_x;
    double scope_height = scope_max_y - scope_min_y;

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
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
        if (database.POIs[i].type == "bus_station"){
            ezgl::surface *png_surface = ezgl::renderer::load_png("libstreetmap/resources/bus_station.png");
            draw_POI_function(g, center_point, font, png_surface, database.POIs[i].name, scope_length, scope_height);
        }
    }
}


//drawing segments of streets
void drawSegment(ezgl::renderer *g, StreetSegmentInfo tempInfo, int i, ezgl::color colorChoice){
    g->set_color(colorChoice);
    
    if(tempInfo.numCurvePoints == 0){
         g->draw_line({database.street_segments[i].start_x, database.street_segments[i].start_y}, {database.street_segments[i].end_x, database.street_segments[i].end_y});
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

//function to label street segments
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

//main canvas
void draw_main_canvas(ezgl::renderer *g) {
    g->draw_rectangle({0, 0}, {1000, 1000});

    ezgl::rectangle scope = g->get_visible_world();
    double scope_length = scope.m_second.x - scope.m_first.x;
    double scope_height = scope.m_second.y - scope.m_first.y;
    
    /*std::cout << "   " << std::endl;
    std::cout << scope.m_second.x << " " << scope.m_first.x << std::endl;
    std::cout << scope.m_second.y << " " << scope.m_first.y << std::endl;
    std::cout << scope_length << std::endl;
    std::cout << scope_height << std::endl;*/

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
            if (findStreetSegmentLength(i) > 70 && scope.m_first.x < database.street_segments[i].mid_x && scope.m_second.x > database.street_segments[i].mid_x && scope.m_first.y < database.street_segments[i].mid_y && scope.m_second.y > database.street_segments[i].mid_y) {
                ezgl::point2d centerPoint(database.street_segments[i].mid_x, database.street_segments[i].mid_y);
                StreetSegmentInfo tempInfo = getStreetSegmentInfo(i);
                
                //display the street names
                g->set_text_rotation(database.street_segments[i].angle);
                g->set_font_size(8);
                g->set_color(ezgl::BLACK);
                
                if(scope_length < 1000){
                    g->set_font_size(12);
                }
                
                if (database.street_segments[i].oneWay){
                    if (database.street_segments[i].reverse){
                       //g->draw_text(centerPoint, "< " + streets[i].name + " <");
                       writeStreetName(g, centerPoint, tempInfo, "< " + database.street_segments[i].name + " <", i);
                    }
                    else {
                       //g->draw_text(centerPoint, "> " + streets[i].name + " >");
                       writeStreetName(g, centerPoint, tempInfo, "> " + database.street_segments[i].name + " >", i);
                    }
                }
                else {
                    //g->draw_text(centerPoint, streets[i].name);
                    writeStreetName(g, centerPoint, tempInfo, database.street_segments[i].name, i);
                    
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
    }
    
    
    //drawing intersections
    for (int id = 0; id < database.intersections.size(); id++) {
        float x = database.intersections[id].x;
        float y = database.intersections[id].y;
        float radius = 1;
        
        //when highlighted, draw all necessary info
        if (database.intersections[id].highlight) {

            //this is where the png will be attached
            ezgl::point2d center(x, y);
            //retrieve png image
            ezgl::surface *png_surface;
            
            double scopeRatioX = 0.00928098;
            double scopeRatioY = 0.05307086;
            double scopeRatioFlagY = 0.049;
            
            if(database.intersections[id].start){
                png_surface = ezgl::renderer::load_png("libstreetmap/resources/RedFlag.png");
                g->draw_surface(png_surface, {center.x, center.y + scopeRatioFlagY * scope_height});
            }else if(database.intersections[id].dest){
                png_surface = ezgl::renderer::load_png("libstreetmap/resources/GreenFlag.png");
                g->draw_surface(png_surface, {center.x, center.y + scopeRatioFlagY * scope_height});
            }else{
                png_surface = ezgl::renderer::load_png("libstreetmap/resources/redpin.png");
                g->draw_surface(png_surface, {center.x - scopeRatioX * scope_length, center.y + scopeRatioY * scope_height});
            }
            //scope ratios are for adjusting where the image/text appears as a function of zoom distance
            //adjust the center point so that the png fits nicely (by default, top left corner of the png is attached to the center point)
            
            //g->draw_surface(png_surface, {center.x - scopeRatioX * scope_length, center.y + scopeRatioY * scope_height});
            ezgl::renderer::free_surface(png_surface);
            
            //print name of intersection
            ezgl::point2d center_point(x, y + scopeRatioY * (1.15) * scope_height);
            g->set_color(ezgl::BLACK);
            g->set_font_size(13);
            g->set_text_rotation(0);
            g->draw_text(center_point, database.intersections[id].name);
            //0.0563237 is also a scope constant as described above
            ezgl::point2d info_point(x, y - (0.0563237) * (scope_height));
            std::string intersectionInfo = "Lon: " + std::to_string(getIntersectionPosition(id).longitude()) + " Lat: " + std::to_string(getIntersectionPosition(id).latitude());
            g->draw_text(info_point, intersectionInfo);
        } 
        else if(database.intersections[id].highlight == false && scope_length < 800){
            g->set_color(ezgl::GREY_55);
            ezgl::point2d center(x, y);
            g->fill_arc(center, radius, 0, 360);
        }
    }

    //drawing path
    if(travelPath.size()!=0){
        for(int i = 0; i < getNumStreetSegments(); i++){
            for(int j = 0; j<travelPath.size(); j++){
                if(i==travelPath[j]){
                g->set_line_width(4);
                g->set_line_cap(ezgl::line_cap::butt);
                StreetSegmentInfo tempInfo = getStreetSegmentInfo(i);
                drawSegment(g, tempInfo, i, ezgl::BLUE);
                }
            }
        }     
    }
    
}

void act_on_mouse_click(ezgl::application* app, GdkEventButton* , double x, double y) {

    LatLon pos = LatLon(lat_from_y(y), lon_from_x(x));
    int id = findClosestIntersection(pos);

    if (database.intersections[id].highlight == true)
        database.intersections[id].highlight = false;
    else
        database.intersections[id].highlight = true;
    
    if (database.intersections[id].start == true)
        database.intersections[id].start = false;
    
    if(database.intersections[id].dest == true)
        database.intersections[id].dest = false;

    app -> refresh_drawing();
}


void drawMap() {
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