#ifndef _LBS_MAPS_ROUTE_H_
#define _LBS_MAPS_ROUTE_H_

#include <glib.h>
#include <stdlib.h>
#include <maps_service.h>

typedef struct {
	double __origin_lat;
	double __origin_lon;
	double __dest_lat;
	double __dest_lon;
	double __distance;
	char __street_name[512];
	char __instruction[512];
	char __turn_string[128];
} route_maneuver_s;

typedef struct {
	double __distance;
	int __duration;

	route_maneuver_s __maneuver[100];
	int __maneuver_count;
} route_s;

int request_route(maps_service_h maps, double src_lat, double src_lon, double dest_lat, double dest_lon, route_s **res);
bool cancel_route_request(maps_service_h handle);

#endif	/* _LBS_MAPS_ROUTE_H_ */
