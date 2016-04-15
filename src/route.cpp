#include "config.h"

#include "lbs-maps.h"
#include "route.h"
#include "route_view.h"
#include "main_view.h"
#include <locations.h>
#include <dlog.h>

#define ROUTE_REQ_ID_IDLE -1

const char *turn_id_string[] = {
		"Undefined",
		"Continue going straight",
		"Keep right",
		"Slight right",
		"Turn right",
		"Make sharp right turn",
		"Right U Turn",
		"Left U Turn",
		"Make sharp left turn",
		"Turn left",
		"Slight left",
		"Keep left",
		"Right Fork",
		"Left Fork",
		"Straight Fork"
	};

int __route_request_id = -1;
location_coords_s segment_origin, segment_destination;
int __maneuver_count = 0;

bool
__maps_route_segment_maneuver_cb(int index, int total, maps_route_maneuver_h maneuver, void *user_data)
{
	route_s **route_result = (route_s **) user_data;

	double _distance = 0.0;
	maps_route_maneuver_get_distance_to_next_instruction(maneuver, &_distance);

	/* Route Segment Maneuver Instruction Text */
	char *instruction = NULL;
	maps_route_maneuver_get_instruction_text(maneuver, &instruction);

	if (instruction) {
		strcpy(((*route_result)->__maneuver[__maneuver_count]).__instruction, instruction);
		free(instruction);
		instruction = NULL;
	} else
		strcpy(((*route_result)->__maneuver[__maneuver_count]).__instruction, "");

	/* Route Segment Maneuver Road Name */
	char *road_name = NULL;
	maps_route_maneuver_get_road_name(maneuver, &road_name);

	if (road_name) {
		strcpy(((*route_result)->__maneuver[__maneuver_count]).__street_name, road_name);
		free(road_name);
		road_name = NULL;
	} else
		strcpy(((*route_result)->__maneuver[__maneuver_count]).__street_name, "");

	((*route_result)->__maneuver[__maneuver_count]).__origin_lat = segment_origin.latitude;
	((*route_result)->__maneuver[__maneuver_count]).__origin_lon = segment_origin.longitude;
	((*route_result)->__maneuver[__maneuver_count]).__dest_lat = segment_destination.latitude;
	((*route_result)->__maneuver[__maneuver_count]).__dest_lon = segment_destination.longitude;

	/* Route Segment Maneuver turn type */
	maps_route_turn_type_e turn_type = MAPS_ROUTE_TURN_TYPE_NONE;
	maps_route_maneuver_get_turn_type(maneuver, &turn_type);

	strcpy(((*route_result)->__maneuver[__maneuver_count]).__turn_string, turn_id_string[turn_type]);

	__maneuver_count++;

	maps_route_maneuver_destroy(maneuver);
	return true;
}

bool
__maps_route_segment_cb(int index, int total, maps_route_segment_h segment, void *user_data)
{
	if (!segment) {
		dlog_print(DLOG_ERROR, LOG_TAG, "critical error : FAILED");
		return false;
	}

	maps_coordinates_h origin = NULL, destination = NULL;

	/* Segment Origin Coordinates */
	maps_route_segment_get_origin(segment, &origin);
	maps_coordinates_get_latitude(origin, &(segment_origin.latitude));
	maps_coordinates_get_longitude(origin, &(segment_origin.longitude));
	maps_coordinates_destroy(origin);

	dlog_print(DLOG_DEBUG, LOG_TAG, "Segment Origin Lat : %f, Lon : %f", segment_origin.latitude, segment_origin.longitude);

	/* Segment Destination Coordinates */
	maps_route_segment_get_destination(segment, &destination);
	maps_coordinates_get_latitude(destination, &(segment_destination.latitude));
	maps_coordinates_get_longitude(destination, &(segment_destination.longitude));
	maps_coordinates_destroy(destination);

	dlog_print(DLOG_DEBUG, LOG_TAG, "Segment Destination Lat : %f, Lon : %f", segment_destination.latitude, segment_destination.longitude);

	/* Segment Maneuvers information */
	maps_route_segment_foreach_maneuver(segment, __maps_route_segment_maneuver_cb, user_data);

	maps_route_segment_destroy(segment);
	return true;
}

bool
__parse_route_data(maps_route_h route, int index, int total, void *user_data)
{
	route_s **route_result = (route_s **) user_data;

	maps_distance_unit_e dist_unit;
	double distance = 0;
	long duration = 0;
	int ret = 0;

	/* Cloning the route */
	if (ret != MAPS_ERROR_NONE || !route) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Route Response error [%d]", ret);
		return false;
	}

	/* Route - Total Distance */
	maps_route_get_total_distance(route, &distance);
	/* Route - Distance Unit */
	maps_route_get_distance_unit(route, &dist_unit);

	/* Changing everything to KM (by default) */
	if (dist_unit == MAPS_DISTANCE_UNIT_M) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "distance : %.5f m", distance);
		distance = distance / 1000;	/* convert the distance into km */
	} else if (dist_unit == MAPS_DISTANCE_UNIT_KM) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "distance : %.5f km", distance);
	}

	/* Route - Total Duration */
	maps_route_get_total_duration(route, &duration);
	dlog_print(DLOG_DEBUG, LOG_TAG, "duration : %ld sec", duration);
	duration = (duration + 30) / 60;	/*converting duration to minutes */

	(*route_result) = (route_s *)malloc(sizeof(route_s));

	(*route_result)->__distance = distance;
	(*route_result)->__duration = duration;

	__maneuver_count = 0;

	/* Check if maneuver path data is supported */
	bool supported = false;
	int error = maps_service_provider_is_data_supported(get_maps_service_handle(),
											MAPS_ROUTE_SEGMENTS_PATH, &supported);
	const bool is_route_segment_path_supported = (error == MAPS_ERROR_NONE) ? supported : false;

	/* Check if maneuver sements data is supported */
	error = maps_service_provider_is_data_supported(get_maps_service_handle(),
											MAPS_ROUTE_SEGMENTS_MANEUVERS, &supported);
	const bool is_route_segment_maneuvers_supported = (error == MAPS_ERROR_NONE) ? supported : false;

	if ((is_route_segment_path_supported) && (is_route_segment_maneuvers_supported)) {
		/* Allow segment maneuvers and path usage */
		maps_route_foreach_segment(route, __maps_route_segment_cb, (void *)route_result);
	}

	(*route_result)->__maneuver_count = __maneuver_count;

	return true;
}

bool
__maps_service_search_route_cb(maps_error_e error, int request_id, int index, int total, maps_route_h route, void *user_data)
{
	route_s **route_result = (route_s **) user_data;

	__parse_route_data(route, index, total, (void *) user_data);

	dlog_print(DLOG_DEBUG, LOG_TAG, "distance :: %f", (*route_result)->__distance);

	on_route_result();

	maps_route_destroy(route);
	return true;
}

int
request_route(maps_service_h maps, double src_lat, double src_lon, double dest_lat, double dest_lon, route_s **res)
{
	if (maps == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "maps service handle is NULL");
		return false;
	}

	int request_id;

	maps_coordinates_h origin = NULL;
	maps_coordinates_h destination = NULL;

	/* Origin Coordinates */
	maps_coordinates_create(src_lat, src_lon, &origin);
	/* Destination Coordinates */
	maps_coordinates_create(dest_lat, dest_lon, &destination);

	/* Maps Preference */
	maps_preference_h preference = NULL;
	maps_preference_create(&preference);

	/* Set Route Transport mode Preference */
	/* Transport Mode - Car */
	maps_preference_set_route_transport_mode(preference, MAPS_ROUTE_TRANSPORT_MODE_CAR);

	/* Route Search */
	int error = maps_service_search_route(maps,
								origin,
								destination,
								preference,
								__maps_service_search_route_cb,
								(void *) res,
								&request_id);

	if (error == MAPS_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "request_id : %d", request_id);
		__route_request_id = request_id;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Route Service Request Failed ::  [%d]", error);
		__route_request_id = ROUTE_REQ_ID_IDLE;
	}

	maps_coordinates_destroy(origin);
	maps_coordinates_destroy(destination);

	maps_preference_destroy(preference);

	return error;
}

bool
cancel_route_request(maps_service_h maps)
{
	if (__route_request_id != ROUTE_REQ_ID_IDLE) {
		if (maps_service_cancel_request(maps, __route_request_id) == MAPS_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Route Request Cancelled");
			__route_request_id = ROUTE_REQ_ID_IDLE;
			return true;
		} else {
			dlog_print(DLOG_ERROR, LOG_TAG, "Route Cancel Request failed");
		}
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "No Route Request ongoing to be cancelled");
	}

	return false;
}
