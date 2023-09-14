#include "portalComm.hpp"

namespace portal
{
    Comm::Comm(std::string serverURL)
    {
        io.connect(serverURL);
    }

    json Comm::makeJsonProfile() {
        json json_profile = {
            {"serialNumber", this->profile.serialNumber},
            {"alias", this->profile.alias},
            {"type", this->profile.type},
            {"authLevel", this->profile.authLevel},
            {"status", this->profile.status},
            {"location", this->profile.location},
            {"createdAt", this->profile.createdAt},
            {"descriptions", this->profile.descriptions},
            {"vender", this->profile.vender},
            {"apps", this->profile.apps}
        };

        return json_profile;

    }

    void Comm::setProfile(Profile profile)
    {
        this->profile = profile;
    }
    
    Profile Comm::getProfile() {
        return this->profile;
    }

    std::string Comm::getSid()
    {
        return this->sid;
    }

    bool Comm::createModule(const char* url)
    {
        CURL* curl;
        CURLcode res;

        // Initialize the Curl library
        curl = curl_easy_init();
        if (curl) {
            // Set the URL of the server API
            curl_easy_setopt(curl, CURLOPT_URL, url);

            // Create a JSON object
            // json post_data;
            // post_data["key1"] = "value1";
            // post_data["key2"] = "value2";
            
            json json_profile = makeJsonProfile();

            // Create a JSON object with the key "profile" and the value as json_profile
            json post_data = {
                {"profile", json_profile}
            };

            // Serialize the JSON data to a string
            std::string json_str = post_data.dump();

            // Set the JSON data as the POST data
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());

            // Set the callback function to handle the server response
            std::string response_data;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

            // Perform the HTTP POST request
            curl_easy_setopt(curl, CURLOPT_POST, 1L);

            // Set the Content-Type header to indicate JSON data
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Disable SSL certificate verification (equivalent to curl -k)
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }
            else {
                // Process the server response in the 'response_data' string
                std::cout << "Server Response: " << response_data << std::endl;

                /*
                // Parse the JSON response
                try {
                    json parsed_response = json::parse(response_data);

                    // Access and use the parsed JSON data
                    std::string someValue = parsed_response["someKey"];
                    // Replace "someKey" with the actual key in your JSON response

                    std::cout << "Parsed JSON Value: " << someValue << std::endl;
                }
                catch (json::exception& e) {
                    std::cerr << "JSON parsing error: " << e.what() << std::endl;
                }
                */
            }

            // Clean up
            curl_easy_cleanup(curl);

            // Free the headers
            curl_slist_free_all(headers);
        }
        else {
            std::cerr << "Curl initialization failed." << std::endl;
            return false;
        }

        return true;

    }

    void Comm::connectModule() {

        json json_profile = makeJsonProfile();

        this->io.socket()->emit("connect-module", json_profile.dump(), [&](sio::message::list const& res) {

            std::string RESPONSE_TAG = res.at(0)->get_map()["tag"]->get_string();
            std::string RESPONSE_STATUS = res.at(0)->get_map()["status"]->get_string();
            std::string RESPONSE_MSG = res.at(0)->get_map()["msg"]->get_string();
            std::cout << "connect-module RESPONSE_TAG : " << RESPONSE_TAG << std::endl;
            std::cout << "connect-module RESPONSE STATUS : " << RESPONSE_STATUS << std::endl;
            std::cout << "connect-module RESPONSE MSG : " << RESPONSE_MSG << std::endl;

         });

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

            if (this->reconnected) {
                std::cout << "Reconnected Successfully, Connect Module." << std::endl;
                this->connectModule();
                this->reconnected = false;
            }

        });

        this->io.set_reconnecting_listener([&]() {
            std::cout << "Connected Failed. Try to Reconnecting ..." << std::endl;
            this->reconnected = true;
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