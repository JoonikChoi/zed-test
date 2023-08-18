#include "portalComm.hpp"

namespace portal
{
    Comm::Comm(string serverURL)
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

                    cout << "this->sid : " << this->sid << endl;
                    this->connectionTriggerOnce = true;
                });
            });
    }

    void Comm::registering()
    {
        // io.socket()->emit("Start_Service", ServiceProfileBuilder(sid));
        sio::message::list argument("POST");
        // argument.push(sio::string_message::create("webrtc"));
        json profile = { {"id", sid}, {"serviceId", "ZED Camera"}, {"serviceType", "webrtc"} };
        argument.push(profile.dump());

        this->io.socket()->emit("registering service", argument, [&](sio::message::list const& res) {
            // registering service 'PUT'

            // const smessage::ptr& element = msg.at(0);
            // std::string smsg = msg.at(0)->get_string();
            // json json_msg = json::parse(smsg);
            cout << "acknowledge : " << (res.at(0)->get_map()["status"]->get_string()) << endl;
            // cout << "acknowledge2 : " << json_msg << endl;
            string status = res.at(0)->get_map()["status"]->get_string();
            string registerdServiceId = res.at(0)->get_map()["registeredService"]->get_map()["serviceId"]->get_string();

            if (status == "fail")
            {
                cout << "registering service failed" << endl;
                return;
            }

            // registering service 'PUT'
            json msg = { {"action", "join"}, {"serviceId", registerdServiceId}, {"userId", sid}, {"isHost", true} };

            sio::message::list argument("PUT");
            // argument.push(sio::string_message::create("null"));
            argument.push(msg.dump());

            // io.socket()->emit("registering service", argument);
            this->io.socket()->emit("registering service", argument, [&](sio::message::list const& res) {});
            });
    }

} // namespace portal