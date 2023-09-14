#include "portalRTC.hpp"

namespace portal
{
    RTC::RTC(portal::Comm* comm)
    {
        this->comm = comm;
        std::cout << "RTC InitLogger ON" << std::endl;
        rtc::InitLogger(rtc::LogLevel::Debug);


        std::string stunServer = "stun:stun.l.google.com:19302";
        // const rtc::IceServer turnServer("turn:3.35.133.246", 3478, "user", "pass");
        this->config.iceServers.emplace_back(stunServer);
        // config.iceServers.emplace_back(turnServer);

        rtc::SctpSettings settings;
        settings.sendBufferSize = 1024 * 1024 * 10;
        rtc::SetSctpSettings(settings);
    }

    void RTC::receiveThread() {
        // const int BUFFER_SIZE = 2048;
        // const rtc::SSRC ssrc = 42;

        // SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
        // struct sockaddr_in addr = {};
        // addr.sin_family = AF_INET;
        // addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        // addr.sin_port = htons(5555);

        // if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0)
        //     throw std::runtime_error("Failed to bind UDP socket on 127.0.0.1:5555");
        // else
        //     std::cout << "Successful to bind UDP socket on 127.0.0.1:5555" << std::endl;


        // int rcvBufSize = 212992;
        // setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&rcvBufSize),
        //     sizeof(rcvBufSize));

        // // Receive from UDP
        // char buffer[BUFFER_SIZE];
        // int len;

        // while ((len = recv(sock, buffer, BUFFER_SIZE, 0)) >= 0) {
            
        //     for (const auto& pair : PeerMap) {
        //         shared_ptr<rtc::Track> track = pair.second.track;
        //         std::string clientID = pair.first;
        //         if (!track) { // if pointer is null, it return false.
        //                         // pointer is NOT exist case.
        //                         // std::cout << "Track Not Init" << std::endl;
        //             continue;
        //         }

        //         if (len < sizeof(rtc::RtpHeader) || !track->isOpen()) {
        //             // std::cout << "Track len, isOpen " << len << track->isOpen() << std::endl;
        //             continue;
        //         }
        //         try {
        //             auto rtp = reinterpret_cast<rtc::RtpHeader*>(buffer);
        //             rtp->setSsrc(ssrc);

        //             if (track->bufferedAmount() != 0) {
        //                 std::cout << "Client: " << clientID << ", Track BufferedAmount: " << track->bufferedAmount() << std::endl;
        //             }
        //             else {
        //                 // std::cout << "track-send" << std::endl;
        //                 track->send(reinterpret_cast<const std::byte*>(buffer), len);
        //             }
        //         }
        //         catch (const std::exception& e) {
        //             std::cerr << "Error: " << e.what() << std::endl;
        //         }
        //     }

        // }
    }


    template <class T> std::weak_ptr<T> RTC::make_weak_ptr(shared_ptr<T> ptr)
    {
        return ptr;
    }

    void RTC::setOnSignaling()
    {
        comm->io.socket()->on("webrtc:connection request", [this](const std::string& name, const sio::message::ptr& data,
            bool isAck, sio::message::list& ack_resp) {
                std::cout << "webrtc:connection request, clientID : " << data->get_string() << std::endl;

                // std::string eventName = name;
                // std::string packet = std::static_pointer_cast<sio::string_message>(data)->get_string();
                // std::cout << "event Name :" << eventName << "\nPacket :" << packet << " .. " << std::endl;
                // json message = json::parse((data)->get_string());
                // std::string targetId = (data)->get_string();

                //config.forceMediaTransport = true;

                std::string clientID = data->get_string();
                // this->setPeerConnectionMap(clientID, createPeerConnection(comm->io.socket(), clientID));
                // this->PeerMap.emplace(clientID, createPeerConnection(comm->io.socket(), clientID));
                PeerMap.emplace(clientID, PeerInfo());
                PeerMap[clientID].pc = createPeerConnection(comm->io.socket(), clientID);
                // get_messages().at(1)->get_map().find("sdp") != ev.get_messages().at(1)->get_map().end()
                // this->pc = this->getPeerConnectionMap().find(clientID)->second;

                const std::string label = "datachannel";
                std::cout << "Creating DataChannel with label \"" << label << "\"" << std::endl;
                std::cout << "connection request done. " << std::endl;
            });

        // it is only works in release build, not debug build
        // because of assert function in 'get_map()'. commented by Joonik 23.07.24
        comm->io.socket()->on("webrtc:signaling", [this](sio::event& ev) {
            std::cout << "webrtc:sigaling : " << ev.get_name() << std::endl;

            std::string clientID = ev.get_messages().at(0)->get_string();
            std::string description_sdp = "";
            std::string description_type = "";
            std::string candidate = "";

            if (clientID.empty() || clientID == "undefined") {
                std::cout << "It is Empty or Undefined!!!!!" << std::endl;
                std::cout << "Received my Message myself. Ignore This Signal." << std::endl;
                return;
            }
            else {
                std::cout << clientID << std::endl;
            }

            /*
            if (clientID == "undefined") {
                std::cout << "Received my Message myself. Ignore This Signal." << std::endl;
                return;
            }
            */
            
            if (ev.get_messages().at(1)->get_map().find("sdp") != ev.get_messages().at(1)->get_map().end())
            {
                description_sdp = ev.get_messages().at(1)->get_map()["sdp"]->get_string();
                description_type = ev.get_messages().at(1)->get_map()["type"]->get_string();
                std::cout << "[GET " << description_type << "] " << std::endl << description_sdp << std::endl;
                if (description_type == "offer")
                {
                    std::cout << "Ignore getting offer" << std::endl;
                    return;
                }
            }
            else if (ev.get_messages().at(2)->get_map().find("candidate") != ev.get_messages().at(2)->get_map().end())
            {
                candidate = ev.get_messages().at(2)->get_map()["candidate"]->get_string();
                std::cout << "[GET candidate]\n" << candidate << std::endl;
                // string mid = ev.get_messages().at(2)->get_map()["mid"]->get_string();
                // cout << "mid :" << mid << endl;
                //  Process the value of description_sdp
            }
            else
            {
                std::cout << "Do nothing (NULL SIGNAL)" << std::endl;
            }

            if (!description_sdp.empty())
            {
                std::cout << "[SET received description]" << std::endl;
                // shared_ptr<rtc::PeerConnection> pc = this->peerConnectionMap.find(clientID)->second;
                PeerMap[clientID].pc->setRemoteDescription(rtc::Description(description_sdp, description_type));
                
            }
            if (!candidate.empty())
            {
                std::cout << "[SET received candaite]" << std::endl;
                // shared_ptr<rtc::PeerConnection> pc = this->peerConnectionMap.find(clientID)->second;
                PeerMap[clientID].pc->addRemoteCandidate(rtc::Candidate(candidate));
                //pc->addRemoteCandidate(rtc::Candidate(candidate));
                // pc->addRemoteCandidate(rtc::Candidate(candidate, mid));
            }
            });
    }

    
    bool RTC::areAnyChannelsOpen()
    {
        for (const auto& pair : PeerMap) {
            if (pair.second.channelActive) return true;
        }

        return false;

    }

    void RTC::sendDataToChannel(std::string type, std::string data)
    {

        for (const auto& pair : PeerMap) {
            // std::cout << "Peer: " << pair.first << ", Channel Active: " << pair.second.channelActive << std::endl;

            std::shared_ptr<rtc::DataChannel> dc = pair.second.dataChannel;

            if (!dc) { // if pointer is null, it return false.
                // pointer is NOT exist case.
                std::cout << "DataChannel Not Init" << std::endl;
                continue;
            }


            if (!dc->isOpen()) continue;
            if (dc->bufferedAmount() != 0) { 
                std::cout << "Client: " << pair.first << ", Channel BufferedAmount: " << dc->bufferedAmount() << std::endl;
                continue;
            };

            try {
                dc->send((std::string)"Sensor data");
                dc->send(data);
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }

    void RTC::sendDataToChannel(std::string type, std::vector<unsigned char>* data)
    {

        for (const auto& pair : PeerMap) {
            // std::cout << "Peer: " << pair.first << ", Channel Active: " << pair.second.channelActive << std::endl;

            std::shared_ptr<rtc::DataChannel> dc = pair.second.dataChannel;
            
            if (!dc) { // if pointer is null, it return false.
                // pointer is NOT exist case.
                std::cout << "DataChannel Not Init" << std::endl;
                continue;
            }


            if (!dc->isOpen()) continue;
            if (dc->bufferedAmount() != 0) {
                std::cout << "Client: " << pair.first << ", Channel BufferedAmount: " << dc->bufferedAmount() << std::endl;
                continue;
            };

            if (type == "RGB")
            {
                auto middle2 = data->begin() + data->size() / 2;
                std::vector<unsigned char> rgbSegment_1(data->begin(), middle2);
                std::vector<unsigned char> rgbSegment_2(middle2, data->end());

                if (!dc->isOpen())
                    return;


                try {
                    dc->send((std::string)"RGB segment");
                    dc->send((std::byte*)(rgbSegment_1.data()), rgbSegment_1.size());
                    dc->send((std::string)"RGB segment");
                    dc->send((std::byte*)(rgbSegment_2.data()), rgbSegment_2.size());
                    dc->send((std::string)"RGB-gathering-done");
                }
                catch (const std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
            }
            if (type == "DEPTH")
            {
                const std::size_t vectorSize = data->size();
                const std::size_t segmentSize = vectorSize / 3;
                const auto firstSegmentBegin = data->begin();
                const auto firstSegmentEnd = firstSegmentBegin + segmentSize;
                const auto secondSegmentBegin = firstSegmentEnd;
                const auto secondSegmentEnd = secondSegmentBegin + segmentSize;
                const auto thirdSegmentBegin = secondSegmentEnd;
                const auto thirdSegmentEnd = data->end();

                std::vector<unsigned char> depthSegment_1(firstSegmentBegin, firstSegmentEnd);
                std::vector<unsigned char> depthSegment_2(secondSegmentBegin, secondSegmentEnd);
                std::vector<unsigned char> depthSegment_3(thirdSegmentBegin, thirdSegmentEnd);

                if (!dc->isOpen())
                    return;
                // dc->
                try {
                    dc->send((std::string)"Depth segment");
                    dc->send((std::byte*)(depthSegment_1.data()), depthSegment_1.size());
                    dc->send((std::string)"Depth segment");
                    dc->send((std::byte*)(depthSegment_2.data()), depthSegment_2.size());
                    dc->send((std::string)"Depth segment");
                    dc->send((std::byte*)(depthSegment_3.data()), depthSegment_3.size());
                    dc->send((std::string)"Depth-gathering-done");
                    dc->send((std::string)"Sensor data");
                }
                catch (const std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
                //std::cout << depthSegment_3.size() << "*3" << std::endl;
            }
            // cout << "Send Done, Data - Frame" << endl;
        }
    }

    shared_ptr<rtc::PeerConnection> RTC::createPeerConnection(shared_ptr<sio::socket> socket, std::string clientID)
    {
        auto pc = std::make_shared<rtc::PeerConnection>(this->config);
        const rtc::SSRC ssrc = 42;
        rtc::Description::Video media("video", rtc::Description::Direction::SendRecv);
        media.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
        media.addSSRC(ssrc, "video-send");
        media.setBitrate(1000);

        shared_ptr<rtc::Track> track = NULL;

        track = pc->addTrack(media);

        track->onOpen([track]() {
            if (track->isOpen())
            {
                std::cout << "Track is open" << std::endl;
                //this->startThread();
                //this->detachThread();
            }
            else
            {
                std::cout << "Track is not open" << std::endl;
            }
        });

        PeerMap[clientID].track = track;
        // trackMap.emplace(clientID, track);
        
        

        pc->onStateChange([this, clientID](rtc::PeerConnection::State state) {
            std::cout << "State: " << state << std::endl;
            if (state == rtc::PeerConnection::State::Closed || state == rtc::PeerConnection::State::Failed)
            {
                std::cout << "webRTC connection is Closed or Failed" << std::endl;
                std::cout << "Clear PeerInfo about " << clientID << std::endl;
                // PeerInfo PeerInfo = PeerMap[clientID];
                PeerMap[clientID].connectionActive = false;
                PeerMap[clientID].trackActive = false;
                PeerMap[clientID].channelActive = false;

                // Delete an element using the key
                /*
                auto it = PeerMap.find(clientID);
                if (it != PeerMap.end()) {
                    PeerMap.erase(it);
                    std::cout << "Element with key '" << clientID << "' deleted." << std::endl;
                }
                else {
                    std::cout << "Element with key '" << clientID << "' not found." << std::endl;
                }
                */
            }


        });

        pc->onGatheringStateChange(
            [](rtc::PeerConnection::GatheringState state) {
                std::cout << "Gathering State: " << state << std::endl;
                if (state == rtc::PeerConnection::GatheringState::Complete) {

                }                
            });

        
        pc->onLocalDescription([this, socket](rtc::Description description) {
            
            json description_json = { {"type", description.typeString()}, {"sdp", std::string(description)} };
            
            std::cout << "[ Making My Description ]" << std::endl;

            sio::message::list arguments;
            arguments.push(this->comm->getProfile().serialNumber);
            arguments.push("undefined");
            arguments.push(description_json.dump());
            arguments.push("undefined");

            // 이것도 사실 socket말고 this->comm->socket으로 대체 가능함
            // 아니면 그냥 javascript에서 처럼 sendSignal 함수를 하나 만드는게 나을지도?
            socket->emit("webrtc:signaling", arguments);
            //this.serialNumber, senderID==undefined, sdp, candidate
            });
        

        pc->onLocalCandidate([this, socket](rtc::Candidate candidate) {
            json candidate_json = { {"candidate", std::string(candidate)}, {"sdpMid", candidate.mid()} };

            sio::message::list arguments;
            arguments.push(this->comm->getProfile().serialNumber);
            arguments.push("undefined");
            arguments.push("undefined");
            arguments.push(candidate_json.dump());

            socket->emit("webrtc:signaling", arguments);
        });


        rtc::DataChannelInit init = {};
        init.reliability.type = rtc::Reliability::Type::Rexmit;
        init.reliability.unordered = true;
        init.reliability.rexmit = 0;
        
        auto dc = pc->createDataChannel("datachannel", init);

        //dc->
        //`>
        dc->onOpen([wdc = make_weak_ptr(dc), this, clientID]() {
            PeerMap[clientID].channelActive = true;
            std::cout << "DataChannel from open" << std::endl;
            if (auto dc = wdc.lock())
                dc->send("Channel Hello message!");
        });

        dc->onClosed([this, clientID]() {
            std::cout << "DataChannel from closed" << std::endl;
            // isChannelOpen = false;
            // dataChannelMap.clear();

            PeerMap[clientID].channelActive = false;



        });

        dc->onMessage([wdc = make_weak_ptr(dc)](auto data) {
            // data holds either std::string or rtc::binary
            if (std::holds_alternative<std::string>(data)) {
                std::cout << "Message from received: " << std::get<std::string>(data) << std::endl;
            }
            else {
                std::cout << "Binary message from "
                    << " received, size=" << std::get<rtc::binary>(data).size() << std::endl;
            }
        });

        PeerMap[clientID].dataChannel = dc;
        // dataChannelMap.emplace(clientID, dc, false);
        // dataChannelMap[clientID] = ChannelInfo(dc, false);
        // this->datachannel = dataChannelMap.find(clientID)->second;

        return pc;
    };
} // namespace portal