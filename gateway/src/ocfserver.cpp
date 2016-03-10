//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///
/// This sample provides steps to define an interface for a resource
/// (properties and methods) and host this resource on the server.
///

#include <functional>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include "OCPlatform.h"
#include "OCApi.h"
#include "gateway.h"

using namespace OC;
using namespace std;
namespace PH = std::placeholders;
int gObservation = 0;
bool isListOfObservers = false;

class LightResource
{
    public:
        /// Access this property from a TB client
        std::string m_name;
        vector<string> props;
        std::string m_lightUri;
        OCResourceHandle m_resourceHandle;
        OCRepresentation m_lightRep;
        ObservationIds m_interestedObservers;

    public:
        /// Constructor
        LightResource()
            :m_name("John's light"), m_lightUri("/a/light"),
            m_resourceHandle(nullptr) {
                // Initialize representation
                m_lightRep.setUri(m_lightUri);
                m_lightRep.setValue("name", m_name);
            }

        void createHVACResource()
        {
            std::string resourceURI = m_lightUri;
            std::string resourceTypeName = "core.light";
            std::string resourceInterface = DEFAULT_INTERFACE;

            uint8_t resourceProperty;
            resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;
            EntityHandler cb = std::bind(&LightResource::entityHandler, this,PH::_1);

            OCStackResult result = OCPlatform::registerResource(
                    m_resourceHandle, resourceURI, resourceTypeName,
                    resourceInterface, cb, resourceProperty);

            if (OC_STACK_OK != result)
            {
                cout << "Resource creation was unsuccessful\n";
            }
        }

        OCStackResult createCarLocationResource()
        {
            std::string resourceURI = "/a/light1";
            std::string resourceTypeName = "core.light";
            std::string resourceInterface = DEFAULT_INTERFACE;
            uint8_t resourceProperty;
            resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;
            EntityHandler cb = std::bind(&LightResource::entityHandler, this,PH::_1);

            OCResourceHandle resHandle;

            OCStackResult result = OCPlatform::registerResource(
                    resHandle, resourceURI, resourceTypeName,
                    resourceInterface, cb, resourceProperty);

            if (OC_STACK_OK != result)
            {
                cout << "Resource creation was unsuccessful\n";
            }

            return result;
        }

        OCResourceHandle getHandle()
        {
            return m_resourceHandle;
        }

        void put(OCRepresentation& rep)
        {
            try {
                if (rep.getValue("name", m_name))
                {
                    cout << "\t\t\t\t" << "name: " << m_name << endl;
                }
                else
                {
                    cout << "\t\t\t\t" << "name not found in the representation" << endl;
                }
            }
            catch (exception& e)
            {
                cout << e.what() << endl;
            }

        }

        OCRepresentation post(OCRepresentation& rep)
        {
            return get();
        }

        OCRepresentation get()
        {
            m_lightRep.setValue("name", m_name);
            return m_lightRep;
        }

        void addType(const std::string& type) const
        {
            OCStackResult result = OCPlatform::bindTypeToResource(m_resourceHandle, type);
            if (OC_STACK_OK != result)
            {
                cout << "Binding TypeName to Resource was unsuccessful\n";
            }
        }

        void addInterface(const std::string& interface) const
        {
            OCStackResult result = OCPlatform::bindInterfaceToResource(m_resourceHandle, interface);
            if (OC_STACK_OK != result)
            {
                cout << "Binding TypeName to Resource was unsuccessful\n";
            }
        }

    private:
        OCEntityHandlerResult entityHandler(std::shared_ptr<OCResourceRequest> request)
        {
            cout << "\tIn Server CPP entity handler:\n";
            OCEntityHandlerResult ehResult = OC_EH_ERROR;
            if(request)
            {
                // Get the request type and request flag
                std::string requestType = request->getRequestType();
                int requestFlag = request->getRequestHandlerFlag();

                if(requestFlag & RequestHandlerFlag::RequestFlag)
                {
                    cout << "\t\trequestFlag : Request\n";
                    auto pResponse = std::make_shared<OC::OCResourceResponse>();
                    pResponse->setRequestHandle(request->getRequestHandle());
                    pResponse->setResourceHandle(request->getResourceHandle());

                    QueryParamsMap queries = request->getQueryParameters();

                    if (!queries.empty())
                    {
                        std::cout << "\nQuery processing upto entityHandler" << std::endl;
                    }
                    for (auto it : queries)
                    {
                        std::cout << "Query key: " << it.first << " value : " << it.second
                            << std:: endl;
                    }

                    if(requestType == "GET")
                    {
                        cout << "\t\t\trequestType : GET\n";
                        pResponse->setErrorCode(200);
                        pResponse->setResponseResult(OC_EH_OK);
                        pResponse->setResourceRepresentation(get());
                        if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                        {
                            ehResult = OC_EH_OK;
                        }
                    }
                    else if(requestType == "PUT")
                    {
                        cout << "\t\t\trequestType : PUT\n";
                        OCRepresentation rep = request->getResourceRepresentation();
                        put(rep);
                        pResponse->setErrorCode(200);
                        pResponse->setResponseResult(OC_EH_OK);
                        pResponse->setResourceRepresentation(get());
                        if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                        {
                            ehResult = OC_EH_OK;
                        }
                    }
                    else if(requestType == "POST")
                    {
                        cout << "\t\t\trequestType : POST\n";

                        OCRepresentation rep = request->getResourceRepresentation();
                        OCRepresentation rep_post = post(rep);
                        pResponse->setResourceRepresentation(rep_post);
                        pResponse->setErrorCode(200);
                        if(rep_post.hasAttribute("createduri"))
                        {
                            pResponse->setResponseResult(OC_EH_RESOURCE_CREATED);
                            pResponse->setNewResourceUri(rep_post.getValue<std::string>("createduri"));
                        }
                        else
                        {
                            pResponse->setResponseResult(OC_EH_OK);
                        }

                        if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
                        {
                            ehResult = OC_EH_OK;
                        }
                    }
                }

                if(requestFlag & RequestHandlerFlag::ObserverFlag)
                {
                    ObservationInfo observationInfo = request->getObservationInfo();
                    if(ObserveAction::ObserveRegister == observationInfo.action)
                    {
                        m_interestedObservers.push_back(observationInfo.obsId);
                    }
                    else if(ObserveAction::ObserveUnregister == observationInfo.action)
                    {
                        m_interestedObservers.erase(std::remove(
                                    m_interestedObservers.begin(),
                                    m_interestedObservers.end(),
                                    observationInfo.obsId),
                                m_interestedObservers.end());
                    }

                    cout << "\t\trequestFlag : Observer\n";
                    gObservation = 1;
                    ehResult = OC_EH_OK;
                }
            }
            else
            {
                std::cout << "Request invalid" << std::endl;
            }

            return ehResult;
        }

};

static FILE* client_open(const char* /*path*/, const char *mode)
{
    return fopen("./oic_svr_db_server.json", mode);
}

void *server_thread(void *data)
{
    OCPersistentStorage ps {client_open, fread, fwrite, fclose, unlink };

    PlatformConfig cfg {
        OC::ServiceType::InProc,
            OC::ModeType::Server,
            "0.0.0.0", // By setting to "0.0.0.0", it binds to all available interfaces
            0,         // Uses randomly available port
            OC::QualityOfService::LowQos,
            &ps
    };

    OCPlatform::Configure(cfg);
	sm_session *s = (sm_session *) data;
	session_data sd;
	fd_set R, W, X;
	int rc = 0;

	GW_INF("Creating OCF Server thread %p", s);
	FD_ZERO(&R);
	FD_ZERO(&W);
	FD_ZERO(&X);
	FD_SET(s->sreqsock[0], &R);
	int max_sd = s->sreqsock[0];

	 //wait for request, indefinitely
	while (true) {
		GW_INF("OIC Server Waiting....");
		rc = select(max_sd + 1, &R, &W, &X, NULL);
		if (rc > 0) {
			bool isAppReq = FD_ISSET(s->sreqsock[0], &R);
			if (isAppReq) {
				int sock = s->sreqsock[0];
				GW_INF("MAX FD IS SET max_sd = %d", max_sd);
				int bytes = read(sock, &sd, sizeof(session_data));
				GW_INF("Read Request Size = %d....waiting", bytes);
				if (bytes > 0) {
					sm_request *r = (sm_request *) sd.rHandle;
					try
					{
						LightResource myLight;
						myLight.createHVACResource();
						std::cout << "Created resource." << std::endl;
						myLight.addType(std::string("core.brightlight"));
						myLight.addInterface(std::string(LINK_INTERFACE));
						std::cout << "Added Interface and Type" << std::endl;
					}
					catch(OCException &e)
					{
						std::cout << "OCException in main : " << e.what() << endl;
					}
				}
			}
		}
	}
    return 0;
}
