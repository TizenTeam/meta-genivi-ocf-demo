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

// OCClient.cpp : Defines the entry point for the console application.
//
#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include "OCPlatform.h"
#include "OCApi.h"
#include "gateway.h"

using namespace OC;

typedef std::map<OCResourceIdentifier, std::shared_ptr<OCResource>> DiscoveredResourceMap;
DiscoveredResourceMap discoveredResources;

std::shared_ptr<OCResource> curResource;
static ObserveType OBSERVE_TYPE_TO_USE = ObserveType::Observe;
std::mutex curResourceLock;

class Light
{
    public:
	std::string m_name;
	std::vector <std::string> props;
        Light() : m_name("") {}
};

Light mylight;

int observe_count()
{
    static int oc = 0;
    return ++oc;
}

void onObserve(const HeaderOptions /*headerOptions*/, const OCRepresentation& rep,
        const int& eCode, const int& sequenceNumber)
{
    try
    {
        if(eCode == OC_STACK_OK && sequenceNumber != OC_OBSERVE_NO_OPTION)
        {
            if(sequenceNumber == OC_OBSERVE_REGISTER)
            {
                std::cout << "Observe registration action is successful" << std::endl;
            }
            else if(sequenceNumber == OC_OBSERVE_DEREGISTER)
            {
                std::cout << "Observe De-registration action is successful" << std::endl;
            }

            std::cout << "OBSERVE RESULT:"<<std::endl;
            std::cout << "\tSequenceNumber: "<< sequenceNumber << std::endl;
            rep.getValue("name", mylight.m_name);

            std::cout << "\tname: " << mylight.m_name << std::endl;

            if(observe_count() == 11)
            {
                std::cout<<"Cancelling Observe..."<<std::endl;
                OCStackResult result = curResource->cancelObserve();

                std::cout << "Cancel result: "<< result <<std::endl;
                sleep(10);
                std::cout << "DONE"<<std::endl;
                std::exit(0);
            }
        }
        else
        {
            if(sequenceNumber == OC_OBSERVE_NO_OPTION)
            {
                std::cout << "Observe registration or de-registration action is failed" << std::endl;
            }
            else
            {
                std::cout << "onObserve Response error: " << eCode << std::endl;
                std::exit(-1);
            }
        }
    }
    catch(std::exception& e)
    {
        std::cout << "Exception: " << e.what() << " in onObserve" << std::endl;
    }

}

void onPost2(const HeaderOptions& /*headerOptions*/,
        const OCRepresentation& rep, const int eCode)
{
    try
    {
        if(eCode == OC_STACK_OK || eCode == OC_STACK_RESOURCE_CREATED)
        {
            std::cout << "POST request was successful" << std::endl;

            if(rep.hasAttribute("createduri"))
            {
                std::cout << "\tUri of the created resource: "
                    << rep.getValue<std::string>("createduri") << std::endl;
            }
            else
            {
                rep.getValue("name", mylight.m_name);
                std::cout << "\tname: " << mylight.m_name << std::endl;
            }

            if (OBSERVE_TYPE_TO_USE == ObserveType::Observe)
                std::cout << std::endl << "Observe is used." << std::endl << std::endl;
            else if (OBSERVE_TYPE_TO_USE == ObserveType::ObserveAll)
                std::cout << std::endl << "ObserveAll is used." << std::endl << std::endl;

            curResource->observe(OBSERVE_TYPE_TO_USE, QueryParamsMap(), &onObserve);

        }
        else
        {
            std::cout << "onPost2 Response error: " << eCode << std::endl;
            std::exit(-1);
        }
    }
    catch(std::exception& e)
    {
        std::cout << "Exception: " << e.what() << " in onPost2" << std::endl;
    }

}

void onPost(const HeaderOptions& /*headerOptions*/,
        const OCRepresentation& rep, const int eCode)
{
    try
    {
        if(eCode == OC_STACK_OK || eCode == OC_STACK_RESOURCE_CREATED)
        {
            std::cout << "POST request was successful" << std::endl;

            if(rep.hasAttribute("createduri"))
            {
                std::cout << "\tUri of the created resource: "
                    << rep.getValue<std::string>("createduri") << std::endl;
            }
            else
            {
                rep.getValue("name", mylight.m_name);
                std::cout << "\tname: " << mylight.m_name << std::endl;
            }

            OCRepresentation rep2;

            std::cout << "Posting light representation..."<<std::endl;
            curResource->post(rep2, QueryParamsMap(), &onPost2);
        }
        else
        {
            std::cout << "onPost Response error: " << eCode << std::endl;
            std::exit(-1);
        }
    }
    catch(std::exception& e)
    {
        std::cout << "Exception: " << e.what() << " in onPost" << std::endl;
    }
}

// Local function to put a different state for this resource
void postLightRepresentation(std::shared_ptr<OCResource> resource)
{
    if(resource)
    {
        OCRepresentation rep;

        std::cout << "Posting light representation..."<<std::endl;

        // Invoke resource's post API with rep, query map and the callback parameter
        resource->post(rep, QueryParamsMap(), &onPost);
    }
}

// callback handler on PUT request
void onPut(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
    try
    {
        if(eCode == OC_STACK_OK)
        {
            std::cout << "PUT request was successful" << std::endl;
            rep.getValue("name", mylight.m_name);
            std::cout << "\tname: " << mylight.m_name << std::endl;

            postLightRepresentation(curResource);
        }
        else
        {
            std::cout << "onPut Response error: " << eCode << std::endl;
            std::exit(-1);
        }
    }
    catch(std::exception& e)
    {
        std::cout << "Exception: " << e.what() << " in onPut" << std::endl;
    }
}

// Local function to put a different state for this resource
void putLightRepresentation(std::shared_ptr<OCResource> resource)
{
    if(resource)
    {
        OCRepresentation rep;

        std::cout << "Putting light representation..."<<std::endl;
        // Invoke resource's put API with rep, query map and the callback parameter
        resource->put(rep, QueryParamsMap(), &onPut);
    }
}

// Callback handler on GET request
void onGet(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
    try
    {
        if(eCode == OC_STACK_OK)
        {
            std::cout << "GET request was successful" << std::endl;
            std::cout << "Resource URI: " << rep.getUri() << std::endl;
            rep.getValue("name", mylight.m_name);
            std::cout << "\tname: " << mylight.m_name << std::endl;

            putLightRepresentation(curResource);
        }
        else
        {
            std::cout << "onGET Response error: " << eCode << std::endl;
            std::exit(-1);
        }
    }
    catch(std::exception& e)
    {
        std::cout << "Exception: " << e.what() << " in onGet" << std::endl;
    }
}

// Local function to get representation of light resource
void getLightRepresentation(std::shared_ptr<OCResource> resource)
{
    if(resource)
    {
        std::cout << "Getting Light Representation..."<<std::endl;
        // Invoke resource's get API with the callback parameter

        QueryParamsMap test;
        resource->get(test, &onGet);
    }
}

// Callback to found resources
void foundResource(std::shared_ptr<OCResource> resource)
{
    std::cout << "In foundResource\n";
    std::string resourceURI;
    std::string hostAddress;
    try
    {
        {
            std::lock_guard<std::mutex> lock(curResourceLock);
            if(discoveredResources.find(resource->uniqueIdentifier()) == discoveredResources.end())
            {
                std::cout << "Found resource " << resource->uniqueIdentifier() <<
                    " for the first time on server with ID: "<< resource->sid()<<std::endl;
                discoveredResources[resource->uniqueIdentifier()] = resource;
            }
            else
            {
                std::cout<<"Found resource "<< resource->uniqueIdentifier() << " again!"<<std::endl;
            }

            if(curResource)
            {
                std::cout << "Found another resource, ignoring"<<std::endl;
                return;
            }
        }

        // Do some operations with resource object.
        if(resource)
        {
            std::cout<<"DISCOVERED Resource:"<<std::endl;
            // Get the resource URI
            resourceURI = resource->uri();
            std::cout << "\tURI of the resource: " << resourceURI << std::endl;

            // Get the resource host address
            hostAddress = resource->host();
            std::cout << "\tHost address of the resource: " << hostAddress << std::endl;

            // Get the resource types
            std::cout << "\tList of resource types: " << std::endl;
            for(auto &resourceTypes : resource->getResourceTypes())
            {
                std::cout << "\t\t" << resourceTypes << std::endl;
            }

            // Get the resource interfaces
            std::cout << "\tList of resource interfaces: " << std::endl;
            for(auto &resourceInterfaces : resource->getResourceInterfaces())
            {
                std::cout << "\t\t" << resourceInterfaces << std::endl;
            }

            if(resourceURI == "/a/light")
            {
                curResource = resource;
                // Call a local function which will internally invoke get API on the resource pointer
                getLightRepresentation(resource);
            }
        }
        else
        {
            // Resource is invalid
            std::cout << "Resource is invalid" << std::endl;
        }

    }
    catch(std::exception& e)
    {
        std::cerr << "Exception in foundResource: "<< e.what() << std::endl;
    }
}

static FILE* client_open(const char* /*path*/, const char *mode)
{
    return fopen("./oic_svr_db_client.json", mode);
}

void *client_thread(void *data)
{
    std::ostringstream requestURI;
    OCPersistentStorage ps {client_open, fread, fwrite, fclose, unlink };
    // Create PlatformConfig object
    PlatformConfig cfg {
        OC::ServiceType::InProc,
            OC::ModeType::Both,
            "0.0.0.0",
            0,
            OC::QualityOfService::LowQos,
            &ps
    };

    OCPlatform::Configure(cfg);

	sm_session *s = (sm_session *) data;
	session_data sd;
	fd_set R, W, X;
	int rc = 0;

	fprintf(stdout,"Creating OCF Client thread %p\n", s);
	FD_ZERO(&R);
	FD_ZERO(&W);
	FD_ZERO(&X);
	FD_SET(s->creqsock[0], &R);
	int max_sd = s->creqsock[0];

	 //wait for request, indefinitely
	while (true) {
		fprintf(stdout,"OIC Client Waiting");
		rc = select(max_sd + 1, &R, &W, &X, NULL);
		//process the OCF Client Request
		if (rc > 0) {
			bool isAppReq = FD_ISSET(s->creqsock[0], &R);
			if (isAppReq) {
				int sock = s->creqsock[0];
				fprintf(stdout,"MAX FD IS SET max_sd = %d\n", max_sd);
				int bytes = read(sock, &sd, sizeof(session_data));
				fprintf(stdout,"Read Request Size = %d....waiting\n", bytes);
				if (bytes > 0) {
					sm_request *r = (sm_request *) sd.rHandle;
					try
					{
						std::cout.setf(std::ios::boolalpha);
						requestURI << OC_RSRVD_WELL_KNOWN_URI;// << "?rt=core.light";
						OCPlatform::findResource("\n", requestURI.str(),
								CT_DEFAULT, &foundResource);
						std::cout<< "Finding Resource... " <<std::endl;
					}catch(OCException& e)
					{
						oclog() << "Exception in main: "<<e.what();
					}
				}
			}
		}
	}
    return 0;
}
