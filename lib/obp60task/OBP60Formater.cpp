#include <Arduino.h>
#include "GwApi.h"
#include "Pagedata.h"

// ToDo
// simulation data
// hold values by missing data

FormatedData formatValue(GwApi::BoatValue *value, CommonData &commondata){
    FormatedData result;

    // Load configuration values
    int timeZone = commondata.config->getInt(commondata.config->timeZone);                      // [UTC -12...+12]
    String lengthFormat = commondata.config->getString(commondata.config->lengthFormat);        // [m|ft]
    String distanceFormat = commondata.config->getString(commondata.config->distanceFormat);    // [m|km|nm]
    String speedFormat = commondata.config->getString(commondata.config->speedFormat);          // [m/s|km/h|kn]
    String windspeedFormat = commondata.config->getString(commondata.config->windspeedFormat);  // [m/s|km/h|kn|bft]
    String tempFormat = commondata.config->getString(commondata.config->tempFormat);            // [K|°C|°F]
    String dateFormat = commondata.config->getString(commondata.config->dateFormat);            // [DE|GB|US]

    if (! value->valid){
        result.svalue = "---";
        result.unit = "";
        return result;
    }
    static const int bsize = 30;
    char buffer[bsize+1];
    buffer[0]=0;
    if (value->getFormat() == "formatDate"){
        time_t tv=tNMEA0183Msg::daysToTime_t(value->value);
        tmElements_t parts;
        tNMEA0183Msg::breakTime(tv,parts);        
        if(String(dateFormat) == "DE"){
            snprintf(buffer,bsize,"%02d.%02d.%04d",parts.tm_mday,parts.tm_mon+1,parts.tm_year+1900);
        }
        else if(String(dateFormat) == "GB"){
            snprintf(buffer,bsize,"%02d/%02d/%04d",parts.tm_mday,parts.tm_mon+1,parts.tm_year+1900);
        }
        else if(String(dateFormat) == "US"){
            snprintf(buffer,bsize,"%02d/%02d/%04d",parts.tm_mon+1,parts.tm_mday,parts.tm_year+1900);
        }
        else{
            snprintf(buffer,bsize,"%02d.%02d.%04d",parts.tm_mday,parts.tm_mon+1,parts.tm_year+1900);  
        }        
    }
    else if(value->getFormat() == "formatTime"){
        double inthr;
        double intmin;
        double intsec;
        double val;
        val=modf(value->value/3600.0,&inthr);
        inthr += timeZone; 
        val=modf(val*3600.0/60.0,&intmin);
        modf(val*60.0,&intsec);
        snprintf(buffer,bsize,"%02.0f:%02.0f:%02.0f",inthr,intmin,intsec);
        if(timeZone == 0){
            result.unit = "UTC";
        }
        else{
            result.unit = "LOT";
        }
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
            snprintf(buffer,bsize,"%2.1f",course);
        }
        if(course >= 100){
            snprintf(buffer,bsize,"%3.0f",course);
        }
    }
    else if (value->getFormat() == "formatKnots" || value->getFormat() == "formatWind"){
        double speed = value->value;
        if(String(speedFormat) == "km/h" || String(windspeedFormat) == "km/h"){
        speed = speed * 3.6;        // Unit conversion form m/s to km/h
            result.unit = "m/s";
        }
        else if(String(speedFormat) == "kn" || String(windspeedFormat) == "kn"){
            speed = speed * 1.94384;      // Unit conversion form m/s to kn
            result.unit = "kn";
        }
        else if(String(windspeedFormat) == "bft"){
            if(speed < 0.3){
                speed = 0;
            }
            if(speed >=0.3 && speed < 1.5){
                speed = 1;
            }
            if(speed >=1.5 && speed < 3.3){
                speed = 2;
            }
            if(speed >=3.3 && speed < 5.4){
                speed = 3;
            }
            if(speed >=5.4 && speed < 7.9){
                speed = 4;
            }
            if(speed >=7.9 && speed < 10.7){
                speed = 5;
            }
            if(speed >=10.7 && speed < 13.8){
                speed = 6;
            }
            if(speed >=13.8 && speed < 17.1){
                speed = 7;
            }
            if(speed >=17.1 && speed < 20.7){
                speed = 8;
            }
            if(speed >=20.7 && speed < 24.4){
                speed = 9;
            }
            if(speed >=24.4 && speed < 28.4){
                speed = 10;
            }
            if(speed >=28.4 && speed < 32.6){
                speed = 11;
            }
            if(speed >=32.6){
                speed = 12;
            }
            result.unit = "bft";
        }
        else{
            speed = speed;              // Unit conversion form m/s to m/s
            result.unit = "m/s";
        }
        if(String(windspeedFormat) == "bft"){
            snprintf(buffer,bsize,"%2.0f",speed);
        }
        else{
            if(speed < 10){
                snprintf(buffer,bsize,"%2.1f",speed);
            }
            if(speed >= 10 && speed < 100){
                snprintf(buffer,bsize,"%2.1f",speed);
            }
            if(speed >= 100){
                snprintf(buffer,bsize,"%3.0f",speed);
            }
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
            snprintf(buffer,bsize,"%2.1f",rotation);
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
            snprintf(buffer,bsize,"%2.1f",dop);
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
        if(String(lengthFormat) == "ft"){
            depth = depth * 3.28084;
            result.unit = "ft";
        }
        else{
             result.unit = "m";
        }
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
        if(String(tempFormat) == "°C"){
            temp = temp - 273.15;
            result.unit = "C";
        }
        else if(String(tempFormat) == "°F"){
            temp = temp - 459.67;
            result.unit = "F";
        }
        else{
            result.unit = "K";
        }
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
        if(String(distanceFormat) == "km"){
            distance = distance * 0.001;
            result.unit = "km";
        }
        else if(String(distanceFormat) == "nm"){
            distance = distance * 0.000539957;
            result.unit = "nm";
        }
        else{
            distance = distance * 0.000539957;
            result.unit = "m";
        }
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
