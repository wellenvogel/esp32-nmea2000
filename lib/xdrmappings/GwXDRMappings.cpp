#include "GwXDRMappings.h"
#include "GWConfig.h"
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
                  BarToP,
                  PtoBar,
                  "P"),
    new GwXDRType(GwXDRType::VOLT, "U", "V"),
    new GwXDRType(GwXDRType::AMP, "I", "A"),
    new GwXDRType(GwXDRType::TEMP, "C", "K"),
    new GwXDRType(GwXDRType::TEMP, "C", "C", CToKelvin, KelvinToC,"K"),
    new GwXDRType(GwXDRType::HUMID, "H", "P"), //percent
    new GwXDRType(GwXDRType::VOLPERCENT, "V", "P"),
    new GwXDRType(GwXDRType::VOLUME, "V", "M", m3ToL, ltrTom3,"L"),
    new GwXDRType(GwXDRType::FLOW, "R", "I", ps2ph, ph2ps),
    new GwXDRType(GwXDRType::GENERIC, "G", ""),
    new GwXDRType(GwXDRType::DISPLACEMENT, "A", "P"),
    new GwXDRType(GwXDRType::DISPLACEMENTD, "A", "D",DegToRad,RadToDeg,"rd"),
    new GwXDRType(GwXDRType::RPM,"T","R")
    };
static GwXDRType genericType(GwXDRType::GENERIC, "G", "");
template<typename T, int size>
int GetArrLength(T(&)[size]){return size;}
static GwXDRType *findType(GwXDRType::TypeCode type, int *start = NULL)
{
    int len=GetArrLength(types);
    int from = 0;
    if (start != NULL)
        from = *start;
    if (from < 0 || from >= len) return NULL;    
    int i = from;
    for (; i< len; i++)
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

static GwXDRType *findType(const String &typeString, const String &unitString)
{
    int len=GetArrLength(types);
    for (int i=0; i< len; i++)
    {
        if (types[i]->xdrtype == typeString && types[i]->xdrunit == unitString)
        {
            return types[i];
        }
    }
    return &genericType;
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
//category,direction,selector,field,instanceMode,instance,name
String GwXDRMappingDef::toString() const
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
    rt += ",";
    rt += xdrName;
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
        if (i < XDRTEMP || i > XDRATTITUDE)
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
    case 6:
        def->xdrName=tok;
        return true;
    default:
        break;
    }
    return true; //ignore unknown trailing stuff
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
    if (rt->direction == GwXDRMappingDef::M_DISABLED || rt->xdrName == ""){
        delete rt;
        return NULL;
    }
    return rt;
}
String GwXDRMappingDef::getTransducerName(int instance) const
{
    String name = xdrName;
    if (instanceMode == GwXDRMappingDef::IS_AUTO)
    {
        name += "#";
        name += String(instance);
    }
    return name;
}

GwXDRFoundMapping::XdrEntry GwXDRFoundMapping::buildXdrEntry(double value)
{
    char buffer[40];
    XdrEntry rt;
    rt.transducer = getTransducerName();
    if (type->tonmea)
    {
        value = (*(type->tonmea))(value);
    }
    snprintf(buffer, 39, "%s,%.3f,%s,%s",
             type->xdrtype.c_str(),
             value,
             type->xdrunit.c_str(),
             rt.transducer.c_str());
    buffer[39] = 0;
    rt.entry=String(buffer);
    return rt;
}

GwXDRMappings::GwXDRMappings(GwLog *logger, GwConfigHandler *config)
{
    this->logger = logger;
    this->config = config;
}
bool GwXDRMappings::addFixedMapping(const GwXDRMappingDef &mapping){
    GwXDRMappingDef *nm=new GwXDRMappingDef(mapping);
    bool res=addMapping(nm);
    if (! res){
        LOG_DEBUG(GwLog::ERROR,"unable to add fixed mapping %s",mapping.toString().c_str());
        return false;
    }
    return true;
}
bool GwXDRMappings::addMapping(GwXDRMappingDef *def)
{
    if (def)
    {
        int typeIndex = 0;
        LOG_DEBUG(GwLog::LOG, "add xdr mapping %s",
                  def->toString().c_str());
        // n2k: find first matching type mapping
        GwXDRType::TypeCode code = findTypeMapping(def->category, def->field);
        if (code == GwXDRType::UNKNOWN)
        {
            LOG_DEBUG(GwLog::ERROR, "no type mapping for %s", def->toString().c_str());
            return false;
        }
        GwXDRType *type = ::findType(code, &typeIndex);
        if (!type)
        {
            LOG_DEBUG(GwLog::ERROR, "no type definition for %s", def->toString().c_str());
            return false;
        }
        long n2kkey = def->n2kKey();
        auto it = n2kMap.find(n2kkey);
        GwXDRMapping *mapping = new GwXDRMapping(def, type);
        if (it == n2kMap.end())
        {
            LOG_DEBUG(GwLog::LOG, "insert mapping with key %ld", n2kkey);
            GwXDRMapping::MappingList mappings;
            mappings.push_back(mapping);
            n2kMap[n2kkey] = mappings;
        }
        else
        {
            LOG_DEBUG(GwLog::LOG, "append mapping with key %ld", n2kkey);
            it->second.push_back(mapping);
        }
        // for nmea0183 there could be multiple entries
        // as potentially there are different units that we can handle
        // so after we inserted the definition we do additional type lookups
        while (type != NULL)
        {
            String n183key = GwXDRMappingDef::n183key(def->xdrName,
                                                      type->xdrtype, type->xdrunit);
            auto it = n183Map.find(n183key);
            if (it == n183Map.end())
            {
                LOG_DEBUG(GwLog::LOG, "insert mapping with n183key %s", n183key.c_str());
                GwXDRMapping::MappingList mappings;
                mappings.push_back(mapping);
                n183Map[n183key] = mappings;
            }
            else
            {
                LOG_DEBUG(GwLog::LOG, "append mapping with n183key %s", n183key.c_str());
                it->second.push_back(mapping);
            }
            type = ::findType(code, &typeIndex);
            if (!type)
                break;
            mapping = new GwXDRMapping(def, type);
        }
        return true;
    }
    else
    {
        return false;
    }
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
                bool res=addMapping(def);
                if (! res){
                    LOG_DEBUG(GwLog::ERROR,"unable to add mapping from %s",cfg);
                    delete cfg;
                }
            }
            else{
                LOG_DEBUG(GwLog::DEBUG,"unable to parse mapping %s",cfg->asCString());
            }
        }
    }
}

/**
 * select the best matching mapping
 * depending on the instance id
 */
GwXDRFoundMapping GwXDRMappings::selectMapping(GwXDRMapping::MappingList *list, int instance, const char *key)
{
    GwXDRMapping *candidate = NULL;
    unsigned long invalidTime=config->getInt(GwConfigDefinitions::timoSensor);
    for (auto mit = list->begin(); mit != list->end(); mit++)
    {
        GwXDRMappingDef *def = (*mit)->definition;
        //if there is no instance (coming from xdr with no instance) we take a mapping with instance type ignore
        //or the first explicit mapping
        //for coming from xdr we only should have one mapping in the list
        //but auto will not match for unknwn instance
        switch (def->instanceMode)
        {
        case GwXDRMappingDef::IS_SINGLE:
            if (def->instanceId == instance)
            {
                LOG_DEBUG(GwLog::DEBUG + 1, "selected mapping %s for %s, i=%d",
                          def->toString().c_str(), key, instance);
                return GwXDRFoundMapping(*mit,invalidTime, instance);
            }
            if (instance < 0)
            {
                if (candidate == NULL)
                    candidate = *mit;
            }
        case GwXDRMappingDef::IS_AUTO:
            if (instance >= 0)
                candidate = *mit;
            break;
        case GwXDRMappingDef::IS_IGNORE:
            if (candidate == NULL)
                candidate = *mit;
            break;

        default:
            break;
        }
    }
    if (candidate != NULL)
    {
        LOG_DEBUG(GwLog::DEBUG + 1, "selected mapping %s for %s, i=%d",
                  candidate->definition->toString().c_str(), key, instance);
        return GwXDRFoundMapping(candidate, invalidTime,instance>=0?instance:candidate->definition->instanceId);
    }
    LOG_DEBUG(GwLog::DEBUG + 1, "no instance mapping found for key=%s, i=%d", key, instance);
    return GwXDRFoundMapping();
}
GwXDRFoundMapping GwXDRMappings::getMapping(String xName,String xType,String xUnit){
    //get an instance suffix from the name and separate it
    int sepIdx=xName.indexOf('#');
    int instance=-1;
    if (sepIdx>=0){
        String istr=xName.substring(sepIdx+1);
        xName=xName.substring(0,sepIdx);
        instance=istr.toInt();
    }
    if (xName == "") return GwXDRFoundMapping();
    String n183Key=GwXDRMappingDef::n183key(xName,xType,xUnit);
    auto it=n183Map.find(n183Key);
    if (it == n183Map.end()) {
        LOG_DEBUG(GwLog::DEBUG+1,"find n183mapping for %s,i=%d - nothing found",n183Key.c_str(),instance);
        return GwXDRFoundMapping();
    }
    return selectMapping(&(it->second),instance,n183Key.c_str());
}
GwXDRFoundMapping GwXDRMappings::getMapping(GwXDRCategory category,int selector,int field,int instance){
    unsigned long n2kKey=GwXDRMappingDef::n2kKey(category,selector,field);
    auto it=n2kMap.find(n2kKey);
    if (it == n2kMap.end()){
        LOG_DEBUG(GwLog::DEBUG+1,"find n2kmapping for c=%d,s=%d,f=%d,i=%d - nothing found",
            (int)category,selector,field,instance);
        addUnknown(category,selector,field,instance);    
        return GwXDRFoundMapping();
    }
    char kbuf[20];
    snprintf(kbuf,19,"%lu",n2kKey);
    kbuf[19]=0;
    GwXDRFoundMapping rt=selectMapping(&(it->second),instance,kbuf);
    if (rt.empty){
        addUnknown(category,selector,field,instance);    
    }
    return rt;
}

bool GwXDRMappings::addUnknown(GwXDRCategory category,int selector,int field,int instance){
    if (unknown.size() >= 200) return false;
    unsigned long uk=((int)category) &0x7f;
    if (selector < 0) selector=0;
    uk=(uk<<7) + (selector &0x7f);
    if (field < 0) field=0;
    uk=(uk<<7) + (field & 0x7f);
    if (instance < 0 || instance > 255) instance=256; //unknown
    uk=(uk << 9) + (instance & 0x1ff);
    unknown.insert(uk);
    return true;
}

const char * GwXDRMappings::getUnMapped(){
    if (unknowAsString == NULL) unknowAsString=new char[(MAX_MAPPINGS+1)*ESIZE];
    *unknowAsString=0;
    char *ptr=unknowAsString;
    for (auto it=unknown.begin();it!=unknown.end();it++){
        snprintf(ptr,ESIZE-1,"%lu\n",*it);
        *(ptr+ESIZE-1)=0;
        while (*ptr != 0) ptr++;
    }
    return unknowAsString;
}

String GwXDRMappings::getXdrEntry(String mapping, double value,int instance){
    String rt;
    GwXDRMappingDef *def=GwXDRMappingDef::fromString(mapping);
    if (! def) return rt;
    int typeIndex=0;
    GwXDRType::TypeCode code = findTypeMapping(def->category, def->field);
    if (code == GwXDRType::UNKNOWN)
    {
        return rt;
    }
    GwXDRType *type = ::findType(code, &typeIndex);
    bool first=true;
    unsigned long invalidTime=config->getInt(GwConfigDefinitions::timoSensor);
    while (type){
        GwXDRFoundMapping found(def,type,invalidTime);
        found.instanceId=instance;
        if (first) first=false;
        else rt+=",";
        rt+=found.buildXdrEntry(value).entry;
        type = ::findType(code, &typeIndex);
    }
    delete def;
    return rt;
}

const GwXDRType * GwXDRMappings::findType(const String &typeString, const String &unitString) const{
    return ::findType(typeString,unitString);
}