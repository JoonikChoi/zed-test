#include "portalComm.hpp"

namespace portal
{
    Comm::Comm(std::string serverURL)
    {
        io.connect(serverURL);
    }

    std::string Comm::getSid()
    {
        return this->sid;
    }

    void Comm::setOnTask()
    {
        this->io.set_open_listener([&]() {
            this->io.socket()->on("guide-socketid", [&](const std::string& name, const sio::message::ptr& data, bool isAck,
                sio::message::list& ack_resp) {
                    if (this->connectionTriggerOnce == true)
                        return;

                    auto mysid = std::static_pointer_cast<sio::string_message>(data)->get_string();
                    this->sid = mysid;

                    std::cout << "this->sid : " << this->sid << std::endl;
                    this->connectionTriggerOnce = true;
                });
            });
    }

    void Comm::registering()
    {
        sio::message::list argument("POST");

        json profile = { {"id", sid}, {"serviceId", "ZED Camera"}, {"serviceType", "webrtc"} };
        argument.push(profile.dump());

        this->io.socket()->emit("registering service", argument, [&](sio::message::list const& res) {

            // registering service 'PUT'
            std::cout << "acknowledge : " << (res.at(0)->get_map()["status"]->get_string()) << std::endl;
            std::string status = res.at(0)->get_map()["status"]->get_string();
            std::string registerdServiceId = res.at(0)->get_map()["registeredService"]->get_map()["serviceId"]->get_string();

            if (status == "fail")
            {
                std::cout << "registering service failed" << std::endl;
                return;
            }

            // registering service 'PUT'
            json msg = { {"action", "join"}, {"serviceId", registerdServiceId}, {"userId", sid}, {"isHost", true} };

            sio::message::list argument("PUT");
            argument.push(msg.dump());

            this->io.socket()->emit("registering service", argument, [&](sio::message::list const& res) {});
            });
    }

} // namespace portal