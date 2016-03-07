#include "properties.h"
#include "debugger.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

properties 	* properties::gproperties = NULL;
void 		* properties::session     = NULL;


enum {
    NONE = 0, SLASH = 1, UNICODE = 2, CONTINUE = 3, KEY_DONE = 4, IGNORE = 5
};


void properties::removeInstance()
{
    SM_INF("Trying to remove properties instance");
    if(gproperties){
        gproperties->save();
        SM_INF("Finished saving properties");
        delete gproperties;
        gproperties = NULL;
        SM_INF("properties removed");
        return;
    }
    SM_INF("properties NOT removed");
}

properties * properties::getInstance()
{
    if(!gproperties){
        gproperties = new properties();
        SM_INF("New properties Object Created - %p", gproperties);
        gproperties->load();
    }
    return gproperties;
}

bool properties::save(void)
{
    char buffer1[1024];
    sprintf(buffer1, "%s/config.properties", sessionInfo[CACHELOCATION].c_str());

    FILE *fp = fopen(buffer1, "w");

    if (fp)
    {
        for (map<string,string>::iterator it=sessionInfo.begin(); it!=sessionInfo.end(); ++it)
        {
            SM_INF("Saving %s => %s", it->first.c_str(), it->second.c_str());
            sprintf(buffer1, "%s=%s\n", it->first.c_str(), it->second.c_str());
            fprintf(fp, "%s", buffer1);
        }
        fclose(fp);
    }
    return true;
}

bool properties::load()
{
    SM_INF("New properties Object Created %p", this);
    sessionInfo[CACHELOCATION]="/tmp/sscache";
    int size = 0;
    int ret = 0;

    ret = mkdir(sessionInfo[CACHELOCATION].c_str(), S_IRWXU | S_IRWXG );

    if(ret!=0)
    {
        if(errno==EEXIST){
            SM_INF("Cache location exists. Reusing properties");
        }else{
            SM_INF("Cache Location Cannot be created - caching will be disabled.");
            return true;
        }
    }
    else
    {
        SM_INF("Cache Location Created. Caching Enabled.");
        return true;
    }

    char buffer[1024];
    sprintf(buffer, "%s/config.properties", sessionInfo[CACHELOCATION].c_str());
    ifstream ifs(buffer);
    std::string contents((std::istreambuf_iterator<char>(ifs) ), (std::istreambuf_iterator<char>()));
    ifs.close();
    FILE *fp = fopen(buffer, "r");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        contents.resize(ftell(fp));
        size = ftell(fp);
        rewind(fp);
        int rest = fread(&contents[0], 1, size, fp);
        SM_INF("Output of fread = %d", rest);
        fclose(fp);
    }
    else
    {
        return false;
    }
    ret = 0;
    char nextChar = ' ';
    bool firstChar = true;
    int offset = 0;
    int mode = NONE;
    int unicode = 0;
    int count = 0;
    string kvString;
    int keyLength = -1;

    while (ret!=size)
    {
        //printf("-%c", nextChar);
        nextChar = contents[ret];

        if (mode == UNICODE) {
            int digit = (int)nextChar;
            if (digit >= 0) {
                unicode = (unicode << 4) + digit;
                if (++count < 4) {
                    ret++;
                    continue;
                }
            } else if (count <= 4) {
                SM_INF("Invalid Unicode sequence: illegal character");
                return false;
            }
            mode = NONE;
            kvString += (char) unicode;
            offset++;
            if (nextChar != '\n') {
                ret++;
                continue;
            }
        }
        if (mode == SLASH) {
            mode = NONE;
            switch (nextChar) {
                case '\r':
                    mode = CONTINUE; // Look for a following \n
                    ret++;
                    continue;
                case '\n':
                    mode = IGNORE; // Ignore whitespace on the next line
                    ret++;
                    continue;
                case 'b':
                    nextChar = '\b';
                    break;
                case 'f':
                    nextChar = '\f';
                    break;
                case 'n':
                    nextChar = '\n';
                    break;
                case 'r':
                    nextChar = '\r';
                    break;
                case 't':
                    nextChar = '\t';
                    break;
                case 'u':
                    mode = UNICODE;
                    unicode = count = 0;
                    ret++;
                    continue;
            }
        } else {
            switch (nextChar) {
                case '#':
                case '!':
                    if (firstChar) {
                        while (true) {
                            ret++;
                            nextChar = contents[ret];
                            if (nextChar == '\r' || nextChar == '\n') {
                                break;
                            }
                        }
                        ret++;
                        continue;
                    }
                    break;
                case '\n':
                    if (mode == CONTINUE) { // Part of a \r\n sequence
                        mode = IGNORE; // Ignore whitespace on the next line
                        ret++;
                        continue;
                    }
                    // fall into the next case
                case '\r':
                    mode = NONE;
                    firstChar = true;
                    if (offset > 0 || (offset == 0 && keyLength == 0)) {
                        if (keyLength == -1) {
                            keyLength = offset;
                        }
                        //printf("\r\n1. Getting substring indexes = %d - %d\r\n", offset, keyLength);
                        //printf("kvString = %s\r\n", kvString.c_str());
                        string key, value;
                        //printf("1. KVP %s - %s", key.c_str(), value.c_str());
                        key = kvString.substr(0, keyLength);
                        //printf("1. KVP %s - %s", key.c_str(), value.c_str());
                        value = kvString.substr(keyLength, (offset-keyLength));
                        SM_INF("1. KVP %s=%s", key.c_str(), value.c_str());
                        sessionInfo[key] = value;
                        kvString.clear();
                    }
                    keyLength = -1;
                    offset = 0;
                    ret++;
                    continue;
                case '\\':
                    if (mode == KEY_DONE) {
                        keyLength = offset;
                    }
                    mode = SLASH;
                    ret++;
                    continue;
                case ':':
                case '=':
                    if (keyLength == -1) { // if parsing the key
                        mode = NONE;
                        keyLength = offset;
                        ret++;
                        continue;
                    }
                    break;
            }
            // Cannot use Character.isWhiteSpace(char) due to Java1.3 restriction
            if (isWhitespace(nextChar)) {
                if (mode == CONTINUE) {
                    mode = IGNORE;
                }
                // if key length == 0 or value length == 0
                if (offset == 0 || offset == keyLength
                        || mode == IGNORE) {
                    ret++;
                    continue;
                }
                if (keyLength == -1) { // if parsing the key
                    mode = KEY_DONE;
                    ret++;
                    continue;
                }
            }
            if (mode == IGNORE || mode == CONTINUE) {
                mode = NONE;
            }
        }
        firstChar = false;
        if (mode == KEY_DONE) {
            keyLength = offset;
            mode = NONE;
        }
        kvString += nextChar;
        offset++;
        //printf("kvString Growing = %s - %d\r\n", kvString.c_str(), kvString.length());
        ret++;
    }
    if (mode == UNICODE && count <= 4) {
        SM_INF("Invalid Unicode sequence: expected format \\uxxxx");
        return false;
    }
    if (keyLength == -1 && offset > 0) {
        keyLength = offset;
    }
    if (keyLength >= 0) {
        //printf("\r\n2. Getting substring indexes = %d - %d\r\n", offset, keyLength);
        string key, value;
        string temp = kvString.substr(0, offset);
        key = temp.substr(0, keyLength-1);
        value = temp.substr(keyLength-1);
        if (mode == SLASH) {
            //value.Append("\u0000");
        }
        SM_INF("2. KVP %s=%s", key.c_str(), value.c_str());
        sessionInfo[key] = value;
        kvString.clear();
    }
    SM_INF("Total Number of Objects Read %d", (int)sessionInfo.size());
    SM_INF("Cache Location = %s", get(CACHELOCATION).c_str());
    return true;
}

bool properties::isWhitespace(char ch) {
    switch (ch) {
        case '\t':
        case '\n':
        case '\f':
        case '\r':
        case ' ':
            return true;
        default:
            return false;
    }
}
