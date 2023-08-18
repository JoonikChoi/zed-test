#ifndef PORTAL_RTC
#define PORTAL_RTC

#include "nlohmann/json.hpp"
#include "portalComm.hpp"
#include "rtc/rtc.hpp"

using namespace std;
using json = nlohmann::json;
using std::shared_ptr;

namespace portal
{
    class RTC
    {
        portal::Comm* comm;
        bool isChannelOpen = false;
        std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> peerConnectionMap;
        std::unordered_map<std::string, shared_ptr<rtc::DataChannel>> dataChannelMap;
        shared_ptr<rtc::Track> track = NULL;
        shared_ptr<rtc::DataChannel> datachannel;

    public:
        RTC(portal::Comm* comm);

        shared_ptr<rtc::PeerConnection> pc;
        void test() const;

        void setOnSignaling();

        void setPeerConnectionMap(std::string sid, shared_ptr<rtc::PeerConnection> pc);
        std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> getPeerConnectionMap();
        bool getChannelStatus();
        size_t getChannelBufferedAmount();

        void sendDataToChannel(std::string type, std::vector<unsigned char> *data);
        void sendDataToChannel(std::string type, std::string data);
        template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr);
        shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration& config,
            shared_ptr<sio::socket> socket, std::string mysid,
            std::string target_sid);
    };
} // namespace portal

#endif