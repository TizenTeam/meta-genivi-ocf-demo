#include <Ecore.h>
#include <dlog.h>
#include <efl_extension.h>
#include "lbs-maps.h"
#include "search_view.h"
#include "main_view.h"
#include "util.h"
#include "place.h"

#define NUM_OF_CATEGORY	5
#define PLACE_RESULT_SIZE	100

const char *category_list_text[] = {
	"Food and Drink",
	"Transport",
	"Hotels",
	"Shopping",
	"Leisure"
};

Evas_Object *m_search_view_layout = NULL;
Evas_Object *m_search_content_layout = NULL;
Evas_Object *m_search_parent_obj = NULL;

place_s *__place_result[PLACE_RESULT_SIZE];
int __place_result_count = 0;

bool __poi_result_obtained = false;

extern int __is_place_search_supported;

Eina_Bool
__search_view_delete_request_cb(void *data, Elm_Object_Item *item)
{
	cancel_place_request(get_maps_service_handle());

	if (!__poi_result_obtained)
		hide_map_poi_overlays(EINA_FALSE);
	else
		set_view_type(MAPS_VIEW_MODE_POI_INFO);

	return EINA_TRUE;
}

void
on_poi_result(int res_cnt)
{
	dlog_print(DLOG_DEBUG, LOG_TAG, "Place Result count :: [%d]", res_cnt);

	__place_result_count = res_cnt;
	int i = 0;
	for (i = 0; i < res_cnt; i++) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "Inside");
		dlog_print(DLOG_DEBUG, LOG_TAG, "place name : %s", __place_result[i]->__place_name);
		dlog_print(DLOG_DEBUG, LOG_TAG, "place coordinates :: %f,%f", __place_result[i]->__lat, __place_result[i]->__lon);
		dlog_print(DLOG_DEBUG, LOG_TAG, "place coordinates :: %f", __place_result[i]->__distance);
	}

	handle_poi_notification(__place_result, __place_result_count);

	if (res_cnt > 0) {
		__poi_result_obtained = true;
		elm_naviframe_item_pop(m_search_parent_obj);
	} else {
		m_search_content_layout = create_nocontent_layout(m_search_view_layout, "No results found", NULL);
		elm_object_part_content_set(m_search_view_layout, "list", m_search_content_layout);
	}
}

static void
__change_progress_view()
{
	m_search_content_layout = elm_progressbar_add(m_search_view_layout);
	elm_object_style_set(m_search_content_layout, "process_large");
	evas_object_size_hint_align_set(m_search_content_layout, EVAS_HINT_FILL, 0.5);
	evas_object_size_hint_weight_set(m_search_content_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_progressbar_pulse(m_search_content_layout, EINA_TRUE);
	evas_object_show(m_search_content_layout);
	elm_object_focus_set(m_search_content_layout, EINA_TRUE);

	elm_object_part_content_set(m_search_view_layout, "list", m_search_content_layout);
}

static char*
__search_category_gl_text_get_cb(void *data, Evas_Object *obj, const char *part)
{
	int index = (int)data;
	if (!strcmp(part, "elm.text"))
		return strdup(category_list_text[index]);

	return NULL;
}

static void
__search_category_gl_select_cb(void *data, Evas_Object *obj, void *event_info)
{
	int index = (int) data;
	int max_results = 25;
	int radius = -1;

	Elm_Object_Item *it = (Elm_Object_Item *)event_info;
	if (it)
		elm_genlist_item_selected_set(it, EINA_FALSE);

	/* Get Map Center Latitude and Longitude */
	double latitude = 0.0;
	double longitude = 0.0;
	map_get_poi_lat_lng(&latitude, &longitude);

	dlog_print(DLOG_DEBUG, LOG_TAG, "Center Location for POI search :: [%f,%f]", latitude, longitude);

	if (__is_place_search_supported) {
		__change_progress_view();

		/* POI Search */
		int error = request_place(get_maps_service_handle(), latitude, longitude, "", index, radius, max_results, __place_result);

		if (error != MAPS_ERROR_NONE) {
			m_search_content_layout = create_nocontent_layout(m_search_view_layout, "Place Request Failed", NULL);
			elm_object_part_content_set(m_search_view_layout, "list", m_search_content_layout);
		}
	} else {
		m_search_content_layout = create_nocontent_layout(m_search_view_layout, "Place Search not supported", NULL);
		elm_object_part_content_set(m_search_view_layout, "list", m_search_content_layout);
	}
}

static Evas_Object*
__create_search_category_genlist(Evas_Object *parent)
{
	Evas_Object *search_category_genlist = elm_genlist_add(parent);
	elm_list_mode_set(search_category_genlist, ELM_LIST_COMPRESS);
	evas_object_show(search_category_genlist);

	/* category */
	Elm_Genlist_Item_Class *category_itc = elm_genlist_item_class_new();
	category_itc->item_style = "type1";
	category_itc->func.content_get = NULL;	/* __search_category_gl_content_get_cb; */
	category_itc->func.text_get = __search_category_gl_text_get_cb;

	for (int i = 0; i < NUM_OF_CATEGORY; i++) {
		elm_genlist_item_append(search_category_genlist,
				category_itc,
				(void *)i,
				NULL,
				ELM_GENLIST_ITEM_NONE,
				__search_category_gl_select_cb,
				(void *)i);
	}

	elm_genlist_item_class_free(category_itc);
	return search_category_genlist;
}

static Evas_Object*
__create_searchbar(Evas_Object *parent)
{
	Evas_Object *searchbar_obj = elm_layout_add(parent);
	evas_object_size_hint_weight_set(searchbar_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(searchbar_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(searchbar_obj);
	elm_layout_theme_set(searchbar_obj, "layout", "searchbar", "default");

	/* entry */
	Evas_Object *editfield = create_editfield(searchbar_obj);
	eext_entry_selection_back_event_allow_set(editfield, EINA_TRUE);
	evas_object_size_hint_weight_set(editfield, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(editfield, EVAS_HINT_FILL, EVAS_HINT_FILL);
	Evas_Object *entry = elm_object_part_content_get(editfield, "elm.swallow.content");
	evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_cnp_mode_set(entry, ELM_CNP_MODE_PLAINTEXT);
	elm_entry_autocapital_type_set(entry, ELM_AUTOCAPITAL_TYPE_NONE);
	elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_NORMAL);
	elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH);
	eext_entry_selection_back_event_allow_set(entry, EINA_TRUE);
	elm_object_part_text_set(entry, "elm.guide", "Search");
	elm_entry_prediction_allow_set(entry, EINA_FALSE);

	evas_object_show(entry);

	elm_object_part_content_set(searchbar_obj, "elm.swallow.content", entry);

	return searchbar_obj;
}

Evas_Object *
create_search_view(Evas_Object *parent)
{
	__poi_result_obtained = false;

	char edj_path[PATH_MAX] = {0, };

	app_get_resource(SEARCH_VIEW_EDJ_FILE, edj_path, (int)PATH_MAX);

	dlog_print(DLOG_DEBUG, LOG_TAG, "edj_path : %s", edj_path);

	m_search_view_layout = elm_layout_add(parent);
	elm_layout_file_set(m_search_view_layout, edj_path, "result-view");
	evas_object_size_hint_weight_set(m_search_view_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(m_search_view_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	Evas_Object *searchbar_obj = __create_searchbar(m_search_view_layout);
	if (searchbar_obj == NULL)
		dlog_print(DLOG_ERROR, LOG_TAG, "searchbar_obj is NULL");
	elm_object_part_content_set(m_search_view_layout, "searchbar", searchbar_obj);

	Evas_Object *scroller = elm_scroller_add(m_search_view_layout);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);

	m_search_content_layout = __create_search_category_genlist(scroller);
	elm_object_part_content_set(m_search_view_layout, "list", m_search_content_layout);

	Elm_Object_Item *navi_item = elm_naviframe_item_push(parent, "Search", NULL, NULL, m_search_view_layout, NULL);
	elm_naviframe_item_title_enabled_set(navi_item, EINA_FALSE, EINA_FALSE);

	elm_naviframe_item_pop_cb_set(navi_item, __search_view_delete_request_cb, (void *)NULL);

	m_search_parent_obj = parent;

	__poi_result_obtained = false;

	return m_search_view_layout;
}
