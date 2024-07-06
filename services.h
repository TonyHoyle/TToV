#ifndef SERVICES__H
#define SERVICES__H

#include <vector>
#include <string>
#include <json/json.h> 

struct service_t;

typedef std::vector<service_t> serviceList_t;

struct service_t
{
    std::string name;
    std::string type;
    std::string url;
    std::string path;
    bool required;
    serviceList_t services;
};

class ServicesFile
{
private:
    serviceList_t serviceList;

    void read_service(service_t& service, const Json::Value& json);

public:
    bool read_services(const char *file);
    serviceList_t& services();
};

#endif