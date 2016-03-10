#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_

#include <string>
#include <cstring>
#include <map>
#include <vector>
#include <stdio.h>

using namespace std;

#define PROPGET(x) 		properties::getInstance()->get(x)
#define PROPSET(x,y) 	properties::getInstance()->set(x,y)

#define CACHELOCATION 	"cachelocation"
#define RVIURL 		"rviurl"

class properties {

	private:

		pthread_mutex_t propmutex;
        static bool isWhitespace(char ch);
        static properties *gproperties;
        map<string, string> sessionInfo;
        static void *session;

        properties() {
            fprintf(stdout,"properties Constructor Invoked.");
            pthread_mutex_init(&propmutex, NULL);
        }

        ~properties(){
            fprintf(stdout,"properties Destructor Invoked.");
            pthread_mutex_destroy(&propmutex);
            sessionInfo.clear();
        }

    public:
        static properties * getInstance();
        static void removeInstance();

        bool load();
        bool save();

        const string& get(const string& key) {
        	pthread_mutex_lock (&propmutex);
        	if(sessionInfo.end() == sessionInfo.find(key)){
        		pthread_mutex_unlock (&propmutex);
        		return NULL;
        	}
        	pthread_mutex_unlock (&propmutex);
            return sessionInfo[key];
        }

        void set(const string& key, const char *value) {
        	if(value!=NULL && key.length()>0){
        		pthread_mutex_lock (&propmutex);
        		if(value){
        			sessionInfo[key] = value;
        		}
        		pthread_mutex_unlock (&propmutex);
                fprintf(stdout,"SETSESSIONINFO : %s = %s\n", key.c_str(), sessionInfo[key].c_str());
            }else{
                fprintf(stdout,"SETSESSIONINFO : %s = Invalid Value\n", key.c_str());
            }
        }

        static void setSessionReference(void *ptr){
        	session = ptr;
        }
        static void * getSessionReference(){
        	return session;
        }
};

#endif /* _SESSION_H_ */
