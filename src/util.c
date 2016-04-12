#include "util.h"
#include <app.h>
#include <Evas.h>
#include <Elementary.h>
#include <efl_extension.h>

void
app_get_resource(const char *edj_file_in, char *edj_path_out, int edj_path_max)
{
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(edj_path_out, edj_path_max, "%s%s", res_path, edj_file_in);
		free(res_path);
	}
}

Evas_Object*
create_editfield(Evas_Object *parent)
{
	Evas_Object *editfield, *entry;

	editfield = elm_layout_add(parent);
	elm_layout_theme_set(editfield, "layout", "searchfield", "singleline");
	evas_object_size_hint_align_set(editfield, EVAS_HINT_FILL, 0.0);
	evas_object_size_hint_weight_set(editfield, EVAS_HINT_EXPAND, 0.0);

	entry = elm_entry_add(editfield);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH);
	elm_object_part_content_set(editfield, "elm.swallow.content", entry);

	return editfield;
}

Evas_Object*
create_nocontent_layout(Evas_Object *parent, const char *text, const char *help_text)
{
	Evas_Object *layout = elm_layout_add(parent);
	elm_layout_theme_set(layout, "layout", "nocontents", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	if (text)
		elm_object_part_text_set(layout, "elm.text", text);
	if (help_text)
		elm_object_part_text_set(layout, "elm.help.text", help_text);

	elm_layout_signal_emit(layout, "text,disabled", "");
	elm_layout_signal_emit(layout, "align.center", "elm");
	return layout;
}
