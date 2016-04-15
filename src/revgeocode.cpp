#include "config.h"

#include "lbs-maps.h"
#include "revgeocode.h"
#include "main_view.h"

#define GEO_REQ_ID_IDLE -1

revgeocode_s *revgeocode_result = NULL;
static int __request_id;

void
__maps_service_reverse_geocode_cb(maps_error_e result, int request_id, int index, int total, maps_address_h address, void *user_data)
{
	__request_id = GEO_REQ_ID_IDLE;

	if (result != MAPS_ERROR_NONE) {
		/* Invalid Result */
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid Reverse Geocode Result");
		handle_addr_notification(NULL);
		return;
	}

	char resultText[1024] = {0, };
	strcpy(resultText, "");

	char *street = NULL;
	maps_address_get_street(address, &street);
	if (street != NULL) {
		strcat(resultText, street);
		free(street);
		street = NULL;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Street is NULL");
	}

	char *district = NULL;
	maps_address_get_district(address, &district);
	if (district != NULL) {
		if (strlen(resultText) > 0)
			strcat(resultText, ", ");
		strcat(resultText, district);
		free(district);
		district = NULL;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "District is NULL");
	}

	char *city = NULL;
	maps_address_get_city(address, &city);
	if (city != NULL) {
		if (strlen(resultText) > 0)
			strcat(resultText, ", ");
		strcat(resultText, city);
		free(city);
		city = NULL;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "City is NULL");
	}

	char *state = NULL;
	maps_address_get_state(address, &state);
	if (state != NULL) {
		if (strlen(resultText) > 0)
			strcat(resultText, ", ");
		strcat(resultText, state);
		free(state);
		state = NULL;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "State is NULL");
	}

	char *country = NULL;
	maps_address_get_country(address, &country);
	if (country != NULL) {
		if (strlen(resultText) > 0)
			strcat(resultText, ", ");
		strcat(resultText, country);
		free(country);
		country = NULL;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Country is NULL");
	}

	char *country_code = NULL;
	maps_address_get_country_code(address, &country_code);
	if (country_code != NULL) {
		if (strlen(resultText) > 0)
			strcat(resultText, ", ");
		strcat(resultText, country_code);
		free(country_code);
		country_code = NULL;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Country Code is NULL");
	}

	char *county = NULL;
	maps_address_get_county(address, &county);
	if (county != NULL) {
		if (strlen(resultText) > 0)
			strcat(resultText, ", ");
		strcat(resultText, county);
		free(county);
		county = NULL;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "County is NULL");
	}

	char *postal_code = NULL;
	maps_address_get_postal_code(address, &postal_code);
	if (postal_code != NULL) {
		if (strlen(resultText) > 0)
			strcat(resultText, ", ");
		strcat(resultText, postal_code);
		free(postal_code);
		postal_code = NULL;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Postal Code is NULL");
	}

	if (strlen(resultText) > 0) {
		revgeocode_result = (revgeocode_s *)malloc(sizeof(revgeocode_s));

		if (!revgeocode_result) {
			handle_addr_notification(NULL);
			return ;
		}

		char *top = strdup(resultText);
		strtok(top, ",");
		if (top == NULL || !strcmp(top, resultText)) {
			if (top) free(top);
			strcpy(top, "Selected location");
		}
		dlog_print(DLOG_ERROR, LOG_TAG, "addr %s, set name as %s", resultText, top);
		strcpy(revgeocode_result->name, top);
		if (strlen(resultText) > 0)
			strcpy(revgeocode_result->address, &resultText[strlen(top) + 2]);
		else
			strcpy(revgeocode_result->address, (char *)"Not found");
	} else
		revgeocode_result = NULL;

	handle_addr_notification(revgeocode_result);

	maps_address_destroy(address);
}

int
request_revgeocode(maps_service_h maps, double latitude, double longitude)
{
	int request_id = -1;

	maps_preference_h preference = NULL;
	maps_preference_create(&preference);

	int error = maps_service_reverse_geocode(maps,					/* maps service handle */
						latitude,				/* center location */
						longitude,				/* search radius from center location */
						preference,				/* reverse geocode preference */
						__maps_service_reverse_geocode_cb,	/* callback */
						(void *)NULL,				/* user_data */
						&request_id);				/* request_id */

	if (error != MAPS_ERROR_NONE)
		__request_id = GEO_REQ_ID_IDLE;
	else
		__request_id = request_id;

	maps_preference_destroy(preference);

	return error;
}

bool
cancel_revgeocode_request(maps_service_h maps)
{
	if (__request_id != GEO_REQ_ID_IDLE) {
		if (maps_service_cancel_request(maps, __request_id) == MAPS_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Reverse Geocode Request Cancelled");
			__request_id = GEO_REQ_ID_IDLE;
			return true;
		} else {
			dlog_print(DLOG_ERROR, LOG_TAG, "Reverse Geocode Cancel Request failed");
		}
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "No Reverse Geocode Request ongoing to be cancelled");
	}

	return false;
}
