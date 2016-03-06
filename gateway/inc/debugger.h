#ifndef _DEBUGGER_H_
#define _DEBUGGER_H_

#include <Eina.h>
#include <Eet.h>

class debugger;

#define SM_INF(fmt, ...) EINA_LOG_DOM_INFO(debugger::getInstance()->logDomain, fmt, ## __VA_ARGS__)
#define SM_DBG(fmt, ...) EINA_LOG_DOM_DBG(debugger::getInstance()->logDomain, fmt, ## __VA_ARGS__)
#define SM_WRN(fmt, ...) EINA_LOG_DOM_WARN(debugger::getInstance()->logDomain, fmt, ## __VA_ARGS__)
#define SM_ERR(fmt, ...) EINA_LOG_DOM_ERR(debugger::getInstance()->logDomain, fmt, ## __VA_ARGS__)

#define SM_ENTER() EINA_LOG_DOM_INF(debugger::getInstance()->logDomain, "ENTER")
#define SM_LEAVE() EINA_LOG_DOM_INF(debugger::getInstance()->logDomain, "LEAVE")

class debugger{
    public:
        static debugger *getInstance();
        int logDomain;
        ~debugger();
        FILE *fp;

    private:
        static debugger *dbg;
        debugger();
};


#endif /* _DEBUGGER_H_ */
