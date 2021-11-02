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
        ConverterEntry(int num,unsigned long *pgn, LConverter cv)
        {
            lconverter = cv;
            numPgn=num;
            this->pgn = pgn;
        }
        ConverterEntry(int num,unsigned long *pgn, Converter cv)
        {
            converter = cv;
            numPgn=num;
            this->pgn = pgn;
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
        unsigned long *lpgn=new unsigned long[1]{pgn};
        ConverterEntry e(1,lpgn, converter);
        converters[String(pgn)] = e;
    }
    void registerConverter(unsigned long pgn, LConverter converter)
    {
        unsigned long *lpgn=new unsigned long[1]{pgn};
        ConverterEntry e(1,lpgn, converter);
        converters[String(pgn)] = e;
    }
    void registerConverter(unsigned long pgn, String sentence, Converter converter)
    {
        unsigned long *lpgn=new unsigned long[1]{pgn};
        ConverterEntry e(1,lpgn, converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, String sentence, LConverter converter)
    {
        unsigned long *lpgn=new unsigned long[1]{pgn};
        ConverterEntry e(1,lpgn, converter);
        converters[sentence] = e;
    }
    void registerConverter(int num,unsigned long *lpgn, String sentence, Converter converter)
    {
        ConverterEntry e(num,lpgn, converter);
        converters[sentence] = e;
    }
    void registerConverter(int num,unsigned long *lpgn, String sentence, LConverter converter)
    {
        ConverterEntry e(num,lpgn, converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, unsigned long pgn2, String sentence, Converter converter)
    {
        unsigned long *lpgn=new unsigned long[2]{pgn,pgn2};
        ConverterEntry e(2, lpgn, converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, unsigned long pgn2, String sentence, LConverter converter)
    {
        unsigned long *lpgn=new unsigned long[2]{pgn,pgn2};
        ConverterEntry e(2, lpgn, converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, unsigned long pgn2, unsigned long pgn3,String sentence, Converter converter)
    {
        unsigned long *lpgn=new unsigned long[3]{pgn,pgn2,pgn3};
        ConverterEntry e(3, lpgn,converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, unsigned long pgn2, unsigned long pgn3,String sentence, LConverter converter)
    {
        unsigned long *lpgn=new unsigned long[3]{pgn,pgn2,pgn3};
        ConverterEntry e(3, lpgn,converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, unsigned long pgn2, unsigned long pgn3,unsigned long pgn4,String sentence, Converter converter)
    {
        unsigned long *lpgn=new unsigned long[4]{pgn,pgn2,pgn3,pgn4};
        ConverterEntry e(4, lpgn,converter);
        converters[sentence] = e;
    }
    void registerConverter(unsigned long pgn, unsigned long pgn2, unsigned long pgn3,unsigned long pgn4,String sentence, LConverter converter)
    {
        unsigned long *lpgn=new unsigned long[4]{pgn,pgn2,pgn3,pgn4};
        ConverterEntry e(4, lpgn,converter);
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
        //for now max 4 pgns per converter
        unsigned long *rt = new unsigned long[converters.size() * 4 + 1];
        int idx = 0;
        for (auto it = converters.begin();
             it != converters.end(); it++)
        {
            for (int i = 0; i < it->second.numPgn && i < 4; i++)
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