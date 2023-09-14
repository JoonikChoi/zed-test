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

    struct PeerInfo {
        shared_ptr<rtc::PeerConnection> pc;
        std::shared_ptr<rtc::DataChannel> dataChannel;
        shared_ptr<rtc::Track> track;
        bool connectionActive;
        bool channelActive;
        bool trackActive;

        PeerInfo() {
            this->pc = NULL;
            this->dataChannel = NULL;
            this->track = NULL;
            this->connectionActive = false;
            this->channelActive = false;
            this->trackActive = false;
        }
        //PeerInfo(shared_ptr<rtc::PeerConnection> pc, std::shared_ptr<rtc::DataChannel> channel, shared_ptr<rtc::Track>, bool connectionActive, bool channelActive, bool trackActive)
          //  : pc(pc), dataChannel(channel), track(track), connectionActive(connectionActive), channelActive(channelActive) ,trackActive(trackActive) {}
    };


    class RTC
    {
        portal::Comm* comm;
        rtc::Configuration config;
        std::unordered_map<std::string, PeerInfo> PeerMap;

        // std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> peerConnectionMap;
        // std::unordered_map<std::string, shared_ptr<rtc::DataChannel>, bool> dataChannelMap; // clientID, datachannel, isChannelOpen
        // std::unordered_map<std::string, shared_ptr<rtc::Track>> trackMap;
        // shared_ptr<rtc::DataChannel> datachannel;
        // json candidate_json;

    public:
        RTC(portal::Comm* comm);

        void setOnSignaling();
        bool areAnyChannelsOpen();
        

        void sendDataToChannel(std::string type, std::vector<unsigned char> *data);
        void sendDataToChannel(std::string type, std::string data);
        template <class T> std::weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr);
        shared_ptr<rtc::PeerConnection> createPeerConnection(
            shared_ptr<sio::socket> socket, std::string clientID);


        void receiveThread();
    };
} // namespace portal

#endif