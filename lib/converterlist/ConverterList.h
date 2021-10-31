#ifndef _NMEA0183CONVERTERLIST_H
#define _NMEA0183CONVERTERLIST_H

#include "WString.h"
#include "ArduinoJson.h"
#include <map>

template <class T, class Msg>
class ConverterList
{
public:
    typedef void (T::*Converter)(const Msg &msg);
    typedef void (*LConverter)(const Msg &msg, T *);

private:
    double dummy;
    class ConverterEntry
    {
    public:
        unsigned long count = 0;
        unsigned long *pgn;
        unsigned int numPgn = 0;
        Converter converter = NULL;
        LConverter lconverter = NULL;
        ConverterEntry()
        {
            pgn = NULL;
            converter = NULL;
        }
        ConverterEntry(unsigned long pgn, LConverter cv)
        {
            lconverter = cv;
            this->pgn = new unsigned long[1];
            this->pgn[0] = pgn;
        }
        ConverterEntry(unsigned long pgn, Converter cv = NULL)
        {
            converter = cv;
            numPgn = 1;
            this->pgn = new unsigned long[1];
            this->pgn[0] = pgn;
        }
        ConverterEntry(unsigned long pgn1, unsigned long pgn2, Converter cv = NULL)
        {
            converter = cv;
            numPgn = 2;
            this->pgn = new unsigned long[2];
            this->pgn[0] = pgn1;
            this->pgn[1] = pgn2;
        }
        ConverterEntry(unsigned long pgn1, unsigned long pgn2, LConverter cv = NULL)
        {
            lconverter = cv;
            numPgn = 2;
            this->pgn = new unsigned long[2];
            this->pgn[0] = pgn1;
            this->pgn[1] = pgn2;
        }
        ConverterEntry(unsigned long pgn1, unsigned long pgn2, unsigned long pgn3, Converter cv = NULL)
        {
            converter = cv;
            numPgn = 3;
            this->pgn = new unsigned long[3];
            this->pgn[0] = pgn1;
            this->pgn[1] = pgn2;
            this->pgn[2] = pgn3;
        }
        ConverterEntry(unsigned long pgn1, unsigned long pgn2, unsigned long pgn3, LConverter cv = NULL)
        {
            lconverter = cv;
            numPgn = 3;
            this->pgn = new unsigned long[3];
            this->pgn[0] = pgn1;
            this->pgn[1] = pgn2;
            this->pgn[2] = pgn3;
        }
    };
    typedef std::map<String, ConverterEntry> ConverterMap;
    ConverterMap converters;

public:
    /**
        *  register a converter
        *  each of the converter functions must be registered in the constructor 
        **/
    void registerConverter(unsigned long pgn, Converter converter)
    {
        ConverterEntry e(pgn, converter);
        converters[String(pgn)] = e;
    }
    void registerConverter(unsigned long pgn, LConverter converter)
    {
        ConverterEntry e(pgn, converter);
        converters[String(pgn)] = e;
    }
    void registerConverter(unsigned long pgn, String sentence, Converter converter)
    {
        ConverterEntry e(pgn, converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, String sentence, LConverter converter)
    {
        ConverterEntry e(pgn, converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, unsigned long pgn2, String sentence, Converter converter)
    {
        ConverterEntry e(pgn, pgn2, converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, unsigned long pgn2, String sentence, LConverter converter)
    {
        ConverterEntry e(pgn, pgn2, converter);
        converters[sentence] = e;
    }

    bool handleMessage(String code, const Msg &msg, T *base)
    {
        auto it = converters.find(code);
        if (it != converters.end())
        {
            (it->second).count++;
            if (it->second.converter)
            {
                //call to member function - see e.g. https://isocpp.org/wiki/faq/pointers-to-members
                ((*base).*((it->second).converter))(msg);
            }
            else
            {
                (*(it->second).lconverter)(msg, base);
            }
        }
        else
        {
            return false;
        }
        return true;
    }

    virtual unsigned long *handledPgns()
    {
        //for now max 3 pgns per converter
        unsigned long *rt = new unsigned long[converters.size() * 3 + 1];
        int idx = 0;
        for (auto it = converters.begin();
             it != converters.end(); it++)
        {
            for (int i = 0; i < it->second.numPgn && i < 3; i++)
            {
                bool found = false;
                for (int e = 0; e < idx; e++)
                {
                    if (rt[e] == it->second.pgn[i])
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    rt[idx] = it->second.pgn[i];
                    idx++;
                }
            }
        }
        rt[idx] = 0;
        return rt;
    }

    int numConverters(){
        return converters.size();
    }
    void toJson(String prefix, JsonDocument &json){
        for (auto it = converters.begin(); it != converters.end(); it++)
        {
            json[prefix][String(it->first)] = it->second.count;
        }
    }
};
#endif