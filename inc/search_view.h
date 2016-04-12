#ifndef _LBS_MAPS_SEARCH_VIEW_H_
#define _LBS_MAPS_SEARCH_VIEW_H_

#include <Evas.h>
#include <Elementary.h>

#define SEARCH_VIEW_EDJ_FILE "edje/searchview.edj"

Evas_Object * create_search_view(Evas_Object *parent);

void on_poi_result(int res_cnt);

#endif	/* _LBS_MAPS_SEARCH_VIEW_H_ */
