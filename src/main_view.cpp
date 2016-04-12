#include <Evas.h>
#include <Ecore.h>
#include <efl_extension.h>
#include <maps_service.h>
#include "lbs-maps.h"
#include "main_view.h"
#include "search_view.h"
#include "route_view.h"
#include "util.h"

#define MAPS_PROVIDER	"HERE"
#define PROVIDER_TEST_KEY	"Insert your key"

#define UA_TIZEN_WEB \
		"Mozilla/5.0 (Linux; Tizen 2.1; SAMSUNG GT-I8800) AppleWebKit/537.3 (KHTML, like Gecko) Version/2.1 Mobile Safari/537.3"

#define DEFAULT_LAT		37.253665
#define DEFAULT_LON		127.054968

bool __is_revgeocode_supported = false;
bool __is_place_search_supported = false;
bool __is_routing_supported = false;

Evas_Object *m_map_obj_layout = NULL;
Evas_Object *m_map_evas_object = NULL;
Evas_Object *m_searchbar_obj = NULL;
Evas_Object *m_map_view_layout = NULL;
Evas_Object *m_parent_evas_obj = NULL;

MAP_VIEW_MODE __view_type = MAPS_VIEW_MODE_MY_LOCATION;
bool __is_long_pressed = false;

double __poi_center_lat = 0.0;
double __poi_center_lon = 0.0;

/* Maps Sevie Handle */
maps_service_h __maps_service_handler = NULL;

/* Reverse Geocode Result */
revgeocode_s *__map_revgeocode_result = NULL;
bool __is_revgeocode_result_obtained = false;

/* Place Result*/
place_s *__map_place_result[100];
int __map_place_result_count = 0;
int __map_selected_place_index = -1;

Elm_Map_Overlay *__m_poi_overlay[50];
int __m_poi_overlay_count;
Elm_Map_Overlay *__m_poi_current_overlay = NULL;
const int __overlay_displayed_zoom_min = 5;

/* Route */
route_s *__map_route_result = NULL;
Elm_Map_Overlay *__m_maneuver_overlay[1000];
int __m_maneuver_overlay_count;
Elm_Map_Overlay *__m_route_overlay[1000];
int __m_route_overlay_count;
Elm_Map_Overlay *__m_start_overlay = NULL;
Elm_Map_Overlay *__m_dest_overlay = NULL;

static void MapLocationView(Evas_Object *view_layout);
static void MapPoiListView();
static void __show_current_selected_poi_overlay(int index);
static void MapDirectionView();
static void __remove_map_revgeocode_overlay();

void set_view_type(MAP_VIEW_MODE type);
void hide_map_poi_overlays(Eina_Bool enable);
void remove_map_poi_overlays();
void hide_map_maneuver_overlays(Eina_Bool enable);
void remove_map_maneuver_overlays();

maps_service_h
get_maps_service_handle()
{
	return __maps_service_handler;
}

Eina_Bool
__map_view_delete_request_cb(void *data, Elm_Object_Item *item)
{
	Eina_Bool bRet = EINA_TRUE;
	switch (__view_type) {
	case MAPS_VIEW_MODE_POI_INFO:
	case MAPS_VIEW_MODE_DIRECTION:
	{
		set_view_type(MAPS_VIEW_MODE_MY_LOCATION);

		Evas_Object *map_view_genlist = elm_object_part_content_get(m_map_view_layout, "map_view_genlist");
		if (map_view_genlist)
			evas_object_del(map_view_genlist);

		MapLocationView(m_map_view_layout);
		remove_map_poi_overlays();
		remove_map_maneuver_overlays();
		bRet = EINA_FALSE;
	}
		break;
	case MAPS_VIEW_MODE_MY_LOCATION:
		break;
	default:
		break;
	}

	if (bRet == EINA_TRUE)
		ui_app_exit();

	return bRet;
}

void
init()
{
	__view_type = MAPS_VIEW_MODE_MY_LOCATION;

	int i = 0;
	for (i = 0; i < 50; i++)
		__m_poi_overlay[i] = NULL;

	__m_poi_overlay_count = 0;

	__m_poi_current_overlay = NULL;

	for (i = 0; i < 1000; i++) {
		__m_maneuver_overlay[i] = NULL;
		__m_route_overlay[i] = NULL;
	}
	__m_maneuver_overlay_count = 0;
	__m_route_overlay_count = 0;

	m_map_obj_layout = NULL;
	m_map_evas_object = NULL;
	m_searchbar_obj = NULL;

	__m_start_overlay = NULL;
	__m_dest_overlay = NULL;

	__is_long_pressed = false;
}

void
set_view_type(MAP_VIEW_MODE type)
{
	__view_type = type;
}

void
map_get_poi_lat_lng(double *latitude, double *longitude)
{
	*latitude = __poi_center_lat;
	*longitude = __poi_center_lon;
}

static int
__get_map_center_location(double *latitude, double *longitude)
{
	if (latitude == NULL || longitude == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid argument");
		return -1;
	}

	if (m_map_evas_object == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "map_object is NULL");
		return -1;
	}

	double lat = 0.0;
	double lon = 0.0;

	elm_map_region_get(m_map_evas_object, &lon, &lat);

	if (lat > 90.0 || lat < -90.0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid latitude %f", lat);
		return -1;
	}

	if (lon > 180.0 || lon < -180.0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Invalid longitude %f", lon);
		return -1;
	}
	dlog_print(DLOG_DEBUG, LOG_TAG, "Map Center Pos [%f, %f]", lat, lon);

	*latitude = lat;
	*longitude = lon;

	return 0;
}

static char*
__dropped_pin_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	if (__map_revgeocode_result == NULL) {
		if (!strcmp(part, "elm.text")) {
			if (__is_revgeocode_result_obtained)
				return strdup("RevGeocode location Not Found");
			if (!__is_revgeocode_supported)
				return strdup("Reverse Geocode not supported");
			return strdup("Processing");
		}
		return NULL;
	}

	if (!strcmp(part, "elm.text"))
		return strdup(__map_revgeocode_result->name);
	else if (!strcmp(part, "elm.text.sub")) {
		char *bottom = NULL;
		bottom = strdup(__map_revgeocode_result->address);
		return bottom;
	}

	return NULL;
}

static void
__droppin_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	if (it)
		elm_genlist_item_selected_set(it, EINA_FALSE);
}

static void
__create_dropped_pin_layout()
{
	char edj_path[PATH_MAX] = {0, };

	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	Evas_Object *base = elm_layout_add(m_map_view_layout);
	evas_object_size_hint_weight_set(base, 0, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(base, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_layout_file_set(base, edj_path, "map_poi_items_layout");
	evas_object_show(base);

	Evas_Object *current_location_layout = elm_genlist_add(m_map_view_layout);
	elm_list_mode_set(current_location_layout, ELM_LIST_COMPRESS);
	evas_object_size_hint_weight_set(current_location_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(current_location_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(current_location_layout);

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	itc->func.text_get = __dropped_pin_text_get_cb;
	itc->item_style = "type1";
	elm_genlist_item_append(current_location_layout, itc, (void *)NULL, NULL, ELM_GENLIST_ITEM_NONE, __droppin_clicked_cb, NULL);

	elm_genlist_item_class_free(itc);
	elm_object_part_content_set(base, "scroller", current_location_layout);
	elm_object_part_content_set(m_map_view_layout, "map_view_genlist", base);
}

static Evas_Object*
__remove_dropped_pin_layout(Evas_Object *parent)
{
	Evas_Object *base = elm_layout_add(parent);
	elm_object_part_content_set(parent, "map_view_genlist", base);
	return base;
}

void
handle_addr_notification(revgeocode_s *result)
{
	if (result) {
		if (__map_revgeocode_result)
			free(__map_revgeocode_result);
		__map_revgeocode_result = NULL;

		__map_revgeocode_result = (revgeocode_s *)malloc(sizeof(revgeocode_s));
		memcpy(__map_revgeocode_result, result, sizeof(revgeocode_s));

		dlog_print(DLOG_ERROR, LOG_TAG, "Name : %s, Address : %s", __map_revgeocode_result->name, __map_revgeocode_result->address);
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "Reverse Geocode Result is NULL");
	}

	__is_revgeocode_result_obtained = true;
	__create_dropped_pin_layout();
}

void
handle_poi_notification(place_s **place_res, int res_cnt)
{
	__map_place_result_count = res_cnt;

	if (res_cnt > 0) {
		remove_map_poi_overlays();
		int i = 0;
		for (i = 0; i < res_cnt; i++)
			__map_place_result[i] = place_res[i];

		MapPoiListView();
	}
}

void
handle_route_notification(route_s *result)
{
	__map_route_result = result;

	MapDirectionView();
}

static void
__remove_revgeocode_pin_overlay()
{
	if (__m_start_overlay) {
		elm_map_overlay_del(__m_start_overlay);
		__m_start_overlay = NULL;
	}
}

static void
__maps_scroll_cb(void *data, Evas_Object *obj, void *event_info)
{
	__is_long_pressed = false;
}

static void
__maps_longpress_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (__view_type != MAPS_VIEW_MODE_MY_LOCATION)
		return;
	__is_long_pressed = true;
}

static void
__maps_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (__view_type != MAPS_VIEW_MODE_MY_LOCATION)
		return;

	__remove_dropped_pin_layout(m_map_view_layout);
	cancel_revgeocode_request(__maps_service_handler);
	__remove_revgeocode_pin_overlay();

	if (__is_long_pressed == true) {
		__is_long_pressed = false;

		double lon = 0.0;
		double lat = 0.0;
		Evas_Coord ox, oy, x, y, w, h;
		int zoom;
		Evas_Event_Mouse_Down *down = (Evas_Event_Mouse_Down *)event_info;
		if (!down || !m_map_evas_object)
			return ;

		evas_object_geometry_get(m_map_evas_object, &ox, &oy, &w, &h);
		zoom = elm_map_zoom_get(m_map_evas_object);
		elm_map_region_get(m_map_evas_object, &lon, &lat);
		elm_map_region_to_canvas_convert(m_map_evas_object, lon, lat, &x, &y);
		elm_map_canvas_to_region_convert(m_map_evas_object, down->canvas.x, down->canvas.y, &lon, &lat);
		dlog_print(DLOG_ERROR, LOG_TAG, "converted geocode (lat,lon) = %.5f,%.5f", lat, lon);

		Elm_Map_Overlay *marker_overlay = elm_map_overlay_add(m_map_evas_object, lon, lat);
		Evas_Object *icon = elm_image_add(m_map_evas_object);
		char edj_path[PATH_MAX] = {0, };
		app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);
		elm_image_file_set(icon, edj_path, "start_location");
		evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		elm_map_overlay_content_set(marker_overlay, icon);

		__m_start_overlay = marker_overlay;

		if (__map_revgeocode_result) {
			free(__map_revgeocode_result);
			__map_revgeocode_result = NULL;
		}

		__is_revgeocode_result_obtained = false;

		if (__is_revgeocode_supported) {
			int error = request_revgeocode(__maps_service_handler, lat, lon);

			if (error == MAPS_ERROR_NONE)
				__create_dropped_pin_layout();
		} else
			__create_dropped_pin_layout();
	}
}

static Evas_Object*
__create_map_object(Evas_Object *layout)
{
	char edj_path[PATH_MAX] = {0, };

	if (m_map_evas_object != NULL) {
		/* WERROR("m_map_evas_object is created already"); */
		return m_map_evas_object;
	}

	m_map_evas_object = elm_map_add(layout);
	if (m_map_evas_object == NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "m_map_evas_object is NULL");
		return NULL;
	}

	elm_map_zoom_min_set(m_map_evas_object, 2);
	elm_map_user_agent_set(m_map_evas_object, UA_TIZEN_WEB);
	evas_object_show(m_map_evas_object);

	Evas_Object *map_obj_layout = elm_layout_add(layout);

	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);
	elm_layout_file_set(map_obj_layout, edj_path, "map_object");
	evas_object_size_hint_weight_set(map_obj_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(map_obj_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(map_obj_layout);
	elm_layout_content_set(map_obj_layout, "content", m_map_evas_object);

	evas_object_smart_callback_add(m_map_evas_object, "scroll", __maps_scroll_cb, (void *)NULL);
	evas_object_smart_callback_add(m_map_evas_object, "longpressed", __maps_longpress_cb, (void *)NULL);
	evas_object_smart_callback_add(m_map_evas_object, "clicked", __maps_clicked_cb, (void *)NULL);

	elm_map_zoom_set(m_map_evas_object, 15);
	elm_map_region_bring_in(m_map_evas_object, DEFAULT_LON, DEFAULT_LAT);

	return map_obj_layout;
}

static void
_searchbar_focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	double _lat = 0.0;
	double _lon = 0.0;

	elm_object_focus_set(m_map_evas_object, EINA_TRUE);

	int ret = __get_map_center_location(&_lat, &_lon);
	if (ret != 0) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to get Center location");
		return;
	}

	__poi_center_lat = _lat;
	__poi_center_lon = _lon;

	__remove_map_revgeocode_overlay();

	hide_map_poi_overlays(EINA_TRUE);

	/* Create POI Search View */
	create_search_view(m_parent_evas_obj);
}

static void
__get_direction_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	hide_map_poi_overlays(EINA_TRUE);

	if (__map_route_result) {
		free(__map_route_result);
		__map_route_result = NULL;
	}

	create_route_view(m_parent_evas_obj);
}

static Evas_Object*
__searchbar_gl_content_cb(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.icon")) {
		if (__view_type != MAPS_VIEW_MODE_POI_INFO)
			return NULL;

		char edj_path[PATH_MAX] = {0, };
		app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

		Evas_Object *layout = elm_layout_add(obj);
		elm_layout_theme_set(layout, "layout", "list/C/type.1", "default");

		Evas_Object *get_direction_button = elm_button_add(layout);
		elm_object_style_set(get_direction_button, "custom");
		evas_object_repeat_events_set(get_direction_button, EINA_FALSE);
		evas_object_propagate_events_set(get_direction_button, EINA_FALSE);
		evas_object_show(get_direction_button);

		Evas_Object *img = elm_image_add(get_direction_button);
		elm_image_file_set(img, edj_path, "direction_arrow");
		elm_image_resizable_set(img, EINA_FALSE, EINA_FALSE);
		evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);

		evas_object_show(img);
		elm_object_part_content_set(get_direction_button, "elm.swallow.content", img);
		evas_object_smart_callback_add(get_direction_button, "clicked", __get_direction_btn_clicked_cb, NULL);

		elm_layout_content_set(layout, "elm.swallow.content", get_direction_button);

		return layout;
	} else if (!strcmp(part, "elm.icon.entry")) {
		Evas_Object *editfield = create_editfield(obj);
		eext_entry_selection_back_event_allow_set(editfield, EINA_TRUE);
		evas_object_size_hint_weight_set(editfield, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(editfield, EVAS_HINT_FILL, EVAS_HINT_FILL);

		Evas_Object *entry = elm_object_part_content_get(editfield, "elm.swallow.content");
		elm_object_part_text_set(entry, "elm.guide", "Search");
		evas_object_smart_callback_add(entry, "clicked", _searchbar_focused_cb, entry);
		evas_object_show(entry);

		return entry;
	} else
		return NULL;
}

static void
__searchbar_gl_realized_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (obj == NULL)
		return;
	Elm_Object_Item *item = elm_genlist_first_item_get(obj);
	elm_object_item_signal_emit(item, "elm,state,bottomline,show", "");
}

static Evas_Object*
__create_map_searchbar(Evas_Object *parent)
{
	Evas_Object *searchbar_obj = elm_genlist_add(parent);
	if (searchbar_obj == NULL)
		dlog_print(DLOG_ERROR, LOG_TAG, "searchbar is NULL");
	evas_object_size_hint_weight_set(searchbar_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(searchbar_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_genlist_mode_set(searchbar_obj, ELM_LIST_COMPRESS);
	evas_object_show(searchbar_obj);
	evas_object_smart_callback_add(searchbar_obj, "realized", __searchbar_gl_realized_cb, NULL);

	Elm_Genlist_Item_Class *search_itc = elm_genlist_item_class_new();
	search_itc->item_style = "entry.icon";
	search_itc->func.text_get = NULL;
	search_itc->func.content_get = __searchbar_gl_content_cb;
	search_itc->func.del = NULL;

	elm_genlist_item_append(searchbar_obj, search_itc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_class_free(search_itc);
	elm_object_focus_custom_chain_append(parent, searchbar_obj, NULL);
	return searchbar_obj;
}

static void
MapLocationView(Evas_Object *view_layout)
{
	char edj_path[PATH_MAX] = {0, };

	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	dlog_print(DLOG_ERROR, LOG_TAG, "edj_path : %s", edj_path);

	elm_layout_file_set(view_layout, edj_path, "map-view");
	evas_object_size_hint_weight_set(view_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(view_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(view_layout);

	if (!m_map_obj_layout) {
		m_map_obj_layout = __create_map_object(view_layout);
		if (m_map_obj_layout == NULL) {
			dlog_print(DLOG_ERROR, LOG_TAG, "failed to create map object");
			return ;
		}
	}

	elm_object_part_content_set(view_layout, "map", m_map_obj_layout);

	if (!m_searchbar_obj)
		m_searchbar_obj = __create_map_searchbar(view_layout);

	if (m_searchbar_obj) {
		elm_genlist_realized_items_update(m_searchbar_obj);
		elm_object_part_content_set(view_layout, "searchbar", m_searchbar_obj);
	}

	elm_object_focus_set(m_map_obj_layout, EINA_TRUE);

	elm_object_focus_custom_chain_append(view_layout, m_map_obj_layout, NULL);
	elm_object_focus_custom_chain_append(view_layout, m_searchbar_obj, NULL);
}

Evas_Object*
create_map_view(Evas_Object *parent)
{
	init();

	Evas_Object *view_layout = elm_layout_add(parent);

	MapLocationView(view_layout);

	Elm_Object_Item *navi_item = elm_naviframe_item_push(parent, "Maps Service Sample", NULL, NULL, view_layout, NULL);
	elm_naviframe_item_pop_cb_set(navi_item, __map_view_delete_request_cb, (void *)NULL);

	m_parent_evas_obj = parent;
	m_map_view_layout = view_layout;

	return view_layout;
}

void
create_maps_service_handle()
{
	int error = maps_service_create(MAPS_PROVIDER, &__maps_service_handler);
	bool supported = false;

	/* Set the provider Key to access the services from provider */
	maps_service_set_provider_key(__maps_service_handler, PROVIDER_TEST_KEY);

	/* Check if Routing is available */
	error = maps_service_provider_is_service_supported(__maps_service_handler,
			MAPS_SERVICE_SEARCH_ROUTE, &supported);
	__is_routing_supported = (error == MAPS_ERROR_NONE) ? supported : false;

	error = maps_service_provider_is_service_supported(__maps_service_handler,
			MAPS_SERVICE_SEARCH_PLACE, &supported);
	__is_place_search_supported = (error == MAPS_ERROR_NONE) ? supported : false;

	error = maps_service_provider_is_service_supported(__maps_service_handler,
				MAPS_SERVICE_REVERSE_GEOCODE, &supported);
	__is_revgeocode_supported = (error == MAPS_ERROR_NONE) ? supported : false;
}

void
destroy_maps_service_handle()
{
	if (__maps_service_handler) {
		int error = maps_service_destroy(__maps_service_handler);
		if (error != MAPS_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to destroy maps service [%d]", error);

		__maps_service_handler = NULL;
	}
}

/******************** Place ***********************/

void
map_get_selected_poi_lat_lng(double *latitude, double *longitude, char **name)
{
	*latitude = __map_place_result[__map_selected_place_index]->__lat;
	*longitude = __map_place_result[__map_selected_place_index]->__lon;
	*name = strdup(__map_place_result[__map_selected_place_index]->__place_name);
}

static char*
__poi_layout_text_get_cb(void *data, Evas_Object *obj, const char* part)
{
	place_s *res = (place_s *) data;
	if (!strcmp(part, "elm.text"))
		return strdup(res->__place_name);
	else if (!strcmp(part, "elm.text.sub")) {
		char distance_text[1024] = {0,};
		if (res->__distance > 0.0)
			snprintf(distance_text, sizeof(distance_text), "%0.2f km", res->__distance);

		return strdup(distance_text);
	}

	return NULL;
}

static void
__poi_text_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	if (it)
		elm_genlist_item_selected_set(it, EINA_FALSE);
}

static void
__poi_index_scroller_scroll_cb(void *data, Evas_Object *scroller, void *event_info)
{
	int page_no;
	elm_scroller_current_page_get(scroller, &page_no, NULL);
	dlog_print(DLOG_DEBUG, LOG_TAG, "Scrolld page no :: %d", page_no);
	if (page_no != __map_selected_place_index) {
		__map_selected_place_index = page_no;
		__show_current_selected_poi_overlay(page_no);
	}
}

static Evas_Object *
__create_map_view_poi_genlist(Evas_Object *layout)
{
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);
	dlog_print(DLOG_DEBUG, LOG_TAG, "edj_path : %s", edj_path);

	Evas_Object *poi_items_layout = elm_layout_add(layout);
	elm_layout_file_set(poi_items_layout, edj_path, "map_poi_items_layout");
	evas_object_size_hint_weight_set(poi_items_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(poi_items_layout);

	Evas_Object *scroller = elm_scroller_add(poi_items_layout);
	elm_scroller_loop_set(scroller, EINA_FALSE, EINA_FALSE);
	evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_scroller_page_relative_set(scroller, 1.0, 0.0);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_scroller_page_scroll_limit_set(scroller, 1, 0);
	elm_scroller_bounce_set(scroller, EINA_TRUE, EINA_FALSE);
	elm_object_scroll_lock_y_set(scroller, EINA_TRUE);
	elm_object_part_content_set(poi_items_layout, "scroller", scroller);

	evas_object_smart_callback_add(scroller, "scroll", __poi_index_scroller_scroll_cb, (void *) NULL);

	Evas_Object *box = elm_box_add(scroller);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_TRUE);
	elm_object_content_set(scroller, box);
	evas_object_show(box);

	int i = 0;
	for (i = 0; i < __map_place_result_count; i++) {
		Evas_Object *page_layout = elm_layout_add(box);
		elm_layout_file_set(page_layout, edj_path, "pagecontrol_page_layout");
		evas_object_size_hint_weight_set(page_layout, 0, 0);
		evas_object_size_hint_align_set(page_layout, 0, EVAS_HINT_FILL);
		evas_object_show(page_layout);

		Evas_Object *poi_genlist = elm_genlist_add(page_layout);
		elm_list_mode_set(poi_genlist, ELM_LIST_COMPRESS);
		evas_object_size_hint_align_set(poi_genlist, 0, 0);
		evas_object_size_hint_weight_set(poi_genlist, 0, EVAS_HINT_FILL);
		evas_object_show(poi_genlist);

		Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
		itc->item_style = "type1";
		itc->func.text_get = __poi_layout_text_get_cb;

		elm_genlist_item_append(poi_genlist, itc, (void *)__map_place_result[i], NULL, ELM_GENLIST_ITEM_NONE, __poi_text_clicked_cb, (void *)i);
		elm_genlist_item_class_free(itc);
		elm_object_part_content_set(page_layout, "page", poi_genlist);

		/* m_plm->addLayout(i, page_layout); */
		elm_box_pack_end(box, page_layout);
	}

	__map_selected_place_index = 0;

	return poi_items_layout;
}

static void
__show_poi_marker(double lat, double lon, int index)
{
	if (m_map_evas_object == NULL)
		return;

	char edj_path[PATH_MAX] = {0, };
	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);
	dlog_print(DLOG_DEBUG, LOG_TAG, "edj_path : %s", edj_path);

	Elm_Map_Overlay *overlay = elm_map_overlay_add(m_map_evas_object, lon, lat);
	elm_map_overlay_displayed_zoom_min_set(overlay, __overlay_displayed_zoom_min);
	elm_map_overlay_data_set(overlay, (void *)index);

	Evas_Object *icon = elm_layout_add(m_map_evas_object);
	elm_layout_file_set(icon, edj_path, "poi-marker-image");
	evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	/* elm_object_signal_callback_add(icon, "marker,clicked", "", _poi_marker_clicked_cb, (void *)index); */
	evas_object_show(icon);
	elm_map_overlay_content_set(overlay, icon);

	__m_poi_overlay[index] = overlay;
}

static void
__show_current_poi_marker(double lat, double lon, int index)
{
	if (m_map_evas_object == NULL)
		return;

	char edj_path[PATH_MAX] = {0, };
	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);
	dlog_print(DLOG_DEBUG, LOG_TAG, "edj_path : %s", edj_path);

	Elm_Map_Overlay *overlay = elm_map_overlay_add(m_map_evas_object, lon, lat);
	elm_map_overlay_displayed_zoom_min_set(overlay, __overlay_displayed_zoom_min);
	elm_map_overlay_data_set(overlay, (void *)index);

	Evas_Object *icon = elm_layout_add(m_map_evas_object);
	elm_layout_file_set(icon, edj_path, "current-poi-marker-image");
	evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	/* elm_object_signal_callback_add(icon, "marker,clicked", "", _poi_marker_clicked_cb, (void *)index); */
	evas_object_show(icon);

	elm_map_overlay_content_set(overlay, icon);

	if (__m_poi_current_overlay) {
		elm_map_overlay_del(__m_poi_current_overlay);
		__m_poi_current_overlay = NULL;
	}

	__m_poi_current_overlay = overlay;
}

static void
__show_current_selected_poi_overlay(int index)
{
	Elm_Map_Overlay *overlay = NULL;

	double lat = (double)__map_place_result[index]->__lat;
	double lon = (double)__map_place_result[index]->__lon;
	__show_current_poi_marker(lat, lon, index);

	overlay = __m_poi_overlay[__m_poi_overlay_count - 1];

	const Evas_Object *marker = elm_map_overlay_content_get(overlay);
	edje_object_signal_emit((Evas_Object *)elm_layout_edje_get(marker), "set_selected", "marker-event");

	elm_map_overlay_show(overlay);
}

static void
MapPoiListView()
{
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	dlog_print(DLOG_DEBUG, LOG_TAG, "edj_path : %s", edj_path);

	elm_layout_file_set(m_map_view_layout, edj_path, "map-view-poi-search");
	evas_object_size_hint_weight_set(m_map_view_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(m_map_view_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(m_map_view_layout);

	if (!m_map_obj_layout) {
		m_map_obj_layout = __create_map_object(m_map_view_layout);
		if (NULL == m_map_obj_layout) {
			dlog_print(DLOG_ERROR, LOG_TAG, "__create_map_object is failed");
			return ;
		}
	}

	if (__map_place_result_count > 0) {
		__m_poi_overlay_count = __map_place_result_count;
		__map_selected_place_index = 0;

		for (int i = 0; i < __map_place_result_count; i++) {
			double lat = __map_place_result[i]->__lat;
			double lon = __map_place_result[i]->__lon;

			__show_poi_marker(lat, lon, i);
		}

		__show_current_selected_poi_overlay(__map_selected_place_index);

		elm_map_region_show(m_map_evas_object, __map_place_result[__map_selected_place_index]->__lon,
									__map_place_result[__map_selected_place_index]->__lat);

		elm_map_zoom_set(m_map_evas_object, 12);
	}

	elm_object_part_content_set(m_map_view_layout, "map", m_map_obj_layout);

	if (!m_searchbar_obj)
		m_searchbar_obj = __create_map_searchbar(m_map_view_layout);

	/* update entry text */
	Elm_Object_Item *item = elm_genlist_first_item_get(m_searchbar_obj);
	if (item != NULL)
		elm_genlist_item_update(item);
	elm_object_part_content_set(m_map_view_layout, "searchbar", m_searchbar_obj);

	elm_object_focus_set(m_map_obj_layout, EINA_TRUE);
	elm_object_focus_custom_chain_append(m_map_view_layout, m_map_obj_layout, NULL);
	elm_object_focus_custom_chain_append(m_map_view_layout, m_searchbar_obj, NULL);

	Evas_Object *current_poi_obj = __create_map_view_poi_genlist(m_map_view_layout);
	elm_object_part_content_set(m_map_view_layout, "map_view_genlist", current_poi_obj);
}

/******************* Route ********************/

void
hide_map_maneuver_overlays(Eina_Bool enable)
{
	int i = 0;
	for (i = 0; i < __m_maneuver_overlay_count; i++) {
		if (__m_maneuver_overlay[i])
			elm_map_overlay_hide_set(__m_maneuver_overlay[i], enable);
	}

	for (i = 0; i < __m_route_overlay_count; i++) {
		if (__m_route_overlay[i])
			elm_map_overlay_hide_set(__m_route_overlay[i], enable);
	}
}

void
remove_map_maneuver_overlays()
{
	int i = 0;
	for (i = 0; i < __m_maneuver_overlay_count; i++) {
		if (__m_maneuver_overlay[i])
			elm_map_overlay_del(__m_maneuver_overlay[i]);
		__m_maneuver_overlay[i] = NULL;
	}

	for (i = 0; i < __m_route_overlay_count; i++) {
		if (__m_route_overlay[i])
			elm_map_overlay_del(__m_route_overlay[i]);
		__m_route_overlay[i] = NULL;
	}

	__m_maneuver_overlay_count = 0;
	__m_route_overlay_count = 0;

	if (__m_start_overlay) {
		elm_map_overlay_del(__m_start_overlay);
		__m_start_overlay = NULL;
	}

	if (__m_dest_overlay) {
		elm_map_overlay_del(__m_dest_overlay);
		__m_dest_overlay = NULL;
	}
}


void
hide_map_poi_overlays(Eina_Bool enable)
{
	int i = 0;
	for (i = 0; i < __m_poi_overlay_count; i++) {
		if (__m_poi_overlay[i])
			elm_map_overlay_hide_set(__m_poi_overlay[i], enable);
	}
	/* poi current overlay */
	if (__m_poi_current_overlay)
		elm_map_overlay_hide_set(__m_poi_current_overlay, enable);
}

static void
__remove_map_revgeocode_overlay()
{
	if (__m_start_overlay) {
		elm_map_overlay_del(__m_start_overlay);
		__m_start_overlay = NULL;
	}
}

void
remove_map_poi_overlays()
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "Removing POI overlays");
	int i = 0;
	for (i = 0; i < __m_poi_overlay_count; i++) {
		if (__m_poi_overlay[i]) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "deleting overlay");
			elm_map_overlay_del(__m_poi_overlay[i]);
		}
		__m_poi_overlay[i] = NULL;
	}

	__m_poi_overlay_count = 0;

	/* poi current overlay */
	if (__m_poi_current_overlay) {
		elm_map_overlay_del(__m_poi_current_overlay);
		__m_poi_current_overlay = NULL;
	}
}

static void
__route_cb(void *data, Evas_Object *obj, void *ev)
{
	Elm_Map_Route *route = (Elm_Map_Route *)data;

	Elm_Map_Overlay *route_ovl = elm_map_overlay_route_add(obj, route);
	elm_map_overlay_color_set(route_ovl, 17, 17, 204, 204);

	__m_route_overlay[__m_route_overlay_count] = route_ovl;
	__m_route_overlay_count += 1;
}

static void
__show_route_maneuvers()
{
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	double fLat = 0.0, fLon = 0.0, tLat = 0.0, tLon = 0.0;
	int maneuver_count = 0;
	maneuver_count = __map_route_result->__maneuver_count;

	dlog_print(DLOG_DEBUG, LOG_TAG, "Maneuver count :: %d", maneuver_count);

	__m_maneuver_overlay_count = 0;
	__m_route_overlay_count = 0;

	int i = 0;
	for (i = 0; i < maneuver_count; i++) {
		fLat = (__map_route_result->__maneuver[i]).__origin_lat;
		fLon = (__map_route_result->__maneuver[i]).__origin_lon;

		tLat = (__map_route_result->__maneuver[i]).__dest_lat;
		tLon = (__map_route_result->__maneuver[i]).__dest_lon;

		Elm_Map_Overlay *instruction_overlay = elm_map_overlay_add(m_map_evas_object, fLon, fLat);

		Evas_Object *icon = elm_layout_add(m_map_evas_object);
		elm_layout_file_set(icon, edj_path, "icon/instruction_point");
		evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		evas_object_show(icon);
		elm_map_overlay_content_set(instruction_overlay, icon);

		__m_maneuver_overlay[i] = instruction_overlay;
		__m_maneuver_overlay_count += 1;

		Elm_Map_Route *map_route = elm_map_route_add(m_map_evas_object,
										ELM_MAP_ROUTE_TYPE_MOTOCAR,
										ELM_MAP_ROUTE_METHOD_FASTEST,
										fLon, fLat, tLon, tLat,
										NULL,
										NULL);

		evas_object_smart_callback_add(m_map_evas_object, "route,loaded", __route_cb, map_route);
	}
}

static void
__show_route_info_in_map()
{
	__show_route_maneuvers();

	char edj_path[PATH_MAX] = {0, };
	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	int maneuver_count = 0;
	maneuver_count = __map_route_result->__maneuver_count;

	double lat = 0.0;
	double lon = 0.0;

	lat = __map_route_result->__maneuver[0].__origin_lat;
	lon = __map_route_result->__maneuver[0].__origin_lon;

	dlog_print(DLOG_DEBUG, LOG_TAG, "Origin - [%f,%f]", lat, lon);

	Elm_Map_Overlay *start_location_overlay = elm_map_overlay_add(m_map_evas_object, lon, lat);
	Evas_Object *start_icon = elm_image_add(m_map_evas_object);
	elm_image_file_set(start_icon, edj_path, "start_location");
	evas_object_size_hint_align_set(start_icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(start_icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_map_overlay_content_set(start_location_overlay, start_icon);
	elm_map_overlay_show(start_location_overlay);

	__m_start_overlay = start_location_overlay;

	lat = __map_route_result->__maneuver[__map_route_result->__maneuver_count - 1].__dest_lat;
	lon = __map_route_result->__maneuver[__map_route_result->__maneuver_count - 1].__dest_lon;

	dlog_print(DLOG_DEBUG, LOG_TAG, "Destination - [%f,%f]", lat, lon);

	Elm_Map_Overlay *dest_location_overlay = elm_map_overlay_add(m_map_evas_object, lon, lat);
	Evas_Object *dest_icon = elm_image_add(m_map_evas_object);
	elm_image_file_set(dest_icon, edj_path, "dest_location");
	evas_object_size_hint_align_set(dest_icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(dest_icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_map_overlay_content_set(dest_location_overlay, dest_icon);
	elm_map_overlay_show(dest_location_overlay);

	__m_dest_overlay = dest_location_overlay;
}


static char*
_route_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup("Via");
	} else if (!strcmp(part, "elm.text.sub")) {
		/* distance (time) */
		char dist[1024] = {0, };
		snprintf(dist, 1023, "%0.1f km (%d min)", __map_route_result->__distance, __map_route_result->__duration);
		return strdup(dist);
	}

	return NULL;
}

static Evas_Object*
__route_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	return NULL;
}

static void
__route_gl_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	if (it)
		elm_genlist_item_selected_set(it, EINA_FALSE);
}

static Evas_Object*
__create_routeinfo_genlist(Evas_Object *parent)
{
	Evas_Object *genlist = elm_genlist_add(parent);
	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(genlist);

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	itc->item_style = "type1";
	itc->func.text_get = _route_gl_text_get;
	itc->func.content_get = __route_gl_content_get;
	itc->func.del = NULL;

	elm_genlist_item_append(genlist,
			itc,
			(void *)NULL,
			NULL,
			ELM_GENLIST_ITEM_NONE,
			__route_gl_select_cb,
			(void *)NULL);

	elm_genlist_item_class_free(itc);

	return genlist;
}

static Evas_Object*
__create_route_genlist(Evas_Object *parent)
{
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	Evas_Object *route_info_layout = elm_layout_add(parent);
	elm_layout_file_set(route_info_layout, edj_path, "map_poi_items_layout");
	evas_object_size_hint_weight_set(route_info_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(route_info_layout);

	Evas_Object *scroller = elm_scroller_add(route_info_layout);
	elm_scroller_loop_set(scroller, EINA_FALSE, EINA_FALSE);
	evas_object_size_hint_weight_set(scroller, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(scroller, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_scroller_page_relative_set(scroller, 1.0, 0.0);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_scroller_page_scroll_limit_set(scroller, 1, 0);
	elm_scroller_bounce_set(scroller, EINA_TRUE, EINA_FALSE);
	elm_object_scroll_lock_y_set(scroller, EINA_TRUE);
	elm_object_part_content_set(route_info_layout, "scroller", scroller);

	Evas_Object *box = elm_box_add(scroller);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_TRUE);
	elm_object_content_set(scroller, box);
	evas_object_show(box);

	Evas_Object *page_content = __create_routeinfo_genlist(box);
	elm_box_pack_end(box, page_content);

	return route_info_layout;
}

static void
MapDirectionView()
{
	set_view_type(MAPS_VIEW_MODE_DIRECTION);

	char edj_path[PATH_MAX] = {0, };
	app_get_resource(MAP_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);
	dlog_print(DLOG_DEBUG, LOG_TAG, "edj_path : %s", edj_path);

	elm_layout_file_set(m_map_view_layout, edj_path, "route-view");
	evas_object_size_hint_weight_set(m_map_view_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(m_map_view_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(m_map_view_layout);

	if (!m_map_obj_layout) {
		m_map_obj_layout = __create_map_object(m_map_view_layout);
		if (NULL == m_map_obj_layout) {
			dlog_print(DLOG_ERROR, LOG_TAG, "__create_map_object is failed");
			return ;
		}
	}

	elm_object_part_content_set(m_map_view_layout, "map", m_map_obj_layout);

	/* Remove searchbar */
	if (m_searchbar_obj) {
		/* evas_object_hide(m_searchbar_obj); */
		elm_object_part_content_unset(m_map_view_layout, "searchbar");
		evas_object_del(m_searchbar_obj);
		m_searchbar_obj = NULL;
	}

	__show_route_info_in_map();

	Evas_Object *map_view_genlist = __create_route_genlist(m_map_view_layout);
	elm_object_part_content_set(m_map_view_layout, "map_view_genlist", map_view_genlist);
}
