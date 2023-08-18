#ifndef PORTAL_COMM
#define PORTAL_COMM

#include "nlohmann/json.hpp"
#include "sio_client.h"
#include <iostream>
#include <thread>

using namespace std;
using json = nlohmann::json;
using std::shared_ptr;

namespace portal
{
    class Comm
    {

    private:
        std::string taskName = "portalCam__0";
        std::string sid = "";
        bool connectionTriggerOnce = false;

    public:
        sio::client io;

        Comm(string serverURL);

        std::string getSid();
        void setOnTask();
        void registering();
    };
} // namespace portal

#endif