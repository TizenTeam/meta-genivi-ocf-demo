#ifndef _LBS_MAPS_REVGEOCODE_H_
#define _LBS_MAPS_REVGEOCODE_H_

#include <glib.h>
#include <stdlib.h>
#include <maps_service.h>

typedef struct revgeocode {
	char name[PATH_MAX];
	char address[PATH_MAX];
} revgeocode_s;

int request_revgeocode(maps_service_h handle, double latitude, double longitude);
bool cancel_revgeocode_request(maps_service_h handle);

#endif	/* _LBS_MAPS_REVGEOCODE_H_ */
