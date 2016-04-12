#ifndef _LBS_MAPS_PLACE_H_
#define _LBS_MAPS_PLACE_H_

#include <glib.h>
#include <stdlib.h>
#include <maps_service.h>

typedef enum Place_Type
{
	food_drink = 0,
	transport,
	accommodation,
	shopping,
	leisure_outdoor,
	none_category
} place_type;

typedef struct place {
	double __lat;
	double __lon;
	double __distance;
	char __place_name[PATH_MAX];
} place_s;

int request_place(maps_service_h maps, double lat, double lon, char* _searchKeyword, place_type category_type, int distance, int maxResults, place_s **place_result);
bool cancel_place_request(maps_service_h handle);

#endif	/* _LBS_MAPS_PLACE_H_ */
