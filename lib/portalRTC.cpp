#include "portalRTC.hpp"

namespace portal
{
RTC::RTC(portal::Comm *comm)
{
    this->comm = comm;
    cout << "RTC constructor" << endl;
}

void RTC::test() const
{
    cout << "RTC class test" << endl;
}

template <class T> weak_ptr<T> RTC::make_weak_ptr(shared_ptr<T> ptr)
{
    return ptr;
}

void RTC::setOnSignaling()
{
    comm->io.socket()->on("webrtc:connection request", [this](const std::string &name, const sio::message::ptr &data,
                                                              bool isAck, sio::message::list &ack_resp) {
        std::cout << "connection request! " << std::endl;
        string eventName = name;
        string packet = std::static_pointer_cast<sio::string_message>(data)->get_string();
        cout << "event Name :" << eventName << "\nPacket :" << packet << " .. " << endl;
        // json message = json::parse((data)->get_string());
        string targetId = (data)->get_string();

        // MaybeStart() //
        rtc::Configuration config;
        std::string stunServer = "stun:stun.l.google:19302";
        config.iceServers.emplace_back(stunServer);
        this->test();
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
    comm->io.socket()->on("webrtc:signaling", [this](sio::event &ev) {
        cout << "[ event Name ] " << ev.get_name() << endl;

        string senderId = "NULL";
        string description_sdp = "NULL";
        string description_type = "NULL";
        string candidate = "NULL";

        if (ev.get_messages().at(1)->get_map().find("sdp") != ev.get_messages().at(1)->get_map().end())
        {
            description_sdp = ev.get_messages().at(1)->get_map()["sdp"]->get_string();
            description_type = ev.get_messages().at(1)->get_map()["type"]->get_string();
            cout << "[GET " << description_type << "] " << endl << description_sdp << endl;
            if (description_type == "offer")
            {
                cout << "ignore getting offer" << endl;
                return;
            }
        }
        else if (ev.get_messages().at(2)->get_map().find("candidate") != ev.get_messages().at(2)->get_map().end())
        {
            cout << "[GET candidate] " << candidate << endl;
            candidate = ev.get_messages().at(2)->get_map()["candidate"]->get_string();
            // string mid = ev.get_messages().at(2)->get_map()["mid"]->get_string();
            // cout << "mid :" << mid << endl;
            //  Process the value of description_sdp
        }
        else
        {
            cout << "something wrong" << endl;
        }

        if (description_sdp != "NULL")
        {
            cout << "[SET received description]" << endl;
            this->pc->setRemoteDescription(rtc::Description(description_sdp, description_type));
        }
        if (candidate != "NULL")
        {
            cout << "[SET received candaite]" << endl;
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

void RTC::sendSensorData(std::string type, std::string data)
{
    cout << "send " << type << endl;
    datachannel->send((std::string) "Sensor data");
    datachannel->send(data);

    // if (type == "quaternion")
    // {
    //     datachannel->send((std::string) "Sensor data");
    //     datachannel->send(data);
    // }
}

void RTC::sendDataToChannel(std::string type, std::vector<unsigned char> data)
{
    if (!isChannelOpen)
        return;

    // datachannel->send((std::string) "hello from c++");

    std::transform(type.begin(), type.end(), type.begin(), ::toupper);
    if (type == "RGB")
    {
        auto middle2 = data.begin() + data.size() / 2;
        std::vector<unsigned char> rgbSegment_1(data.begin(), middle2);
        std::vector<unsigned char> rgbSegment_2(middle2, data.end());

        datachannel->send((std::string) "RGB segment");
        datachannel->send((std::byte *)(rgbSegment_1.data()), rgbSegment_1.size());
        datachannel->send((std::string) "RGB segment");
        datachannel->send((std::byte *)(rgbSegment_2.data()), rgbSegment_2.size());
        datachannel->send((std::string) "RGB-gathering-done");
        cout << "RGB send done" << endl;
    }
    if (type == "DEPTH")
    {
        const std::size_t vectorSize = data.size();
        const std::size_t segmentSize = vectorSize / 3;
        const auto firstSegmentBegin = data.begin();
        const auto firstSegmentEnd = firstSegmentBegin + segmentSize;
        const auto secondSegmentBegin = firstSegmentEnd;
        const auto secondSegmentEnd = secondSegmentBegin + segmentSize;
        const auto thirdSegmentBegin = secondSegmentEnd;
        const auto thirdSegmentEnd = data.end();

        std::vector<unsigned char> depthSegment_1(firstSegmentBegin, firstSegmentEnd);
        std::vector<unsigned char> depthSegment_2(secondSegmentBegin, secondSegmentEnd);
        std::vector<unsigned char> depthSegment_3(thirdSegmentBegin, thirdSegmentEnd);

        datachannel->send((std::string) "Depth segment");
        datachannel->send((std::byte *)(depthSegment_1.data()), depthSegment_1.size());
        datachannel->send((std::string) "Depth segment");
        datachannel->send((std::byte *)(depthSegment_2.data()), depthSegment_2.size());
        datachannel->send((std::string) "Depth segment");
        datachannel->send((std::byte *)(depthSegment_3.data()), depthSegment_3.size());
        datachannel->send((std::string) "Depth-gathering-done");
        datachannel->send((std::string) "Sensor data");
        cout << "DEPTH send done" << endl;
    }
}

shared_ptr<rtc::PeerConnection> RTC::createPeerConnection(const rtc::Configuration &config,
                                                          shared_ptr<sio::socket> socket, std::string mysid,
                                                          std::string target_sid)
{
    auto pc = std::make_shared<rtc::PeerConnection>(config);

    pc->onStateChange([this](rtc::PeerConnection::State state) {
        std::cout << "State: " << state << std::endl;
        if (state == rtc::PeerConnection::State::Closed || state == rtc::PeerConnection::State::Failed)
        {
            cout << "webRTC connection is Closed or Failed \nClear peerConnectionMap ..." << endl;
            isChannelOpen = false;
            peerConnectionMap.clear();
        }
    });

    pc->onGatheringStateChange(
        [](rtc::PeerConnection::GatheringState state) { std::cout << "Gathering State: " << state << std::endl; });

    pc->onLocalDescription([socket, mysid, target_sid](rtc::Description description) {
        cout << " =========== Set LocalDescription... ===========" << endl;
        cout << " target_sid : " << target_sid << endl;
        cout << " mysid : " << mysid << endl;
        cout << " type : " << description.typeString() << endl;
        cout << " sdp : " << std::string(description) << endl;
        cout << " =========== ======================= ===========" << endl;

        json description_json = {{"type", description.typeString()}, {"sdp", std::string(description)}};

        sio::message::list arguments;
        arguments.push(target_sid);
        arguments.push(mysid);
        arguments.push(description_json.dump());

        socket->emit("webrtc:signaling", arguments);
    });

    pc->onLocalCandidate([socket, target_sid, mysid](rtc::Candidate candidate) {
        json candidate_json = {{"candidate", std::string(candidate)}, {"sdpMid", candidate.mid()}};

        sio::message::list arguments;
        arguments.push(target_sid);
        arguments.push(mysid);
        arguments.push("undefined");
        arguments.push(candidate_json.dump());

        socket->emit("webrtc:signaling", arguments);
    });

    const rtc::SSRC ssrc = 42;
    rtc::Description::Video media("video", rtc::Description::Direction::SendRecv);
    media.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
    media.addSSRC(ssrc, "video-send");
    media.setBitrate(1000);

    track = pc->addTrack(media);
    track->onOpen([this]() {
        if (track->isOpen())
        {
            std::cout << "track is open" << std::endl;
        }
        else
        {
            std::cout << "track is not open" << std::endl;
        }
    });

    auto dc = pc->createDataChannel("datachannel");

    dc->onOpen([wdc = make_weak_ptr(dc), this]() {
        isChannelOpen = true;
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
        if (std::holds_alternative<std::string>(data))
            std::cout << "Message from received: " << std::get<std::string>(data) << std::endl;
        else
            std::cout << "Binary message from "
                      << " received, size=" << std::get<rtc::binary>(data).size() << std::endl;
    });

    // 수정 필요
    dataChannelMap.emplace(mysid, dc);
    this->datachannel = dataChannelMap.find(mysid)->second;
    return pc;
};
} // namespace portal