/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: addObserver.c
 *
 * Description: Wilddog demo file for observe.
 *
 * Usage: addObserver <-h|-l url>
 * 
 *          -h: help
 *          -l: note that a url followed
 *          url:
 *                  like coap://<your appid>.wilddogio.com/[path], <your appid>
 *                  is the appid of the app you created, and path is the path(
 *                  node path) in the app. if the tree like this, <1> is your 
 *                  appid, <a> and <a/b> are both path.
 *                  
 *                  your data tree in cloud:
 *
 *                  1.wilddogio.com
 *                  |
 *                  + a
 *                    |
 *                    + b: 1234
 *
 *      example: if we input :
 *                  addObserver -l coap://1.wilddogio.com/a
 *               we can get value of a, which is a node of app <1>.
 *               and then if we change a's value ,the new value will print in
 *               the console.
 *
 * History:
 * Version      Author          Date        Description
 *
 * 0.4.3        Baikal.Hu       2015-07-16  Create file.
 * 0.5.1        Jimmy.Pan       2015-09-22  Add notes.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "wilddog.h"
#include "wilddog_debug.h"
#include "demo.h"




STATIC void test_addObserverFunc(
    const Wilddog_Node_T* p_snapshot, 
    void* arg,
    Wilddog_Return_T err)
{
    
    *(BOOL*)arg = TRUE;
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("addObserver failed!");
        return;
    }

    wilddog_debug_printnode(p_snapshot);
    wilddog_debug("addObserver data!");
    
    return;
}

int main(int argc, char **argv)
{
    int opt;
    char url[1024];
    BOOL isFinished = FALSE;
    Wilddog_T wilddog;
    STATIC int count = 0;   
    
    memset( url, 0, sizeof(url));


    while ((opt = getopt(argc, argv, "hl:")) != -1) 
    {
        switch (opt) 
        {
		case 'h':
			fprintf(stderr, "Usage: %s  -l url\n",
		           argv[0]);
			return 0;
		case 'l':
			strcpy(url, (const char*)optarg);
			break;			
		default: /* '?' */
			fprintf(stderr, "Usage: %s  -l url\n",
		           argv[0]);
			return 0;
        }
    }

    if( argc <3 )
    {
        printf("Usage: %s  -l url\n", argv[0]);
        return 0;
    }


    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);

    if(0 == wilddog)
    {
        wilddog_debug("new wilddog failed!");
        return 0;
    }
    wilddog_addObserver(wilddog, WD_ET_VALUECHANGE, test_addObserverFunc, (void*)&isFinished);
    while(1)
    {
        if(TRUE == isFinished)
        {
            /*do not quit*/
            //wilddog_debug("get new data %d times!", count++);
            isFinished = FALSE;
            if(count > 0)
            {
                //wilddog_debug("off the data!");
                wilddog_removeObserver(wilddog, WD_ET_VALUECHANGE);
                break;
            }
        }
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);
    
    return 0;
}

