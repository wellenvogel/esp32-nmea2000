#include "GwXDRMappings.h"
#include "N2kMessages.h"
double PtoBar(double v){
    if (N2kIsNA(v)) return v;
    return v/100000L;
}
double BarToP(double v){
    if (N2kIsNA(v)) return v;
    return v*100000L;
}
GwXDRType * types[]={
    new GwXDRType(GwXDRType::PRESS,"P","P"),
    new GwXDRType(GwXDRType::PRESS,"P","B",
    PtoBar,
    BarToP),
    new GwXDRType(GwXDRType::VOLT,"U","V"),
    new GwXDRType(GwXDRType::AMP,"I","A"),
    NULL
};


String GwXDRMappingDef::toString(){
    String rt="";
    rt+=String((int)category);
    rt+=",";
    rt+=String((int)direction);
    rt+=",";
    rt+=String(selector);
    rt+=",";
    rt+=String(field);
    rt+=",";
    rt+=String(instanceMode);
    rt+=",";
    rt+=String(instanceId);
    return rt;
}
bool GwXDRMappingDef::handleToken(String tok,int index,GwXDRMappingDef *def){
    switch(index){
        int i;
        case 0:
            //category
            i=tok.toInt();
            if (i< XDRTEMP || i > XDRENGINE) return false;
            def->category=(GwXDRCategory)i;
            return true;
        case 1:
            //direction
            i=tok.toInt();
            if (i < GwXDRMappingDef::M_DISABLED || 
                i>= GwXDRMappingDef::M_LAST) return false;
            def->direction=(GwXDRMappingDef::Direction)i;
            return true;
        case 2:
            //selector
            //TODO: check selector?
            i=tok.toInt();
            def->selector=i;
            return true;
        case 3:
            //field
            i=tok.toInt();
            def->field=i;
            return true;
        case 4:
            //instance mode
            i=tok.toInt();
            if (i < GwXDRMappingDef::IS_SINGLE || 
                i>= GwXDRMappingDef::IS_LAST) return false;
            def->instanceMode=(GwXDRMappingDef::InstanceMode)i;
            return true;
        case 5:
            //instance id
            i=tok.toInt();
            def->instanceId=i;
            return true;
        default:
            break;
    }
    return false;
}
GwXDRMappingDef *GwXDRMappingDef::fromString(String s){
    int found=0;
    int last=0;
    int index=0;
    GwXDRMappingDef *rt=new GwXDRMappingDef();
    while ((found = s.indexOf(',',last)) >= 0){
        String tok=s.substring(last,found);
        if (!handleToken(tok,index,rt)){
            delete rt;
            return NULL;
        }
        last=found+1;
        index++;
    }
    if (last < s.length()){
        String tok=s.substring(last);
        if (!handleToken(tok,index,rt)){
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
void GwXDRMappings::begin(){
    char namebuf[10];
    for (int i=0;i<MAX_MAPPINGS;i++){
        snprintf(namebuf,9,"XDR%d",i);
        namebuf[9]=0;
        GwConfigInterface *cfg=config->getConfigItem(String(namebuf));
        if (cfg){
            GwXDRMappingDef *mapping=GwXDRMappingDef::fromString(cfg->asCString());
            if (mapping){
                LOG_DEBUG(GwLog::DEBUG,"read xdr mapping %s",mapping->toString().c_str());
            }
        }
    }
}