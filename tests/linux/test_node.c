#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
 
#include "wilddog_debug.h"
#include "wilddog.h"
#include "test.h"

#define TST_ERR	"TST_ERR"
#define TST_STR 	"WILDDOG_STRING"
#define TST_INT		(123456)
#define TST_FLOAT	(3.1412)
#define TST_KEY_SRT		"STR"
#define TST_KEY_BSRT	"BSSTR"
#define TST_KEY_INT		"INT"
#define TST_KEY_FLOAT	"FLOAT"
#define TST_KEY_BOOL	"BOOL"

typedef enum
{
	TST_DATATYPE_STR = 0,
	TST_DATATYPE_BSSTR,
	TST_DATATYPE_INT,
	TST_DATATYPE_FLOAT,
	TST_DATATYPE_BOOL,
	TST_DATATYPE_NULL,
	TST_DATATYPE_MAX
}Test_Data_Type;


#define TST_STR 	"WILDDOG_STRING"
//#define TST_SBSSRT	"1234567890"
#define TST_INT		(123456)
#define TST_FLOAT	(3.1412)
u8 data_type[TST_DATATYPE_MAX][64];
u8 key_str[TST_DATATYPE_MAX][64];
static void test_datainit(void)
{
	int *p_int = (int*)data_type[TST_DATATYPE_INT];
	float *p_float = (float*)data_type[TST_DATATYPE_FLOAT];
	memcpy(data_type[TST_DATATYPE_STR],TST_STR,strlen(TST_STR));
	data_type[TST_DATATYPE_BSSTR][0] = 0xaa;
	data_type[TST_DATATYPE_BSSTR][1] = 0x44;
	data_type[TST_DATATYPE_BSSTR][2] = 0x55;
	data_type[TST_DATATYPE_BSSTR][3] = 0x03;
	*p_int =  TST_INT;
	*p_float = TST_FLOAT;

	
	memcpy(key_str[TST_DATATYPE_STR],TST_KEY_SRT,strlen(TST_KEY_SRT));
	memcpy(key_str[TST_DATATYPE_BSSTR],TST_KEY_BSRT,strlen(TST_KEY_BSRT));
	memcpy(key_str[TST_DATATYPE_INT],TST_KEY_INT,strlen(TST_KEY_INT));	
	memcpy(key_str[TST_DATATYPE_FLOAT],TST_KEY_FLOAT,strlen(TST_KEY_FLOAT));
	memcpy(key_str[TST_DATATYPE_BOOL],TST_KEY_BOOL,strlen(TST_KEY_BOOL));

	//memcpy(data_type[TST_DATATYPE_BSSTR],TST_SBSSRT,strlen(TST_SBSSRT));
		
}
int _wilddog_test_api(void)
{
	test_datainit();
	Wilddog_Node_T *p_success_nodeStr = NULL;

	Wilddog_Node_T *p_fault_nodeBsStr = NULL;
	Wilddog_Node_T *p_fault_nodeInt = NULL,*p_fault_nodeFloat = NULL;
	
	Wilddog_Node_T *p_fault_null = NULL;

	
	
	p_fault_null =  wilddog_node_createObject(NULL);
	p_success_nodeStr =  wilddog_node_createObject(data_type[TST_DATATYPE_STR]);
	p_fault_nodeBsStr =  wilddog_node_createObject(data_type[TST_DATATYPE_BSSTR]);
	p_fault_nodeInt =  wilddog_node_createObject(data_type[TST_DATATYPE_INT]);
	p_fault_nodeFloat =  wilddog_node_createObject(data_type[TST_DATATYPE_FLOAT]);

	if( p_success_nodeStr == NULL || p_fault_nodeBsStr  ||
		p_fault_nodeInt || p_fault_nodeFloat || p_fault_null == NULL)
	{
		
		TEST_RESULT_PRINTF("wilddog_node_createObject",TESTFUNCNAME_TABLECASE,'N',0);
		printf("NULL key =%p,str node=%p;bssrt=%p;int=%p;float=%p\n",p_fault_null,p_success_nodeStr,p_fault_nodeBsStr,p_fault_nodeInt,p_fault_nodeFloat); 
		wilddog_debug_level(WD_DEBUG_ERROR,"wilddog_node_createObject  API error!!\n");
		return -1;
	}
	else 
		TEST_RESULT_PRINTF("wilddog_node_createObject",TESTFUNCNAME_TABLECASE-1,'Y',0);
	wilddog_node_delete(p_success_nodeStr);
	wilddog_node_delete(p_fault_null);

	p_fault_null = wilddog_node_createNull(NULL);
	p_success_nodeStr =  wilddog_node_createNull(data_type[TST_DATATYPE_STR]);
	p_fault_nodeBsStr =  wilddog_node_createNull(data_type[TST_DATATYPE_BSSTR]);
	p_fault_nodeInt =  wilddog_node_createNull(data_type[TST_DATATYPE_INT]);
	p_fault_nodeFloat =  wilddog_node_createNull(data_type[TST_DATATYPE_FLOAT]);
	if( p_success_nodeStr ==NULL || p_fault_nodeBsStr ||
		p_fault_nodeInt || p_fault_nodeFloat || p_fault_null == NULL)
	{
		
		TEST_RESULT_PRINTF("wilddog_node_createNull",TESTFUNCNAME_TABLECASE,'N',0);
		printf("_nullnode = %p;srt node=%p;bssrt=%p;int=%p;float=%p\n",p_fault_null,
				p_success_nodeStr,p_fault_nodeBsStr,p_fault_nodeInt,p_fault_nodeFloat); 
		wilddog_debug_level(WD_DEBUG_ERROR,"wilddog_node_createNull  API error!!\n");
		return -1;
	}
	else 
		TEST_RESULT_PRINTF("wilddog_node_createNull",TESTFUNCNAME_TABLECASE,'Y',0);
	wilddog_node_delete(p_success_nodeStr);
	wilddog_node_delete(p_fault_null);
	
	
	p_fault_null = wilddog_node_createTrue(NULL);
	p_success_nodeStr =  wilddog_node_createTrue(data_type[TST_DATATYPE_STR]);
	p_fault_nodeBsStr =  wilddog_node_createTrue(data_type[TST_DATATYPE_BSSTR]);
	p_fault_nodeInt =  wilddog_node_createTrue(data_type[TST_DATATYPE_INT]);
	p_fault_nodeFloat =  wilddog_node_createTrue(data_type[TST_DATATYPE_FLOAT]);
	if( p_success_nodeStr ==NULL || p_fault_nodeBsStr  ||
		p_fault_nodeInt || p_fault_nodeFloat || p_fault_null == NULL)
	{
		
		TEST_RESULT_PRINTF("wilddog_node_createTrue",TESTFUNCNAME_TABLECASE,'N',0);
		printf("null_node=%p;srt node=%p;bssrt=%p;int=%p;float=%p\n",p_fault_null,p_success_nodeStr,p_fault_nodeBsStr,p_fault_nodeInt,p_fault_nodeFloat); 
		wilddog_debug_level(WD_DEBUG_ERROR,"wilddog_node_createTrue  API error!!\n");
		return -1;
	}
	else
		TEST_RESULT_PRINTF("wilddog_node_createTrue",TESTFUNCNAME_TABLECASE,'Y',0);
	wilddog_node_delete(p_success_nodeStr);
	wilddog_node_delete(p_fault_null);
	
	p_fault_null = wilddog_node_createFalse(NULL);
	p_success_nodeStr =  wilddog_node_createFalse(data_type[TST_DATATYPE_STR]);
	p_fault_nodeBsStr =  wilddog_node_createFalse(data_type[TST_DATATYPE_BSSTR]);
	p_fault_nodeInt =  wilddog_node_createFalse(data_type[TST_DATATYPE_INT]);
	p_fault_nodeFloat =  wilddog_node_createFalse(data_type[TST_DATATYPE_FLOAT]);
	if( p_success_nodeStr ==NULL || p_fault_nodeBsStr ||
		p_fault_nodeInt || p_fault_nodeFloat || p_fault_null == NULL)
	{
		
		TEST_RESULT_PRINTF("wilddog_node_createFalse",TESTFUNCNAME_TABLECASE,'N',0);
		printf("null_node=%p,srt node=%p;bssrt=%p;int=%p;float=%p\n",p_fault_null,p_success_nodeStr,p_fault_nodeBsStr,p_fault_nodeInt,p_fault_nodeFloat); 
		wilddog_debug_level(WD_DEBUG_ERROR,"wilddog_node_createFalse  API error!!\n");
		return -1;
	}
	else
		TEST_RESULT_PRINTF("wilddog_node_createFalse",TESTFUNCNAME_TABLECASE-1,'Y',0);
		
	//wilddog_debug_printnode(p_success_nodeStr);
	wilddog_node_delete(p_success_nodeStr);
	wilddog_node_delete(p_fault_null);

	return 0;
}

int _wilddog_test_node_controlTest(void)
{
	test_datainit();
	
	Wilddog_Node_T *p_node_str=NULL,*p_node_false=NULL,*p_node_ture=NULL;
	Wilddog_Node_T *p_node_bstr=NULL,*p_node_int=NULL,*p_node_float=NULL;
	Wilddog_Node_T *p_root = NULL,*p_clone_root = NULL;
	/*Wilddog_Return_T p_succ = 0,p_succ_rsb =0,p_succ_ifft =0;*/
	Wilddog_Node_T *p_clone_int = NULL,*p_succ_find = NULL;
	char  str[] = "123";
	int len = 0;

 	p_root = wilddog_node_createObject((Wilddog_Str_T *)"Root");
	p_node_str = wilddog_node_createUString(key_str[TST_DATATYPE_STR],data_type[TST_DATATYPE_STR]);
	p_node_bstr = wilddog_node_createBString((Wilddog_Str_T *)key_str[TST_DATATYPE_BSSTR],data_type[TST_DATATYPE_BSSTR],strlen((const char *)data_type[TST_DATATYPE_BSSTR]));
	p_node_int = wilddog_node_createNum(key_str[TST_DATATYPE_INT],TST_INT);
	p_node_float = wilddog_node_createFloat(key_str[TST_DATATYPE_FLOAT],TST_FLOAT);	
	p_node_false =  wilddog_node_createFalse(key_str[TST_DATATYPE_BOOL]);
	p_node_ture =  wilddog_node_createTrue(key_str[TST_DATATYPE_BOOL]);
	
	if( WILDDOG_ERR_NOERR == wilddog_node_setValue(p_node_str, (u8 *)str, strlen((const char *)str)) )
	{
		TEST_RESULT_PRINTF("wilddog_node_setValue ",TESTFUNCNAME_TABLECASE,TEST_OK,0);
	}
	else
	{
		TEST_RESULT_PRINTF("wilddog_node_setValue ",TESTFUNCNAME_TABLECASE,TEST_ERR,0);
	}

	if( (strncmp((const char *)wilddog_node_getValue(p_node_str, &len), (const char *)str, strlen((const char *)str)) == 0) && (len == strlen((const char *)str)))
	{
		TEST_RESULT_PRINTF("wilddog_node_getValue ",TESTFUNCNAME_TABLECASE,TEST_OK,0);
	}
	else
	{
		TEST_RESULT_PRINTF("wilddog_node_getValue ",TESTFUNCNAME_TABLECASE,TEST_ERR,0);
	}

	if(!p_root|| !p_node_str || !p_node_bstr || !p_node_int || !p_node_float || !p_node_false || !p_node_ture)
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"%s : in creat \n",TST_ERR);
		TEST_RESULT_PRINTF("node creat error ",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	/* no brother test */
	if( wilddog_node_addChild(p_root,p_node_str)< 0 ||wilddog_node_addChild(p_node_str,p_node_bstr) ||
		wilddog_node_addChild(p_node_int,p_node_float)<0 || wilddog_node_addChild(p_node_float,p_node_false)<0  ||
		wilddog_node_addChild(p_node_false,p_node_ture)<0 )
		{
			wilddog_debug_level(WD_DEBUG_ERROR,"%s : in add list \n",TST_ERR);
			TEST_RESULT_PRINTF("wilddog_node_addChild error ",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
			return ABORT_ERR;
		}
	
	p_succ_find = wilddog_node_find(p_node_int,"/FLOAT");
	if(p_succ_find == NULL)
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"%s : find/INT/FLOAT \n",TST_ERR);
		TEST_RESULT_PRINTF("wilddog_node_find error ",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	else
		TEST_RESULT_PRINTF("wilddog_node_find  ",TESTFUNCNAME_TABLECASE,TEST_OK,0);
	wilddog_node_addChild(p_node_bstr,p_node_int);
	/*p_clone_root =  /s/bs/int/float/false/ture */
	p_clone_root = wilddog_node_clone(p_root);
	//wilddog_debug_printnode(p_root);
	if(wilddog_node_find(p_clone_root,"/STR/BSSTR/INT/FLOAT") == NULL)
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"/n %s : clone root  /STR/BSSTR/INT/FLOAT \n",TST_ERR);
		TEST_RESULT_PRINTF("wilddog_node_clone error ",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	else 
		TEST_RESULT_PRINTF("wilddog_node_clone  ",TESTFUNCNAME_TABLECASE,TEST_OK,0);
	
	/* brother test p_clone_root =  /STR/BSSTR/INT/FLOAT/BOLL/BOLL
	** p_root = /STR/BSSTR/INT/BOLL/BOLL
	*/
	p_clone_int = wilddog_node_find(p_clone_root,"/STR/BSSTR\n");
	/*/STR/BSSTR/INT/FLOAT/BOLL/BOLL
	**			   |
	*			   /STR/BSSTR/INT/BOLL/BOLL
	*
	*/
	#if 0
	printf("\n-- root:\n");
	wilddog_debug_printnode(p_root);
	
	printf("\n-- p clone int:\n");
	#endif
	wilddog_node_addChild(p_clone_int,p_root);
	p_clone_int = wilddog_node_find(p_clone_root,"/STR/BSSTR/INT");
	
	//wilddog_debug_printnode(p_clone_int);
	/*/STR/BSSTR/STR/BSSTR/INT/BOLL/BOLL
	**				|
	*				|-/BSSTR/INT/BOLL/BOLL
	*/
	//printf("\n broather clone:\n");
	//wilddog_debug_printnode(p_clone_root);
	
	wilddog_node_delete(p_clone_int);

//	wilddog_debug("p_head = %p, next = %p", p_clone_root, p_clone_root->p_wn_child, p_clone_root->);
#if 0
	printf("\ndele clone:\n");
	wilddog_debug_printnode(p_clone_root);
	printf("\n ");
#endif	
	if( wilddog_node_find(p_clone_root,"/STR/BSSTR/INT"))
	{
		wilddog_debug_level(WD_DEBUG_ERROR,"/n%s : DELE /STR/BSSTR/INT FAULT \n",TST_ERR);
		TEST_RESULT_PRINTF("delete node error ",TESTFUNCNAME_TABLECASE,TEST_ERR,ABORT_ERR);
		return ABORT_ERR;
	}
	TEST_RESULT_PRINTF("wilddog_node_delete ",TESTFUNCNAME_TABLECASE,TEST_OK,0);
	wilddog_node_delete(p_clone_root);
	
	return 0;
}

int main(void)
{
	int res = 0;
	res = _wilddog_test_api();
	if(res != 0)
		return res;
	res = _wilddog_test_node_controlTest();
	return res;
}
