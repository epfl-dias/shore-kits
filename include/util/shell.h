/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shell.h
 *
 *  @brief:  Abstract shell class for the test cases
 *
 *  @author: Ippokratis Pandis (ipandis)
 */

#ifndef __UTIL_SHELL_H
#define __UTIL_SHELL_H

#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif

#include <map>

#include <readline/readline.h>
#include <readline/history.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include "util/command/command_handler.h"
#include "util/command/tracer.h"
#include "util/envvar.h"

#include "util.h"


using namespace std;



extern "C" void sig_handler_fwd(int sig);


typedef map<string,command_handler_t*> cmdMap;
typedef cmdMap::iterator cmdMapIt;


/*********************************************************************
 *
 *  @brief: Few basic commands
 *
 *********************************************************************/

class envVar;


class quit_cmd_t : public command_handler_t {
public:
    quit_cmd_t() { }
    ~quit_cmd_t() { }
    void init() { }
    void close() { }
    void setaliases();
    const int handle(const char* cmd) { return (SHELL_NEXT_QUIT); }
    void usage() { TRACE( TRACE_ALWAYS, "Exits the system\n"); }
    const string desc() { return (string("quit")); }               
};


class help_cmd_t : public command_handler_t {
private:
    cmdMap* _pcmds; // pointer to the supported commands
public:
    help_cmd_t(cmdMap* pcmds) : _pcmds(pcmds) { assert(pcmds); }
    ~help_cmd_t() { }
    void init() { }
    void close() { }
    void setaliases();
    const int handle(const char* cmd);
    void usage() { 
        TRACE( TRACE_ALWAYS, "HELP       - prints usage\n"); 
        TRACE( TRACE_ALWAYS, "HELP <cmd> - prints detailed help for <cmd>\n"); 
    }
    const string desc() { return (string("help")); }              

    void list_cmds();
}; 

class set_cmd_t : public command_handler_t {
private:
    envVar* ev;
public:
    set_cmd_t() { }
    ~set_cmd_t() { }
    void init() { ev = envVar::instance(); }
    void close() { }
    void setaliases();
    const int handle(const char* cmd);
    void usage();
    const string desc() { return (string("Sets env vars")); }               
};

class env_cmd_t : public command_handler_t {
private:
    envVar* ev;
public:
    env_cmd_t() { }
    ~env_cmd_t() { }
    void init() { ev = envVar::instance(); }
    void close() { }
    void setaliases();
    const int handle(const char* cmd);
    void usage();
    const string desc() { return (string("Prints env vars")); }               
};

class conf_cmd_t : public command_handler_t {
private:
    envVar* ev;
public:
    conf_cmd_t() { }
    ~conf_cmd_t() { }
    void init() { ev = envVar::instance(); }
    void close() { }
    void setaliases();
    const int handle(const char* cmd);
    void usage();
    const string desc() { return (string("Rereads env vars")); }               
};




/*********************************************************************
 *
 *  @abstract class: shell_t
 *
 *  @brief:          Base class for shells.
 *
 *  @usage:          - Inherit from this class
 *                   - Implement the process_command() function
 *                   - Call the start() function
 *
 *********************************************************************/



class shell_t 
{
private:
    char* _cmd_prompt;
    int   _cmd_counter;
    bool  _save_history;
    int   _state;

    // cmds
    guard<quit_cmd_t> _quit_cmd;
    guard<help_cmd_t> _help_cmd;
    guard<set_cmd_t>  _set_cmd;
    guard<env_cmd_t>  _env_cmd;
    guard<conf_cmd_t> _conf_cmd;
    guard<trace_cmd_t>   _tracer_cmd;
    
    const int _register_commands();    

protected:
    mcs_lock _lock;
    cmdMap _cmds;
    cmdMap _aliases;
    bool _processing_command;
    
public:

    shell_t(const char* prompt = QPIPE_PROMPT, bool save_history = true) 
        : _cmd_counter(0), _save_history(save_history), 
          _state(SHELL_NEXT_CONTINUE), _processing_command(false)
    {
        _cmd_prompt = new char[SHELL_COMMAND_BUFFER_SIZE];
        if (prompt)
            strncpy(_cmd_prompt, prompt, strlen(prompt));

        _register_commands();
    }

    virtual ~shell_t() 
    {
        if (_cmd_prompt)
            delete [] _cmd_prompt;       
    }


    static shell_t* &instance() { static shell_t* _instance; return _instance; }

    const int get_command_cnt() { return (_cmd_counter); }

    static void sig_handler(int sig) {
	assert(sig == SIGINT && instance());	
	if( int rval=instance()->SIGINT_handler() )
	    exit(rval);
    }

    // should register own commands
    virtual const int register_commands()=0;
    const int add_cmd(command_handler_t* acmd);
    const int init_cmds();
    const int close_cmds();

    // basic shell functionality    
    //    virtual int process_command(const char* cmd, const char* cmd_tag)=0;

    virtual int SIGINT_handler() { return (ECANCELED); /* exit immediately */ }     
    int start();
    int process_one();
    virtual int process_command(const char* command, const char* command_tag)=0;

}; // EOF: shell_t



#endif /* __UTIL_SHELL_H */

