#ifndef _DEFINES_H_
#define _DEFINES_H_

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string>
#include <sqlite3.h>
#include <glib.h>
#include <json-glib/json-glib.h>
#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Con.h>

#define ESSG(ptr) ((ptr)?eina_strbuf_string_get(ptr):"")

//GW_INF("DEALLOCATING STRBUF %p Value =%s", ptr, eina_strbuf_string_get(ptr));
#define EINA_STRBUF_FREE(ptr) \
    do {	\
        if (ptr!=NULL) {	\
            eina_strbuf_free(ptr); \
            ptr = NULL; \
        }	\
    } while (0);


#define SET_VAL(buf,srch, i,s) {\
    if(g_str_has_suffix(buf, srch)) {\
        i = s; \
        GW_INF("Debug Allocating %p = %p Value =%s", i, s, eina_strbuf_string_get(s)); \
        continue; \
    }\
}

#define EINA_STRBUF_CLONE(a,b)   { \
    a = eina_strbuf_new(); \
    int len = eina_strbuf_length_get(b); \
    const char *val = eina_strbuf_string_get(b); \
    eina_strbuf_append_length(a, val, len); \
}

#define GW_CALLOC(ptr, number, type)	\
    do {	\
        if ((int)(number) <= 0) {	\
            ptr = NULL;	\
            assert(0); \
        } else {	\
            ptr = (type *)calloc(number , sizeof(type));	\
            assert(ptr); \
        }	\
    } while (0);

#define GW_FREE(ptr)    \
    do {	\
        if (ptr!=NULL) {	\
            free(ptr); \
            ptr = NULL; \
        }	\
    } while (0);

#endif
