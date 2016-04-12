#ifndef _LBS_MAPS_ROUTE_VIEW_H_
#define _LBS_MAPS_ROUTE_VIEW_H_

#include <Evas.h>
#include <Elementary.h>

#define ROUTE_VIEW_EDJ_FILE "edje/routeview.edj"

void create_route_view(Evas_Object *parent);

void on_route_result();

#endif	/* _LBS_MAPS_ROUTE_VIEW_H_ */
