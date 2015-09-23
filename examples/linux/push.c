/*
 * Copyright (C) 2014-2016 Wilddog Technologies. All Rights Reserved. 
 *
 * FileName: push.c
 *
 * Description: Wilddog demo file for push.
 *
 * Usage: push <-h|-l url>
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
 *                  push -l coap://1.wilddogio.com/a
 *               we will push a node(key is 2, value is 1234) to the cloud,
 *               and cloud will create a father node(key is a hash value), 
 *               result as follow:
 *
 *                  1.wilddogio.com
 *                  |
 *                  + a
 *                    |
 *                    + b: 1234
 *                    |
 *                    + 1442998306824 (key is a hash value, created by cloud)
 *                      |
 *                      + 2: 1234 (the node we pushed)
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


STATIC void test_pushFunc(u8 *p_path,void* arg, Wilddog_Return_T err)
{
                        
    if(err < WILDDOG_HTTP_OK || err >= WILDDOG_HTTP_NOT_MODIFIED)
    {
        wilddog_debug("push failed");
        return;
    }       
    wilddog_debug("new path is %s", p_path);
    *(BOOL*)arg = TRUE;
    return;
}



int main(int argc, char **argv)
{
	int opt;
    char url[1024];
    BOOL isFinish = FALSE;
    Wilddog_T wilddog;
    Wilddog_Node_T * p_node = NULL, *p_head = NULL;
    
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
			//printf("uid:%s\n",optarg);
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

    p_head = wilddog_node_createObject(NULL);

    p_node = wilddog_node_createNum((Wilddog_Str_T *)"2",1234);
    wilddog_node_addChild(p_head, p_node);
    
    wilddog = wilddog_initWithUrl((Wilddog_Str_T *)url);

    wilddog_push(wilddog, p_head, test_pushFunc, (void *)&isFinish);    
    wilddog_node_delete(p_head);
    
    while(1)
    {
        if(isFinish)
        {
            //wilddog_debug("push success!");
            break;
        }
        wilddog_trySync();
    }
    wilddog_destroy(&wilddog);

    return 0;
}

