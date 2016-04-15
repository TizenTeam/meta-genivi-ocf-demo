// -*-c++-*-
//******************************************************************
//
// Copyright 2014 Intel Corporation.
// Copyright 2015 Eurogiciel <philippe.coval@eurogiciel.fr>
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
#include <unistd.h>
#include <ctime>
#include "client.h"

using namespace std;
using namespace OC;


Resource::Resource(shared_ptr<OCResource> resource)
{
    m_resourceHandle = resource;
    m_GETCallback = bind(&Resource::onGet, this, placeholders::_1, placeholders::_2, placeholders::_3);
    m_PUTCallback = bind(&Resource::onPut, this, placeholders::_1, placeholders::_2, placeholders::_3);
}

Resource::~Resource()
{
}


shared_ptr<Resource> IoTClient::getPlatformResource()
{
    return m_platformResource;
}


void Resource::get()
{
    QueryParamsMap params;
    m_resourceHandle->get(params, m_GETCallback);
}

void Resource::put(std::string data)
{
    QueryParamsMap params;
    OCRepresentation rep;
    rep.setValue(Config::m_key, data);

    static double lat = 52.165;
    static double lon = -2.21;
    lat += 0.01;
    lon += 0.01;

    rep.setValue("lat", lat);
    rep.setValue("lon", lon);
    m_resourceHandle->put(rep, params, m_PUTCallback);
}

void Resource::onGet(const HeaderOptions &headerOptions, const OCRepresentation &representation,
                     int errCode)
{
    if (errCode == OC_STACK_OK)
    {
        int value;
        representation.getValue(Config::m_key, value);
        cout << endl << endl << "Resource switch state is: " << value << endl;
    }
    else
    {
        cerr << endl << endl << "Error in GET response from Resource resource" << endl;
    }
    IoTClient::DisplayMenu();
}

void Resource::onPut(const HeaderOptions &headerOptions, const OCRepresentation &representation,
                     int errCode)
{
    if (errCode == OC_STACK_OK)
    {
        int value;
        representation.getValue(Config::m_key, value);
        cout << endl << endl << "Set Resource switch to: " << value << endl;
    }
    else
    {
        cerr << endl << endl << "Error in PUT response from Resource resource" << endl;
    }
    IoTClient::DisplayMenu();
}


IoTClient::IoTClient()
{
    cout << "Running IoTClient constructor" << endl;
    initializePlatform();
}

IoTClient::~IoTClient()
{
    cout << "Running IoTClient destructor" << endl;
}


IoTClient *IoTClient::mInstance = nullptr;

IoTClient *IoTClient::getInstance()
{
    if ( IoTClient::mInstance == 0 )
    {
        mInstance = new IoTClient;
    }
    return mInstance;
}

void IoTClient::initializePlatform()
{
    m_platformConfig = make_shared<PlatformConfig>(ServiceType::InProc, ModeType::Client, "0.0.0.0",
                       0, OC::QualityOfService::HighQos);
    OCPlatform::Configure(*m_platformConfig);
    m_resourceDiscoveryCallback = bind(&IoTClient::discoveredResource, this, placeholders::_1);
}

void IoTClient::findResource()
{
    string coap_multicast_discovery = string(OC_RSRVD_WELL_KNOWN_URI "?if=" );
    coap_multicast_discovery += Config::m_interface;
    OCConnectivityType connectivityType(CT_ADAPTER_IP);
    OCPlatform::findResource("", coap_multicast_discovery.c_str(),
                             connectivityType,
                             m_resourceDiscoveryCallback,
                             OC::QualityOfService::LowQos);
}

void IoTClient::discoveredResource(shared_ptr<OCResource> resource)
{
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
                m_platformResource = make_shared<Resource>(resource);
            }
        }
        IoTClient::DisplayMenu();
    }
    catch (OCException &ex)
    {
        cerr << "Caught exception in discoveredResource: " << ex.reason() << endl;
    }
}



void IoTClient::DisplayMenu()
{
    cout << "\nEnter:" << endl
         << "*) Display this menu" << endl
         << "9) Quit" << endl;
}


#if 1
#define main client_main
#endif

int main(int argc, char *argv[])
{
    IoTClient client;
    cout << "Performing Discovery..." << endl;
    client.findResource();
    int choice = 0;

    do
    {

        string data = "0,0";

        time_t const now = time(0);
        char *dt = ctime(&now);
        data = string(dt);

        if (client.getPlatformResource())
        {
            client.getPlatformResource()->put(data);
        }
        else
        {
            cout << "Resource resource not yet discovered" << endl;
        }
        cerr << "sleeping" << endl;
        sleep(10);
        cerr << "slept:" << choice << endl;

    }
    while (choice != 9);
    return 0;
}
