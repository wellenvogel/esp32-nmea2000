#include "GwXDRMappings.h"
#include "N2kMessages.h"
double PtoBar(double v)
{
    if (N2kIsNA(v))
        return v;
    return v / 100000L;
}
double BarToP(double v)
{
    if (N2kIsNA(v))
        return v;
    return v * 100000L;
}
double ltrTom3(double v)
{
    if (N2kIsNA(v))
        return v;
    return v / 1000.0;
}
double m3ToL(double v)
{
    if (N2kIsNA(v))
        return v;
    return v * 1000.0;
}
double ph2ps(double v)
{
    if (N2kIsNA(v))
        return v;
    return v * 3600.0;
}
double ps2ph(double v)
{
    if (N2kIsNA(v))
        return v;
    return v / 3600.0;
}
//https://www.nmea.org/Assets/20190916%20Standards%20Update%20NMEA%20Conferencev2.pdf
GwXDRType *types[] = {
    new GwXDRType(GwXDRType::PRESS, "P", "P"),
    new GwXDRType(GwXDRType::PRESS, "P", "B",
                  PtoBar,
                  BarToP),
    new GwXDRType(GwXDRType::VOLT, "U", "V"),
    new GwXDRType(GwXDRType::AMP, "I", "A"),
    new GwXDRType(GwXDRType::TEMP, "C", "K"),
    new GwXDRType(GwXDRType::TEMP, "C", "C", CToKelvin, KelvinToC),
    new GwXDRType(GwXDRType::HUMID, "H", "P"), //percent
    new GwXDRType(GwXDRType::VOLPERCENT, "V", "P"),
    new GwXDRType(GwXDRType::VOLUME, "V", "M", m3ToL, ltrTom3),
    new GwXDRType(GwXDRType::FLOW, "R", "I", ps2ph, ph2ps),
    //important to have 2x NULL!
    NULL,
    NULL};

static GwXDRType *findType(GwXDRType::TypeCode type, int *start = NULL)
{
    int from = 0;
    if (start != NULL)
        from = *start;
    if (types[from] == NULL)
        return NULL;
    int i = from;
    for (; types[i] != NULL; i++)
    {
        if (types[i]->code == type)
        {
            if (start != NULL)
                *start = i + 1;
            return types[i];
        }
    }
    if (start != NULL)
        *start = i;
    return NULL;
}

#include "GwXdrTypeMappings.h"

static GwXDRType::TypeCode findTypeMapping(GwXDRCategory category, int field)
{
    for (int i = 0; typeMappings[i] != NULL; i++)
    {
        if (typeMappings[i]->fieldIndex == field &&
            typeMappings[i]->category == category)
        {
            return typeMappings[i]->type;
        }
    }
    return GwXDRType::UNKNOWN;
}

String GwXDRMappingDef::toString()
{
    String rt = "";
    rt += String((int)category);
    rt += ",";
    rt += String((int)direction);
    rt += ",";
    rt += String(selector);
    rt += ",";
    rt += String(field);
    rt += ",";
    rt += String(instanceMode);
    rt += ",";
    rt += String(instanceId);
    return rt;
}
bool GwXDRMappingDef::handleToken(String tok, int index, GwXDRMappingDef *def)
{
    switch (index)
    {
        int i;
    case 0:
        //category
        i = tok.toInt();
        if (i < XDRTEMP || i > XDRENGINE)
            return false;
        def->category = (GwXDRCategory)i;
        return true;
    case 1:
        //direction
        i = tok.toInt();
        if (i < GwXDRMappingDef::M_DISABLED ||
            i >= GwXDRMappingDef::M_LAST)
            return false;
        def->direction = (GwXDRMappingDef::Direction)i;
        return true;
    case 2:
        //selector
        //TODO: check selector?
        i = tok.toInt();
        def->selector = i;
        return true;
    case 3:
        //field
        i = tok.toInt();
        def->field = i;
        return true;
    case 4:
        //instance mode
        i = tok.toInt();
        if (i < GwXDRMappingDef::IS_SINGLE ||
            i >= GwXDRMappingDef::IS_LAST)
            return false;
        def->instanceMode = (GwXDRMappingDef::InstanceMode)i;
        return true;
    case 5:
        //instance id
        i = tok.toInt();
        def->instanceId = i;
        return true;
    default:
        break;
    }
    return false;
}
GwXDRMappingDef *GwXDRMappingDef::fromString(String s)
{
    int found = 0;
    int last = 0;
    int index = 0;
    GwXDRMappingDef *rt = new GwXDRMappingDef();
    while ((found = s.indexOf(',', last)) >= 0)
    {
        String tok = s.substring(last, found);
        if (!handleToken(tok, index, rt))
        {
            delete rt;
            return NULL;
        }
        last = found + 1;
        index++;
    }
    if (last < s.length())
    {
        String tok = s.substring(last);
        if (!handleToken(tok, index, rt))
        {
            delete rt;
            return NULL;
        }
    }
    return rt;
}

GwXDRMappings::GwXDRMappings(GwLog *logger, GwConfigHandler *config)
{
    this->logger = logger;
    this->config = config;
}

#define MAX_MAPPINGS 100
void GwXDRMappings::begin()
{
    char namebuf[10];
    /*
        build our mappings
        for each defined mapping we fetch the type code and type definition
        and create a mapping entries in our 2 maps:
        n2kmap: map category,selector and field index to a mapping
        n183map: map transducer name, transducer type and transducer unit to a mapping
                 a #nnn will be stripped from the transducer name
        entries in the map are lists of mappings as potentially
        we have different mappings for different instances         
    */
    for (int i = 0; i < MAX_MAPPINGS; i++)
    {
        snprintf(namebuf, 9, "XDR%d", i);
        namebuf[9] = 0;
        GwConfigInterface *cfg = config->getConfigItem(String(namebuf));
        if (cfg)
        {
            GwXDRMappingDef *def = GwXDRMappingDef::fromString(cfg->asCString());
            if (def)
            {
                int typeIndex = 0;
                LOG_DEBUG(GwLog::DEBUG, "read xdr mapping %s", def->toString().c_str());
                //n2k: find first matching type mapping
                GwXDRType::TypeCode code = findTypeMapping(def->category, def->field);
                if (code == GwXDRType::UNKNOWN)
                {
                    LOG_DEBUG(GwLog::DEBUG, "no type mapping for %s", def->toString().c_str());
                    continue;
                }
                GwXDRType *type = findType(code, &typeIndex);
                if (!type)
                {
                    LOG_DEBUG(GwLog::DEBUG, "no type definition for %s", def->toString().c_str());
                    continue;
                }
                long n2kkey=def->n2kKey();
                auto it=n2kMap.find(n2kkey);
                if (it == n2kMap.end()){
                    LOG_DEBUG(GwLog::DEBUG,"insert mapping with key %ld",n2kkey);
                    GwXDRMapping *mapping = new GwXDRMapping(def, type);
                    n2kMap[n2kkey]=mapping;
                }
                else{
                    LOG_DEBUG(GwLog::DEBUG,"append mapping with key %ld",n2kkey);
                    it->second->addMappingDef(def);
                }
                //for nmea0183 there could be multiple entries
                //as potentially there are different units that we can handle
                //so after we inserted the definition we do additional type lookups
                while (type != NULL){
                    String n183key=GwXDRMappingDef::n183key(def->xdrName,
                        type->xdrtype,type->xdrunit);
                    auto it=n183Map.find(n183key);
                    if (it == n183Map.end()){
                        LOG_DEBUG(GwLog::DEBUG,"insert mapping with n183key %s",n183key.c_str());
                        n183Map[n183key]=new GwXDRMapping(def,type);
                    }
                    else{
                        LOG_DEBUG(GwLog::DEBUG,"append mapping with n183key %s",n183key.c_str());
                        it->second->addMappingDef(def);
                    }
                    type=findType(code,&typeIndex);
                    if (! type) break;
                }
            }
        }
    }
}