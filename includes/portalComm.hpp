#ifndef PORTAL_COMM
#define PORTAL_COMM

#include "nlohmann/json.hpp"
#include "sio_client.h"
#include <iostream>
#include <thread>
#include <curl/curl.h>

using json = nlohmann::json;
using std::shared_ptr;




namespace portal
{

    struct Profile {
        std::string serialNumber;
        std::string alias;
        std::string type;
        std::string authLevel;
        std::string status;
        std::string location;
        std::string createdAt;
        std::string descriptions;
        std::string vender;
        std::vector<std::string> apps;
    };


    class Comm
    {

    private:
        Profile profile;
        std::string sid = "";
        bool connectionTriggerOnce = false;
        json makeJsonProfile();

    public:
        sio::client io;

        Comm(std::string serverURL);
        void Comm::setProfile(Profile profile);
        std::string getSid();
        bool createModule(const char* url);
        void connectModule();
        void setOnTask();
        void registering();

        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
            size_t total_size = size * nmemb;
            output->append(static_cast<char*>(contents), total_size);
            return total_size;
        }

    };
} // namespace portal

#endif