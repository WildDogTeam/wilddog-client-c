/*
 * debug.c
 *
 *  Created on: 2014年11月6日
 *      Author: x
 */
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "wilddog_debug.h"

uint32_t log_level_at_run_time = LOG_LEVEL;

void log_print_(int level, int line, const char *func, const char *file, const char *msg, ...)
{
       char _buffer[MAX_DEBUG_MESSAGE_LENGTH];
        static char * levels[] = {
                "",
                "LOG  ",
                "DEBUG",
                "WARN ",
                "ERROR",

        };
        va_list args;
        va_start(args, msg);
        //file = file ? strrchr(file,'/') + 1 : "";
        int trunc = snprintf(_buffer, arraySize(_buffer), "<%s> %s %s(%d):", levels[level], func, file, line);
        if (debug_output_)
        {
            debug_output_(_buffer);
          if (trunc > arraySize(_buffer))
          {
              debug_output_("...");
          }
        }
        trunc = vsnprintf(_buffer,arraySize(_buffer), msg, args);
        if (debug_output_)
        {
          debug_output_(_buffer);
          if (trunc > arraySize(_buffer))
          {
              debug_output_("...");
          }
          debug_output_("\r\n");
        }
}

void debug_output_(const char* c)
{
	printf("%s",c);
}




