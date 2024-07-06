#include <json/json.h> 
#include <stdio.h>
#include "services.h"

bool ServicesFile::read_services(const char *file)
{
    Json::Value json_file;
    Json::Reader reader;

    FILE *f = fopen(file,"r");
    if(f == NULL) 
    {
        perror("Open failed");
        return false;
    }
        
    fseek(f,0,SEEK_END);
    int len = ftell(f);
    fseek(f,0,SEEK_SET);
    char *buf = (char *)malloc(len+1);
    fread(buf, len, 1, f);
    fclose(f);
    buf[len]='\0';

    bool success = reader.parse(buf, json_file);
    free(buf);
    if(!success)
        return false;

    Json::Value s = json_file["services"];
    for(auto i = s.begin(); i!=s.end(); i++)
    {
        serviceList.resize(serviceList.size()+1);
        read_service(serviceList.back(), *i);
    }
    return true;
}

void ServicesFile::read_service(service_t& service, const Json::Value& json)
{
    service.name = json["name"].asString();
    service.type = json["type"].asString();
    service.path = json["path"].asString();
    service.url = json["uri"].asString();
    service.required = json["required"].asBool();
    Json::Value s = json["services"];
    for(auto i = s.begin(); i!=s.end(); i++)
    {
        service.services.resize(service.services.size()+1);
        read_service(service.services.back(), *i);
    }
    s = json["subservices"];
    for(auto i = s.begin(); i!=s.end(); i++)
    {
        service.services.resize(service.services.size()+1);
        read_service(service.services.back(), *i);
    }
}

serviceList_t& ServicesFile::services()
{
    return serviceList;
}
