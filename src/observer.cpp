// -*-c++-*-
//******************************************************************
//
// Copyright 2014 Intel Corporation.
// Copyright 2016 Samsung <philippe.coval@osg.samsung.com>
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
#include "config.h"
#include <cstdio>

#include "observer.h"
#include "main_view.h"

#include "logger.h"
#include "logger.inl"


#include <dlog.h>
#include <lbs-maps.h>

using namespace std;
using namespace OC;

IoTObserver *IoTObserver::mInstance = nullptr;
const OC::ObserveType IoTObserver::OBSERVE_TYPE_TO_USE = OC::ObserveType::Observe;


IoTObserver::IoTObserver()
{
    cout << __PRETTY_FUNCTION__ << endl;
    initializePlatform();
}

IoTObserver *IoTObserver::getInstance()
{
    cout << __PRETTY_FUNCTION__;
    if (IoTObserver::mInstance == nullptr)
    {
        mInstance = new IoTObserver;
    }
    return mInstance;
}

IoTObserver::~IoTObserver()
{
    cout << __PRETTY_FUNCTION__ << endl;
}

void IoTObserver::initializePlatform()
{
    m_platformConfig = make_shared<PlatformConfig>(ServiceType::InProc, ModeType::Client, "0.0.0.0",
                       0, OC::QualityOfService::HighQos);
    OCPlatform::Configure(*m_platformConfig);
    m_resourceDiscoveryCallback = bind(&IoTObserver::discoveredResource, this, placeholders::_1);
}

void IoTObserver::findResource()
{
    cout << __PRETTY_FUNCTION__ << endl;
    string coap_multicast_discovery = string(OC_RSRVD_WELL_KNOWN_URI);
    coap_multicast_discovery += "?if=";
    coap_multicast_discovery += Config::m_interface;
    OCConnectivityType connectivityType(CT_ADAPTER_IP);
    OCPlatform::findResource("", coap_multicast_discovery.c_str(),
                             connectivityType,
                             m_resourceDiscoveryCallback,
                             OC::QualityOfService::LowQos // TODO
                            );
}

void IoTObserver::discoveredResource(shared_ptr<OCResource> resource)
{
    cout << __PRETTY_FUNCTION__ << endl;
    try
    {
        if (resource)
        {
            string resourceUri = resource->uri();
            string hostAddress = resource->host();

            cout << "\nFound Resource" << endl << "Resource Types:" << endl;
            for (auto & resourceTypes : resource->getResourceTypes())
            {
                cout << "\t" << resourceTypes << endl;
            }

            cout << "Resource Interfaces: " << endl;
            for (auto & resourceInterfaces : resource->getResourceInterfaces())
            {
                cout << "\t" << resourceInterfaces << endl;
            }
            cout << "Resource uri: " << resourceUri << endl;
            cout << "host: " << hostAddress << endl;

            if (resourceUri == Config::m_endpoint)
            {
                QueryParamsMap test;
                resource->observe(OBSERVE_TYPE_TO_USE, test, &IoTObserver::onObserve);

            }
        }
    }
    catch (OCException &ex)
    {
        cerr << "Caught exception in discoveredResource: " << ex.reason() << endl;
    }
}


void handleObserve()
{

			static double lat = 52.165;
			static double lon = -2.21;

			if (rep.hasAttribute("lat")) {
				lat = rep.getValue<double>("lat");
			}
			if (rep.hasAttribute("lon")) {
				lon = rep.getValue<double>("lon");
			}

			dlog_print(DLOG_INFO, LOG_TAG, "location: %f,%f", lat, lon);
			map_region_show(lon, lat);
}


void IoTObserver::onObserve(const HeaderOptions /*headerOptions*/, const OCRepresentation &rep,
                            const int &eCode, const int &sequenceNumber)
{
    cout << __PRETTY_FUNCTION__ << endl;
    try
    {
        if (eCode == OC_STACK_OK && sequenceNumber != OC_OBSERVE_NO_OPTION)
        {
            if (sequenceNumber == OC_OBSERVE_REGISTER)
            {
                std::cout << "Observe registration action is successful" << std::endl;
            }
            else if (sequenceNumber == OC_OBSERVE_DEREGISTER)
            {
                std::cout << "Observe De-registration action is successful" << std::endl;
            }

            std::cout << "OBSERVE RESULT:" << std::endl;
            std::cout << "\tSequenceNumber: " << sequenceNumber << std::endl;
            std::string data;
            rep.getValue( Config::m_key, data);

            std::cout << Config::m_key << "=" << data << std::endl;
        }

	handleObserve();

        else
        {
            if (sequenceNumber == OC_OBSERVE_NO_OPTION)
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
    catch (std::exception &e)
    {
        std::cout << "Exception: " << e.what() << " in onObserve" << std::endl;
    }

}

#if 0
#define main observer_main


int main(int argc, char *argv[])
{
    IoTObserver observer;
    cout << "Performing Discovery..." << endl;
    observer.findResource();
    int choice = 0;
    do
    {
        cin >> choice;
        switch (choice)
        {
            case 9:
            default:
                return 0;
        }
    }
    while (choice != 9);
    return 0;
}
#endif