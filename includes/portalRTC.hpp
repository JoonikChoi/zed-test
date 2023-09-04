#ifndef PORTAL_RTC
#define PORTAL_RTC

#include "nlohmann/json.hpp"
#include "portalComm.hpp"
#include "rtc/rtc.hpp"
#include <thread>
#include <memory>
#include <stdexcept>
#include <utility>
#include <cstddef>
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
typedef int SOCKET;
#endif
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
        shared_ptr<rtc::DataChannel> datachannel;
        json candidate_json;
        std::thread *thread_;

    public:
        RTC(portal::Comm* comm);
        shared_ptr<rtc::Track> track = NULL;

        shared_ptr<rtc::PeerConnection> pc;

        void setOnSignaling();

        void setPeerConnectionMap(std::string sid, shared_ptr<rtc::PeerConnection> pc);
        std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> getPeerConnectionMap();
        bool getChannelStatus();
        size_t getChannelBufferedAmount();

        void sendDataToChannel(std::string type, std::vector<unsigned char> *data);
        void sendDataToChannel(std::string type, std::string data);
        template <class T> std::weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr);
        shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration& config,
            shared_ptr<sio::socket> socket, std::string mysid,
            std::string target_sid);


        void receiveThread();
        void startThread();
        void detachThread();
    };
} // namespace portal

#endif