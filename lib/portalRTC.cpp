#include "portalRTC.hpp"

namespace portal
{
    RTC::RTC(portal::Comm* comm)
    {
        this->comm = comm;
        std::cout << "RTC InitLogger ON" << std::endl;
        rtc::InitLogger(rtc::LogLevel::Debug);
    }

    void RTC::receiveThread() {
        const int BUFFER_SIZE = 2048;
        const rtc::SSRC ssrc = 42;

        SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(5555);

        if (bind(sock, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) < 0)
            throw std::runtime_error("Failed to bind UDP socket on 127.0.0.1:5555");
        else
            std::cout << "Successful to bind UDP socket on 127.0.0.1:5555" << std::endl;


        int rcvBufSize = 212992;
        setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&rcvBufSize),
            sizeof(rcvBufSize));

        // Receive from UDP
        char buffer[BUFFER_SIZE];
        int len;

        while ((len = recv(sock, buffer, BUFFER_SIZE, 0)) >= 0) {
            
            if (!track) { // if pointer is null, it return false.
                // pointer is NOT exist case.
                // std::cout << "Track Not Init" << std::endl;
                continue;
            }
            
            if (len < sizeof(rtc::RtpHeader) || !track->isOpen()) {
                // std::cout << "Track len, isOpen " << len << track->isOpen() << std::endl;
                continue;
            }
            try {
                auto rtp = reinterpret_cast<rtc::RtpHeader*>(buffer);
                rtp->setSsrc(ssrc);

                track->send(reinterpret_cast<const std::byte*>(buffer), len);
                // std::cout << "send ... " << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }

    void RTC::startThread() {
        std::thread recvThrd(std::bind(&RTC::receiveThread, this));
        recvThrd.detach();
        //thread_ = &recvThrd;
        //thread_ = std::thread([=] () mutable {this->receiveThread(); });
    }
    void RTC::detachThread() {
        //thread_->detach();
    }


    template <class T> std::weak_ptr<T> RTC::make_weak_ptr(shared_ptr<T> ptr)
    {
        return ptr;
    }

    void RTC::setOnSignaling()
    {
        comm->io.socket()->on("webrtc:connection request", [this](const std::string& name, const sio::message::ptr& data,
            bool isAck, sio::message::list& ack_resp) {
                std::cout << "connection request! " << std::endl;
                std::string eventName = name;
                std::string packet = std::static_pointer_cast<sio::string_message>(data)->get_string();
                std::cout << "event Name :" << eventName << "\nPacket :" << packet << " .. " << std::endl;
                // json message = json::parse((data)->get_string());
                std::string targetId = (data)->get_string();

                // MaybeStart() //
                rtc::Configuration config;
                std::string stunServer = "stun:stun.l.google:19302";
                config.iceServers.emplace_back(stunServer);
                //config.forceMediaTransport = true;

                this->setPeerConnectionMap(comm->getSid(),
                    createPeerConnection(config, comm->io.socket(), comm->getSid(), targetId));

                // 이 부분 나중에 수정 필요 !!
                // 상대방의 sid를 이용해서 map을 생성해야 하는데 현재는 자신의 sid를 이용해서 함.
                // 그렇다면 결론적으로 RTC class의 PC 멤버변수도 필요 없음. (일단은 임시로 존재)
                // 사실 최종적으로는 상대방의 sid도 아니고, 상대방의 이름(username)으로 바꿔야할듯.
                // commented by Joonik 23.07.19
                this->pc = this->getPeerConnectionMap().find(comm->getSid())->second;

                // pc->setLocalDescription();
                // auto data_channel = pc->createDataChannel("mydatachannel");
                const std::string label = "datachannel";
                std::cout << "Creating DataChannel with label \"" << label << "\"" << std::endl;
                std::cout << "connection request done. " << std::endl;
            });

        // it is only works in release build, not debug build
        // because of assert function in 'get_map()'. commented by Joonik 23.07.24
        comm->io.socket()->on("webrtc:signaling", [this](sio::event& ev) {
            std::cout << "[ event Name ] " << ev.get_name() << std::endl;

            std::string senderId = "NULL";
            std::string description_sdp = "NULL";
            std::string description_type = "NULL";
            std::string candidate = "NULL";

            if (ev.get_messages().at(1)->get_map().find("sdp") != ev.get_messages().at(1)->get_map().end())
            {
                description_sdp = ev.get_messages().at(1)->get_map()["sdp"]->get_string();
                description_type = ev.get_messages().at(1)->get_map()["type"]->get_string();
                std::cout << "[GET " << description_type << "] " << std::endl << description_sdp << std::endl;
                if (description_type == "offer")
                {
                    std::cout << "ignore getting offer" << std::endl;
                    return;
                }
            }
            else if (ev.get_messages().at(2)->get_map().find("candidate") != ev.get_messages().at(2)->get_map().end())
            {
                std::cout << "[GET candidate] " << candidate << std::endl;
                candidate = ev.get_messages().at(2)->get_map()["candidate"]->get_string();
                // string mid = ev.get_messages().at(2)->get_map()["mid"]->get_string();
                // cout << "mid :" << mid << endl;
                //  Process the value of description_sdp
            }
            else
            {
                std::cout << "something wrong" << std::endl;
            }

            if (description_sdp != "NULL")
            {
                std::cout << "[SET received description]" << std::endl;
                this->pc->setRemoteDescription(rtc::Description(description_sdp, description_type));
            }
            if (candidate != "NULL")
            {
                std::cout << "[SET received candaite]" << std::endl;
                pc->addRemoteCandidate(rtc::Candidate(candidate));
                // cout << "something wrong 2" << candidate << endl;
                // pc->addRemoteCandidate(rtc::Candidate(candidate, mid));
            }
            });
    }

    void RTC::setPeerConnectionMap(std::string sid, shared_ptr<rtc::PeerConnection> pc)
    {
        this->peerConnectionMap.emplace(sid, pc);
    }

    std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> RTC::getPeerConnectionMap()
    {
        return peerConnectionMap;
    }

    bool RTC::getChannelStatus()
    {

        return this->isChannelOpen;
    }

    size_t RTC::getChannelBufferedAmount() {
        return this->datachannel->bufferedAmount();
    }

    void RTC::sendDataToChannel(std::string type, std::string data)
    {

        if (!datachannel->isOpen())
            return;

        if (datachannel->bufferedAmount() != 0) {
            //cout << "Buffered Amount : " << datachannel->bufferedAmount() << endl;
            return;
        }

        datachannel->send((std::string)"Sensor data");
        datachannel->send(data);
        // cout << "Send Done, Data - Quaternion" << endl;

        // if (type == "quaternion")
        // {
        //     datachannel->send((std::string) "Sensor data");
        //     datachannel->send(data);
        // }
    }

    void RTC::sendDataToChannel(std::string type, std::vector<unsigned char>* data)
    {
        if (!isChannelOpen)
            return;

        if (datachannel->bufferedAmount() != 0) {
            //std::cout << "Buffered Amount : " << datachannel->bufferedAmount() << std::endl;
            return;
        }


        // datachannel->send((std::string) "hello from c++");

        std::transform(type.begin(), type.end(), type.begin(), ::toupper);
        if (type == "RGB")
        {
            auto middle2 = data->begin() + data->size() / 2;
            std::vector<unsigned char> rgbSegment_1(data->begin(), middle2);
            std::vector<unsigned char> rgbSegment_2(middle2, data->end());

            if (!datachannel->isOpen())
                return;

            datachannel->send((std::string)"RGB segment");
            datachannel->send((std::byte*)(rgbSegment_1.data()), rgbSegment_1.size());
            datachannel->send((std::string)"RGB segment");
            datachannel->send((std::byte*)(rgbSegment_2.data()), rgbSegment_2.size());
            datachannel->send((std::string)"RGB-gathering-done");
            //std::cout << rgbSegment_1.size() << "*2" << std::endl;
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

            if (!datachannel->isOpen())
                return;

            datachannel->send((std::string)"Depth segment");
            datachannel->send((std::byte*)(depthSegment_1.data()), depthSegment_1.size());
            datachannel->send((std::string)"Depth segment");
            datachannel->send((std::byte*)(depthSegment_2.data()), depthSegment_2.size());
            datachannel->send((std::string)"Depth segment");
            datachannel->send((std::byte*)(depthSegment_3.data()), depthSegment_3.size());
            datachannel->send((std::string)"Depth-gathering-done");
            datachannel->send((std::string)"Sensor data");
            //std::cout << depthSegment_3.size() << "*3" << std::endl;
        }
        // cout << "Send Done, Data - Frame" << endl;
    }

    shared_ptr<rtc::PeerConnection> RTC::createPeerConnection(const rtc::Configuration& config,
        shared_ptr<sio::socket> socket, std::string mysid,
        std::string target_sid)
    {
        auto pc = std::make_shared<rtc::PeerConnection>(config);
        const rtc::SSRC ssrc = 42;
        rtc::Description::Video media("video", rtc::Description::Direction::SendRecv);
        media.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
        media.addSSRC(ssrc, "video-send");
        media.setBitrate(1000);

        track = pc->addTrack(media);

        track->onOpen([this]() {
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
        

        pc->onStateChange([this](rtc::PeerConnection::State state) {
            std::cout << "State: " << state << std::endl;
            if (state == rtc::PeerConnection::State::Closed || state == rtc::PeerConnection::State::Failed)
            {
                std::cout << "webRTC connection is Closed or Failed \nClear peerConnectionMap ..." << std::endl;
                isChannelOpen = false;
                peerConnectionMap.clear();
            }
            });

        pc->onGatheringStateChange(
            [this, pc, socket, target_sid, mysid](rtc::PeerConnection::GatheringState state) {
                std::cout << "Gathering State: " << state << std::endl;
                if (state == rtc::PeerConnection::GatheringState::Complete) {
                    /*
                    auto description = pc->localDescription();
                    json message = { {"type", description->typeString()},
                                    {"sdp", std::string(description.value())} };
                    std::cout << message << std::endl;

                    json description_json = { {"type", description->typeString()}, {"sdp", std::string(description.value())} };

                    sio::message::list arguments;
                    arguments.push(target_sid);
                    arguments.push(mysid);
                    arguments.push(description_json.dump());

                    socket->emit("webrtc:signaling", arguments);

                    
                    //this->candidate_json = { {"candidate", std::string(candidate)}, {"sdpMid", candidate.mid()} };

                    sio::message::list arguments2;
                    arguments2.push(target_sid);
                    arguments2.push(mysid);
                    arguments2.push("undefined");
                    arguments2.push(this->candidate_json.dump());

                    socket->emit("webrtc:signaling", arguments2);
                    */
                }
                
            });

        
        pc->onLocalDescription([socket, mysid, target_sid](rtc::Description description) {
            std::cout << " =========== Set LocalDescription... ===========" << std::endl;
            std::cout << " target_sid : " << target_sid << std::endl;
            std::cout << " mysid : " << mysid << std::endl;
            std::cout << " type : " << description.typeString() << std::endl;
            std::cout << " sdp : " << std::string(description) << std::endl;
            std::cout << " =========== ======================= ===========" << std::endl;

            json description_json = { {"type", description.typeString()}, {"sdp", std::string(description)} };

            sio::message::list arguments;
            arguments.push(target_sid);
            arguments.push(mysid);
            arguments.push(description_json.dump());

            socket->emit("webrtc:signaling", arguments);
            });
        

        pc->onLocalCandidate([this, socket, target_sid, mysid](rtc::Candidate candidate) {
            this->candidate_json = { {"candidate", std::string(candidate)}, {"sdpMid", candidate.mid()} };

            sio::message::list arguments;
            arguments.push(target_sid);
            arguments.push(mysid);
            arguments.push("undefined");
            arguments.push(candidate_json.dump());

            socket->emit("webrtc:signaling", arguments);
        });


        auto dc = pc->createDataChannel("datachannel");

        dc->onOpen([wdc = make_weak_ptr(dc), this]() {
            this->isChannelOpen = true;
            std::cout << "DataChannel from open" << std::endl;
            if (auto dc = wdc.lock())
                dc->send("Channel Hello message!");
        });

        dc->onClosed([this]() {
            std::cout << "DataChannel from closed" << std::endl;
            isChannelOpen = false;
            dataChannelMap.clear();
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

        // 수정 필요
        dataChannelMap.emplace(mysid, dc);
        this->datachannel = dataChannelMap.find(mysid)->second;

        return pc;
    };
} // namespace portal