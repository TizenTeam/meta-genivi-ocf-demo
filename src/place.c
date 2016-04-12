#include "lbs-maps.h"
#include "place.h"
#include "main_view.h"
#include "search_view.h"
#include <locations.h>
#include <dlog.h>

#define POI_REQ_ID_IDLE -1
#define POI_SERVICE_CATEGORY_SEARCH_RADIUS 5000	/* meters */

int __request_id = -1;

char*
__get_category_name(place_type category)
{
	switch (category) {
	case food_drink:
		return strdup("eat-drink");
	case transport:
		return strdup("transport");
	case accommodation:
		return strdup("accommodation");
	case shopping:
		return strdup("shopping");
	case leisure_outdoor:
		return strdup("leisure-outdoor");
	default:
		break;
	}

	return strdup("");
}

static bool
__maps_service_search_place_cb(maps_error_e error, int request_id , int index, int length , maps_place_h place , void *user_data)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "Place result >> index [%d]/ length [%d]", index, length);

	place_s** place_result = (void *) user_data;

	maps_coordinates_h coordinates;
	static double cur_lat, cur_lon;
	double distance = 0.0;

	if (error != MAPS_ERROR_NONE) {
		__request_id = POI_REQ_ID_IDLE;
		on_poi_result(length);
		return false;
	}

	char *name = NULL;
	double latitude = 0.0, longitude = 0.0;

	if (index == 0)
		map_get_poi_lat_lng(&cur_lat, &cur_lon);

	place_result[index] = (place_s *) malloc(sizeof(place_s));

	/* Place Name */
	maps_place_get_name(place , &name);
	strcpy(place_result[index]->__place_name, name);

	/* Place Location */
	maps_place_get_location(place,  &coordinates);
	maps_coordinates_get_latitude(coordinates, &latitude);
	maps_coordinates_get_longitude(coordinates, &longitude);
	place_result[index]->__lat = latitude;
	place_result[index]->__lon = longitude;
	maps_coordinates_destroy(coordinates);

	/* Distance Calculation */
	location_manager_get_distance(cur_lat, cur_lon, latitude, longitude, &distance);
	distance = distance * 0.001;
	place_result[index]->__distance = distance;

	if (index == (length-1))
		on_poi_result(length);

	__request_id = POI_REQ_ID_IDLE;

	/* Release the place result */
	maps_place_destroy(place);
	return true;
}

int
request_place(maps_service_h maps, double lat, double lon, char *search_keyword, place_type category_type, int distance, int max_results, place_s **place_res)
{
	int ret = 0;
	int request_id = 0;

	maps_place_filter_h filter = NULL;
	maps_preference_h preference = NULL;
	maps_place_category_h category = NULL;
	maps_coordinates_h coords = NULL;

	if (distance < 1)
		distance = POI_SERVICE_CATEGORY_SEARCH_RADIUS;

	maps_preference_create(&preference);	/* Create Maps Preference */
	maps_place_filter_create(&filter);		/* Create Maps Place Filter */

	if (category_type != none_category) {
		/* CATEGORY Search */
		maps_place_category_create(&category);	/* Create place_category */
		maps_place_category_set_id(category, __get_category_name(category_type));	/* Set Category Name for place search */

		ret = maps_place_filter_set_category(filter, category);	/* Set place_category to filter */

		if (ret != MAPS_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "maps_place_filter_set error [%d]", ret);
			goto EXIT;
		}
	} else {
		/* Keyword Search */
		dlog_print(DLOG_DEBUG, LOG_TAG, "poi search keyword : %s", search_keyword);

		ret = maps_place_filter_set_place_name(filter, search_keyword);	/* Keyword Search. No POI category search */
		if (ret != MAPS_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "maps_place_filter_set error [%d]", ret);
			goto EXIT;
		}
	}

	maps_preference_set_max_results(preference, max_results);	/* Set Max Results Preference */

	maps_coordinates_create(lat, lon, &coords);	/* Create maps coordinates for radius search */
	if (coords == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "coords is NULL");
		goto EXIT;
	}

	/* Places Search */
	ret = maps_service_search_place(maps,
							coords,
							distance,
							filter,
							preference,
							__maps_service_search_place_cb,
							(void *) place_res,
							&request_id);

	if (ret != MAPS_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "failed to poi_service_search.");
		__request_id = POI_REQ_ID_IDLE;
		goto EXIT;
	} else {
		__request_id = request_id;
		dlog_print(DLOG_ERROR, LOG_TAG, "request_id : %d", __request_id);
	}

EXIT:
	if (coords)
		maps_coordinates_destroy(coords);
	if (category)
		maps_place_category_destroy(category);
	if (filter)
		maps_place_filter_destroy(filter);
	maps_preference_destroy(preference);

	return ret;
}

bool
cancel_place_request(maps_service_h maps)
{
	if (__request_id != POI_REQ_ID_IDLE) {
		if (maps_service_cancel_request(maps, __request_id) == MAPS_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Place Request Cancelled");
			__request_id = POI_REQ_ID_IDLE;
			return true;
		} else {
			dlog_print(DLOG_ERROR, LOG_TAG, "Place Cancel Request failed");
		}
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "No Place Request ongoing to be cancelled");
	}

	return false;
}
