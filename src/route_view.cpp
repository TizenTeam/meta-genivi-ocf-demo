#include <Ecore.h>
#include <dlog.h>
#include <efl_extension.h>
#include <dlog.h>
#include "lbs-maps.h"
#include "route_view.h"
#include "main_view.h"
#include "util.h"
#include "route.h"

Evas_Object *m_route_view_layout = NULL;
Evas_Object *m_route_searchbox_layout = NULL;
Evas_Object *m_route_genlist_layout = NULL;
Evas_Object *m_route_parent_obj = NULL;

route_s *__route_result = NULL;

bool __route_result_obtained = false;

extern bool __is_routing_supported;

static void __update_genlist();

Eina_Bool
__route_view_delete_request_cb(void *data, Elm_Object_Item *item)
{
	cancel_route_request(get_maps_service_handle());

	if (!__route_result_obtained)
		hide_map_poi_overlays(EINA_FALSE);

	return EINA_TRUE;
}

void
on_route_result()
{
	__update_genlist();
}

static void
__show_progress()
{
	Evas_Object *parent = m_route_view_layout;
	Evas_Object *progressbar = NULL;
	progressbar = elm_progressbar_add(parent);
	elm_object_style_set(progressbar, "process_large");
	evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, 0.5);
	evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_pulse(progressbar, EINA_TRUE);
	evas_object_show(progressbar);
	elm_object_focus_set(progressbar, EINA_TRUE);
	elm_object_part_content_set(parent, "genlist", progressbar);
}

static void
find_route()
{
	double fLat = 0.0;
	double fLng = 0.0;
	double tLat = 0.0;
	double tLng = 0.0;
	char *name = NULL;

	map_get_poi_lat_lng(&fLat, &fLng);
	map_get_selected_poi_lat_lng(&tLat, &tLng, &name);

	dlog_print(DLOG_DEBUG, LOG_TAG, "From : [%f,%f]", fLat, fLng);
	dlog_print(DLOG_DEBUG, LOG_TAG, "To : [%f,%f]", tLat, tLng);

	if (__is_routing_supported) {
		/* Request Route */
		int error = request_route(get_maps_service_handle(), fLat, fLng, tLat, tLng, &__route_result);

		if (error != MAPS_ERROR_NONE) {
			m_route_genlist_layout = create_nocontent_layout(m_route_view_layout, "Route Request Failed", NULL);
			elm_object_part_content_set(m_route_view_layout, "genlist", m_route_genlist_layout);
		} else {
			__show_progress();
		}
	} else {
		m_route_genlist_layout = create_nocontent_layout(m_route_view_layout, "Route Search not supported", NULL);
		elm_object_part_content_set(m_route_view_layout, "genlist", m_route_genlist_layout);
	}
}

static void
__route_list_gl_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	handle_route_notification(__route_result);

	__route_result_obtained = true;

	elm_naviframe_item_pop(m_route_parent_obj);
}

static char*
_route_list_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.text")) {
		return strdup("Via");
	} else if (!strcmp(part, "elm.text.sub")) {
		char dist[1024] = {0, };
		snprintf(dist, 1023, "%0.1f km (%d min)", __route_result->__distance, __route_result->__duration);
		dlog_print(DLOG_ERROR, LOG_TAG, "sub.left.bottom: str[%s]", dist);

		return strdup(dist);
	}
	return NULL;
}

static void
__gl_content_get_by_type(Evas_Object *parent)
{
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(ROUTE_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	Evas_Object *img = elm_image_add(parent);
	elm_image_file_set(img, ROUTE_VIEW_EDJ_FILE, "transportation_car");
	evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(img);
	elm_layout_content_set(parent, "elm.swallow.content", img);
}

static Evas_Object*
__route_list_gl_content_get(void *data, Evas_Object *obj, const char *part)
{
	if (!strcmp(part, "elm.swallow.icon.1")) {
		Evas_Object *content = elm_layout_add(obj);
		elm_layout_theme_set(content, "layout", "list/B/type.3", "default");
		__gl_content_get_by_type(content);
		evas_object_show(content);

		return content;
	}

	return NULL;
}

static void
__update_genlist()
{
	Evas_Object *base_layout = m_route_view_layout;

	Evas_Object *genlist = NULL;
	genlist = elm_genlist_add(base_layout);

	elm_genlist_homogeneous_set(genlist, EINA_TRUE);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();

	itc->item_style = "type1";
	itc->func.text_get = _route_list_gl_text_get;
	itc->func.content_get = __route_list_gl_content_get;
	itc->func.del = NULL;

	elm_genlist_item_append(genlist, itc, (void *)NULL, NULL, ELM_GENLIST_ITEM_NONE, __route_list_gl_select_cb, (void*)NULL);

	elm_genlist_item_class_free(itc);

	evas_object_show(genlist);
	elm_object_part_content_set(base_layout, "genlist", genlist);
}


static Evas_Object*
__create_searchbar_from(Evas_Object *layout)
{
	Evas_Object *editfield = create_editfield(layout);
	eext_entry_selection_back_event_allow_set(editfield, EINA_TRUE);
	evas_object_size_hint_weight_set(editfield, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(editfield, EVAS_HINT_FILL, EVAS_HINT_FILL);
	Evas_Object *entry = elm_object_part_content_get(editfield, "elm.swallow.content");
	elm_entry_input_panel_enabled_set(entry, EINA_FALSE);
	eext_entry_selection_back_event_allow_set(entry, EINA_TRUE);
	elm_object_focus_allow_set(entry, EINA_FALSE);

	elm_entry_entry_set(entry, "Map Center Location");

	return entry;
}

static Evas_Object*
__create_searchbar_to(Evas_Object *layout)
{
	Evas_Object *editfield = create_editfield(layout);
	eext_entry_selection_back_event_allow_set(editfield, EINA_TRUE);
	evas_object_size_hint_weight_set(editfield, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(editfield, EVAS_HINT_FILL, EVAS_HINT_FILL);
	Evas_Object *entry = elm_object_part_content_get(editfield, "elm.swallow.content");
	elm_entry_input_panel_enabled_set(entry, EINA_FALSE);
	eext_entry_selection_back_event_allow_set(entry, EINA_TRUE);
	elm_entry_input_panel_enabled_set(entry, EINA_FALSE);
	elm_object_focus_allow_set(entry, EINA_FALSE);

	double tLat = 0.0;
	double tLng = 0.0;
	char *name = NULL;

	map_get_selected_poi_lat_lng(&tLat, &tLng, &name);

	elm_entry_entry_set(entry, name);

	return entry;
}

static Evas_Object*
__create_route_searchbox(Evas_Object *parent)
{
	char edj_path[PATH_MAX] = {0, };
	app_get_resource(ROUTE_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	Evas_Object *searchbox_layout = elm_layout_add(parent);
	elm_layout_file_set(searchbox_layout, edj_path, "searchbox-layout");
	evas_object_size_hint_weight_set(searchbox_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(searchbox_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(searchbox_layout);

	/* set icon */
	Evas_Object *icon_layout = elm_layout_add(searchbox_layout);
	elm_layout_file_set(icon_layout, edj_path, "searchbox-icon-layout");
	evas_object_size_hint_weight_set(icon_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(icon_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(icon_layout);

	/* set Searchbox entry */
	Evas_Object *entry_layout = elm_layout_add(searchbox_layout);
	elm_layout_file_set(entry_layout, edj_path, "searchbox-entry-layout");
	evas_object_size_hint_weight_set(entry_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(entry_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(entry_layout);
	elm_object_part_content_set(searchbox_layout, "searchbox-entry", entry_layout);

	/* Set From */
	Evas_Object *searchbar_from_obj = __create_searchbar_from(entry_layout);
	elm_object_part_content_set(entry_layout, "entry-from", searchbar_from_obj);

	/* Set dest */
	Evas_Object *searchbar_to_obj = __create_searchbar_to(entry_layout);
	elm_object_part_content_set(entry_layout, "entry-to", searchbar_to_obj);

	elm_object_part_content_set(parent, "searchbar-box", searchbox_layout);

	return searchbox_layout;
}

void
create_route_view(Evas_Object *parent)
{
	__route_result_obtained = false;
	__route_result = NULL;

	char edj_path[PATH_MAX] = {0, };
	app_get_resource(ROUTE_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	Evas_Object *m_view_layout = elm_layout_add(parent);
	elm_layout_file_set(m_view_layout, edj_path, "route-view");
	evas_object_size_hint_weight_set(m_view_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(m_view_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(m_view_layout);

	m_route_searchbox_layout = __create_route_searchbox(m_view_layout);
	if (m_route_searchbox_layout != NULL)
		elm_object_part_content_set(m_view_layout, "searchbar-box", m_route_searchbox_layout);

	m_route_view_layout = m_view_layout;

	Elm_Object_Item *navi_item = elm_naviframe_item_push(parent, "Search", NULL, NULL, m_route_view_layout, NULL);
	elm_naviframe_item_title_enabled_set(navi_item, EINA_FALSE, EINA_FALSE);

	m_route_parent_obj = parent;

	elm_naviframe_item_pop_cb_set(navi_item, __route_view_delete_request_cb, (void *)NULL);

	find_route();
}
