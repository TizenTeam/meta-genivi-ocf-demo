#ifndef __LBS_MAPS_UTIL_H__
#define __LBS_MAPS_UTIL_H__

#include <Evas.h>

void app_get_resource(const char *edj_file_in, char *edj_path_out, int edj_path_max);
Evas_Object* create_editfield(Evas_Object *parent);
Evas_Object* create_nocontent_layout(Evas_Object* parent, const char* text, const char* help_text);

#endif
