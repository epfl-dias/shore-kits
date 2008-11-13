/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   envvar.h
 *
 *  @brief:  "environment variables" singleton class
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#ifndef __UTIL_ENVVAR_H
#define __UTIL_ENVVAR_H

#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif


#include <iostream>
#include <boost/tokenizer.hpp>

#include <map>

#include <readline/readline.h>
#include <readline/history.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include "sm_vas.h"

#include "util/trace.h"
#include "util/guard.h"
#include "util/confparser.h"

using namespace std;



/*********************************************************************
 *
 *  @class: envVar
 *
 *  @brief: Encapsulates the "environment" variables functionality.
 *          It does two things. First, it has the functions that parse
 *          a config file. Second, it stores all the parsed params to
 *          a map of <string,string>. The params may be either read from
 *          the config file or set at runtime.
 *
 *  @note:  Singleton
 *
 *  @usage: - Get instance
 *          - Call setVar()/getVar() for setting/getting a specific variable.
 *          - Call readConfVar() to parse the conf file for a specific variable.
 *            The read value will be stored at the map.
 *          - Call parseSetReq() for parsing and setting a set of params
 *
 *********************************************************************/

const string ENVCONFFILE = "shore.conf";

class envVar 
{
private:

    typedef map<string,string>        envVarMap;
    typedef envVarMap::iterator       envVarIt;
    typedef envVarMap::const_iterator envVarConstIt;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    typedef tokenizer::iterator  tokit;

    envVarMap _evm;
    string _cfname;
    mcs_lock _lock;
    guard<ConfigFile> _pfparser;

    envVar(const string sConfFile=ENVCONFFILE) 
        : _cfname(sConfFile)
    { 
        assert (!_cfname.empty());
        _pfparser = new ConfigFile(_cfname);
        assert (_pfparser);
    }
    ~envVar() { }

    // Helpers

    template <class T>
    string _toString(const T& arg)
    {
        ostringstream out;
        out << arg;
        return (out.str());
    }

    // reads the conf file for a specific param
    // !!! the caller should have the lock !!!
    string _readConfVar(const string& sParam, const string& sDefValue); 
    
public:

    static envVar* instance() { static envVar _instance; return (&_instance); }

    // refreshes all the env vars from the conf file
    const int refreshVars(void);

    // sets a new parameter
    const int setVar(const string& sParam, const string& sValue);
    const int setVarInt(const string& sParam, const int& iValue);

    // retrieves a specific param from the map. if not found, searches the conf file
    // and updates the map
    // @note: after this call the map will have an entry about sParam
    string getVar(const string& sParam, const string& sDefValue);  
    int getVarInt(const string& sParam, const int& iDefValue);  

    // checks if a specific param is set at the map, or, if not at the map, at the conf file
    // @note: this call does not update the map 
    void checkVar(const string& sParam);      

    // sets as input another conf file
    void setConfFile(const string& sConfFile);

    // prints all the env vars
    void printVars(void);

    // parses a SET request
    const int parseOneSetReq(const string& in);
    
    // parses a string of SET requests
    const int parseSetReq(const string& in);

    // gets db-config and then <db-config>-system
    string getSysname();

}; // EOF: envVar



#endif /* __UTIL_ENVVAR_H */

