#ifndef _LBS_MAPS_MAP_VIEW_H_
#define _LBS_MAPS_MAP_VIEW_H_

#include <Evas.h>
#include <Elementary.h>


#define MAP_VIEW_EDJ_FILE "edje/mainview.edj"

typedef enum
{
	MAPS_VIEW_MODE_MY_LOCATION,
	MAPS_VIEW_MODE_POI_INFO,
	MAPS_VIEW_MODE_DIRECTION
} MAP_VIEW_MODE;


Evas_Object* create_map_view(Evas_Object *parent);

void map_get_poi_lat_lng(double *latitude, double *longitude);
void map_get_selected_poi_lat_lng(double *latitude, double *longitude, char **name);


void hide_map_poi_overlays(Eina_Bool enable);
void remove_map_poi_overlays();
void hide_map_maneuver_overlays(Eina_Bool enable);
void remove_map_maneuver_overlays();

void set_view_type(MAP_VIEW_MODE type);

void map_region_show(double latitude, double longitude);
#endif
