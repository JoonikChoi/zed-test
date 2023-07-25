#include "portalComm.hpp"
#include "portalRTC.hpp"
#include "portalZed.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
using namespace std;

int main(int argc, char *argv[])
{

    portal::Zed zed;
    zed.setResloution(1280, 720);
    if (!zed.startZed())
        return -1;
    cout << "opened zed" << endl;

    portal::Comm comm("https://192.168.0.35:3333/portalComm_v0/");
    comm.setOnTask();
    comm.registering();

    portal::RTC portalRTC(&comm);

    // portalRTC.test();
    portalRTC.setOnSignaling();

    // sleep(100);

    // std::vector<unsigned char> rgb;

    // int i = 0;
    while (true)
    {
        // cout << "loop" << endl;
        if (portalRTC.getChannelStatus())
        {

            // pass by reference
            auto [rgb, depth, quaternion] = zed.extractFrame(portalRTC.getChannelStatus());

            portalRTC.sendDataToChannel("RGB", &rgb); // pass by reference
            portalRTC.sendDataToChannel("DEPTH", &depth);
            portalRTC.sendSensorData("SENSOR", quaternion);
        }
        else
        {
            // cout << "channel not opened" << endl;
        }
        // sleep(0.2);
    }
    zed.close();

    return 0;
}

// portal:: namespace 통일
// rtc, comm 파일 나누기
