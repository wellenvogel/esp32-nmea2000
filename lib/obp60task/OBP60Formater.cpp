#include <Arduino.h>
#include "GwApi.h"
#include "Pagedata.h"

// ToDo
// simulation data
// hold values by missing data
// different unit convertion

FormatedData formatValue(GwApi::BoatValue *value){
    FormatedData result;

    if (! value->valid){
        result.svalue = "---";
        result.unit = "";
        return result;
    }
    static const int bsize=30;
    char buffer[bsize+1];
    buffer[0]=0;
    if (value->getFormat() == "formatDate"){
        time_t tv=tNMEA0183Msg::daysToTime_t(value->value);
        tmElements_t parts;
        tNMEA0183Msg::breakTime(tv,parts);
        snprintf(buffer,bsize,"%04d/%02d/%02d",parts.tm_year+1900,parts.tm_mon+1,parts.tm_mday);
        result.unit = "";
    }
    else if(value->getFormat() == "formatTime"){
        double inthr;
        double intmin;
        double intsec;
        double val;
        val=modf(value->value/3600.0,&inthr);
        val=modf(val*3600.0/60.0,&intmin);
        modf(val*60.0,&intsec);
        snprintf(buffer,bsize,"%02.0f:%02.0f:%02.0f",inthr,intmin,intsec);
        result.unit = "";
    }
    else if (value->getFormat() == "formatFixed0"){
        snprintf(buffer,bsize,"%.0f",value->value);
         result.unit = "";
    }
    else if (value->getFormat() == "formatCourse"){
        double course = value->value;
        course = course * 57.2958;      // Unit conversion form rad to deg
        result.unit = "Deg";
        if(course < 10){
            snprintf(buffer,bsize,"%2.1f",course);
        }
        if(course >= 10 && course < 100){
            snprintf(buffer,bsize,"%3.1f",course);
        }
        if(course >= 100){
            snprintf(buffer,bsize,"%3.0f",course);
        }
    }
    else if (value->getFormat() == "formatKnots" || value->getFormat() == "formatWind"){
        double speed = value->value;
        speed = speed * 1.94384;      // Unit conversion form m/s to kn
        result.unit = "kn";
        if(speed < 10){
            snprintf(buffer,bsize,"%2.1f",speed);
        }
        if(speed >= 10 && speed < 100){
            snprintf(buffer,bsize,"%3.1f",speed);
        }
        if(speed >= 100){
            snprintf(buffer,bsize,"%3.0f",speed);
        }
    }
    else if (value->getFormat() == "formatRot"){
        double rotation = value->value;
        rotation = rotation * 57.2958;      // Unit conversion form rad/s to deg/s
        result.unit = "deg/s";
        if(rotation < 10){
            snprintf(buffer,bsize,"%2.1f",rotation);
        }
        if(rotation >= 10 && rotation < 100){
            snprintf(buffer,bsize,"%3.1f",rotation);
        }
        if(rotation >= 100){
            snprintf(buffer,bsize,"%3.0f",rotation);
        }
    }
    else if (value->getFormat() == "formatDop"){
        double dop = value->value;
        result.unit = "m";
        if(dop < 10){
            snprintf(buffer,bsize,"%2.1f",dop);
        }
        if(dop >= 10 && dop < 100){
            snprintf(buffer,bsize,"%3.1f",dop);
        }
        if(dop >= 100){
            snprintf(buffer,bsize,"%3.0f",dop);
        }
    }
    else if (value->getFormat() == "formatLatitude"){
        double lat = value->value;
        String latitude = "";
        String latdir = "";
        float degree = int(lat);
        float minute = (lat - degree) * 60;
        float secound = (minute - int(minute)) * 60;
        if(lat > 0){
            latdir = "N";
        }
        else{
            latdir = "S";
        }
        latitude = String(degree,0) + "\" " + String(minute,0) + "' " + String(secound, 4) + " " + latdir;
        result.unit = "";
        strcpy(buffer, latitude.c_str());
    }
    else if (value->getFormat() == "formatLongitude"){
        double lon = value->value;
        String longitude = "";
        String londir = "";
        float degree = int(lon);
        float minute = (lon - degree) * 60;
        float secound = (minute - int(minute)) * 60;
        if(lon > 0){
            londir = "E";
        }
        else{
            londir = "W";
        }
        longitude = String(degree,0) + "\" " + String(minute,0) + "' " + String(secound, 4) + " " + londir;
        result.unit = "";
        strcpy(buffer, longitude.c_str());
    }
    else if (value->getFormat() == "formatDepth"){
        double depth = value->value;
        result.unit = "m";
        if(depth < 10){
            snprintf(buffer,bsize,"%2.1f",depth);
        }
        if(depth >= 10 && depth < 100){
            snprintf(buffer,bsize,"%3.1f",depth);
        }
        if(depth >= 100){
            snprintf(buffer,bsize,"%3.0f",depth);
        }
    }
    else if (value->getFormat() == "kelvinToC"){
        double temp = value->value;
        temp = temp - 273.15;
        result.unit = "degree";
        if(temp < 10){
            snprintf(buffer,bsize,"%2.1f",temp);
        }
        if(temp >= 10 && temp < 100){
            snprintf(buffer,bsize,"%3.1f",temp);
        }
        if(temp >= 100){
            snprintf(buffer,bsize,"%3.0f",temp);
        }
    }
    else if (value->getFormat() == "mtr2nm"){
        double distance = value->value;
        distance = distance * 0.000539957;
        result.unit = "nm";
        if(distance < 10){
            snprintf(buffer,bsize,"%2.1f",distance);
        }
        if(distance >= 10 && distance < 100){
            snprintf(buffer,bsize,"%3.1f",distance);
        }
        if(distance >= 100){
            snprintf(buffer,bsize,"%3.0f",distance);
        }
    }
    else{
        snprintf(buffer,bsize,"%3.0f",value->value);
         result.unit = "";
    }
    buffer[bsize]=0;
    result.svalue = String(buffer);
    return result;
}
