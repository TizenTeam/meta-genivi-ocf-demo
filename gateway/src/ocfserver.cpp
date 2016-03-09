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

        /* Note that this does not need to be a member function: for classes you do not have
           access to, you can accomplish this with a free function: */

        /// This function internally calls registerResource API.
        void createResource()
        {
            //URI of the resource
            std::string resourceURI = m_lightUri;
            //resource type name. In this case, it is light
            std::string resourceTypeName = "core.light";
            // resource interface.
            std::string resourceInterface = DEFAULT_INTERFACE;

            // OCResourceProperty is defined ocstack.h
            uint8_t resourceProperty;
            resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;
            EntityHandler cb = std::bind(&LightResource::entityHandler, this,PH::_1);

            // This will internally create and register the resource.
            OCStackResult result = OCPlatform::registerResource(
                    m_resourceHandle, resourceURI, resourceTypeName,
                    resourceInterface, cb, resourceProperty);

            if (OC_STACK_OK != result)
            {
                cout << "Resource creation was unsuccessful\n";
            }
        }

        OCStackResult createResource1()
        {
            // URI of the resource
            std::string resourceURI = "/a/light1";
            // resource type name. In this case, it is light
            std::string resourceTypeName = "core.light";
            // resource interface.
            std::string resourceInterface = DEFAULT_INTERFACE;

            // OCResourceProperty is defined ocstack.h
            uint8_t resourceProperty;
            resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;
            EntityHandler cb = std::bind(&LightResource::entityHandler, this,PH::_1);

            OCResourceHandle resHandle;

            // This will internally create and register the resource.
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

        // Puts representation.
        // Gets values from the representation and
        // updates the internal state
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

        // Post representation.
        // Post can create new resource or simply act like put.
        // Gets values from the representation and
        // updates the internal state
        OCRepresentation post(OCRepresentation& rep)
        {
            static int first = 1;

            // for the first time it tries to create a resource
            if(first)
            {
                first = 0;

                if(OC_STACK_OK == createResource1())
                {
                    OCRepresentation rep1;
                    rep1.setValue("createduri", std::string("/a/light1"));

                    return rep1;
                }
            }

            // from second time onwards it just puts
            put(rep);
            return get();
        }


        // gets the updated representation.
        // Updates the representation with latest internal state before
        // sending out.
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
        // This is just a sample implementation of entity handler.
        // Entity handler can be implemented in several ways by the manufacturer
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

                    // Check for query params (if any)
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

                    // If the request type is GET
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

                        // Do related operations related to PUT request
                        // Update the lightResource
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

                        // Do related operations related to POST request
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
                    else if(requestType == "DELETE")
                    {
                        cout << "Delete request received" << endl;
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

    // Create PlatformConfig object
    PlatformConfig cfg {
        OC::ServiceType::InProc,
            OC::ModeType::Server,
            "0.0.0.0", // By setting to "0.0.0.0", it binds to all available interfaces
            0,         // Uses randomly available port
            OC::QualityOfService::LowQos,
            &ps
    };

    OCPlatform::Configure(cfg);
    try
    {
        // Create the instance of the resource class
        // (in this case instance of class 'LightResource').
        LightResource myLight;

        // Invoke createResource function of class light.
        myLight.createResource();
        std::cout << "Created resource." << std::endl;

        myLight.addType(std::string("core.brightlight"));
        myLight.addInterface(std::string(LINK_INTERFACE));
        std::cout << "Added Interface and Type" << std::endl;


        // A condition variable will free the mutex it is given, then do a non-
        // intensive block until 'notify' is called on it.  In this case, since we
        // don't ever call cv.notify, this should be a non-processor intensive version
        // of while(true);
        std::mutex blocker;
        std::condition_variable cv;
        std::unique_lock<std::mutex> lock(blocker);
        std::cout <<"Waiting" << std::endl;
        cv.wait(lock, []{return false;});
    }
    catch(OCException &e)
    {
        std::cout << "OCException in main : " << e.what() << endl;
    }

    // No explicit call to stop the platform.
    // When OCPlatform::destructor is invoked, internally we do platform cleanup

    return 0;
}
