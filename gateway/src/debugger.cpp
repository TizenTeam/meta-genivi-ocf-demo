#include "debugger.h"

debugger * debugger::dbg = NULL;

debugger * debugger::getInstance(){
    if(!dbg)
        dbg = new debugger();
    return dbg;
}

debugger::debugger(){
    eina_init();
    eet_init();
    logDomain = eina_log_domain_register("gateway", EINA_COLOR_CYAN);
    if(logDomain<0)
        logDomain = EINA_LOG_DOMAIN_GLOBAL;
    FILE *fp = fopen("/tmp/logger.log", "w");
    eina_log_print_cb_set(eina_log_print_cb_file, fp);
}

debugger::~debugger(){
    if(logDomain != EINA_LOG_DOMAIN_GLOBAL)
        eina_log_domain_unregister(logDomain);
    fflush(fp);
    fclose(fp);
}
