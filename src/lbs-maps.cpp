#include "config.h"

#include <tizen.h>
#include <locations.h>

#include "lbs-maps.h"
#include "main_view.h"
#include "observer.h"
#include "config.h"

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *layout;
	Evas_Object *conform;
	Evas_Object *nf;
	location_manager_s *manager;
	Ecore_Thread *thread;
} appdata_s;


static void
get_current_location(appdata_s *ad)
{
    double altitude, latitude, longitude, climb, direction, speed;
    double horizontal, vertical;
    location_accuracy_level_e level;
    time_t timestamp;
    
    int ret = location_manager_get_location
      (ad->manager, &altitude, &latitude, &longitude, &climb, &direction, &speed, &level, &horizontal, &vertical, &timestamp);
	dlog_print(DLOG_INFO, LOG_TAG, "get current location success? %s", ret == 0 ? "Yes" : "No");

	Eina_Strbuf *strbuf = eina_strbuf_new();
	eina_strbuf_append_printf(strbuf,
			    "altitude: %f <br/>latitude: %f <br/>longitude: %f <br/>climb: %f<br/>direction: %f <br/>speed: %f <br/>level: %f <br/>horizontal: %f <br/>vertical: %f <br/>timestamp: %lf",
			    altitude, latitude, longitude, climb, direction, speed, level, horizontal, vertical, timestamp);
	char *gps_info = eina_strbuf_string_steal(strbuf);
	//elm_object_text_set(ad->gps_lable, gps_info);

	dlog_print(DLOG_INFO, LOG_TAG, "%s", gps_info);
	
	eina_strbuf_free(strbuf);
	free(gps_info);
}

static void
position_updated(double latitude, double longitude, double altitude, time_t timestamp, void *user_data)
{
	appdata_s *ad = (appdata_s*)user_data;

	dlog_print(DLOG_INFO, LOG_TAG, "position_updated");

	Eina_Strbuf *strbuf = eina_strbuf_new();
	eina_strbuf_append_printf(strbuf,
			    "altitude: %f <br/>latitude: %f <br/>longitude: %f <br/>timestamp: %lf",
			    altitude, latitude, longitude, timestamp);
	char *gps_info = eina_strbuf_string_steal(strbuf);
	//elm_object_text_set(ad->gps_lable, gps_info);

    dlog_print(DLOG_INFO, LOG_TAG, "%s", gps_info);

    eina_strbuf_free(strbuf);
    free(gps_info);
}

static location_service_state_e service_state;

static void
__state_changed_cb(location_service_state_e state, void *user_data)
{
    service_state = state;
    dlog_print(DLOG_INFO, LOG_TAG, "service_state: %s", state == 0 ? "LOCATIONS_SERVICE_DISABLED" : "LOCATIONS_SERVICE_ENABLED");

    if(state != LOCATIONS_SERVICE_ENABLED)
    	return;

    get_current_location((appdata_s*)user_data);
}
static void
start_location_manager(appdata_s *ad)
{
	int ret = location_manager_create(LOCATIONS_METHOD_GPS, &ad->manager);
	ret = location_manager_set_service_state_changed_cb(ad->manager, __state_changed_cb, (void*)ad);
	location_manager_set_position_updated_cb(ad->manager, position_updated, 2, (void*)ad);
	ret = location_manager_start(ad->manager);
}

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_conformant_set(ad->win, EINA_TRUE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* layout */
	ad->layout = elm_layout_add(ad->conform);
	evas_object_size_hint_weight_set(ad->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_theme_set(ad->layout, "layout", "application", "default");
	elm_object_content_set(ad->conform, ad->layout);
	evas_object_show(ad->layout);


	ad->nf = elm_naviframe_add(ad->layout);
	elm_object_part_content_set(ad->layout, "elm.swallow.content", ad->nf);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_BACK, eext_naviframe_back_cb, NULL);
	evas_object_show(ad->nf);

	create_map_view(ad->nf);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);

}

static void end_func(void *data, Ecore_Thread *thread) {
	appdata_s *ad =  (appdata_s *) data;
	ad->thread = 0;
}

static void cancel_func(void *user_data, Ecore_Thread *thread) {
	appdata_s *ad =  (appdata_s *) user_data;
	ad->thread = 0;
}



static void notify_func(void *data, Ecore_Thread *thread/* __UNUSED__*/,
		void *msgdata) {
}

static void heavy_func(void *data /*__UNUSED__*/, Ecore_Thread *thread) {
	appdata_s *ad = (appdata_s*) data;
	ad->thread = thread;


	IoTObserver::getInstance()->findResource();
}

static void eval_cb(void *user_data, Evas_Object *obj, void *event_info) {
	int status = 0;
	appdata_s *ad = (appdata_s*) user_data;
	if (ad->thread == 0) {
		ad->thread = ecore_thread_feedback_run(heavy_func, notify_func,
				end_func, cancel_func, ad, EINA_FALSE);
	} else {
		if (ad->thread) {
			status = ecore_thread_cancel(ad->thread);
			if (status) {
				ad->thread = 0;
			}
		}
	}
}


static bool
app_create(void *data)
{
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);

	/* Hook to take necessary actions before main event loop starts
	   Initialize UI resources and application's data
	   If this function returns true, the main loop of application starts
	   If this function returns false, the application is terminated */
	appdata_s *ad = (appdata_s *) data;
	ad->thread = 0;

	create_base_gui(ad);

	if (false) {
		start_location_manager(ad);
	} else if (false) {
		double lat = 52.165;
		double lon = -2.21;
		map_region_show(lon,lat);
	}

	int status;
	if (ad->thread == 0) {
		ad->thread = ecore_thread_feedback_run(heavy_func, notify_func,
				end_func, cancel_func, ad, EINA_FALSE);
	} else {
		if (ad->thread) {
			status = ecore_thread_cancel(ad->thread);
			if (status) {
				ad->thread = 0;
			}
		}
	}



	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);

}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);
}

static void
app_resume(void *data)
{
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);

	/* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);

	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	dlog_print(DLOG_ERROR, LOG_TAG,__PRETTY_FUNCTION__);
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);
	ui_app_remove_event_handler(handlers[APP_EVENT_LOW_MEMORY]);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ui_app_main() is failed. err = %d", ret);

	return ret;
}
