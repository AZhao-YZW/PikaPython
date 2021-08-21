/* banchmark for memory */
#include "BaseObj.h"
#include "SysObj.h"
#include <stdio.h>
extern DMEM_STATE DMEMS;
static uint8_t DMEMORY[DMEM_TOTAL_SIZE];

void checker_printMem(char *info, uint32_t size)
{
	printf("%s", info);
	if (size <= 1024)
	{
		printf("%d byte\r\n", size);
		return;
	}
	printf("%0.2f Kb\r\n", size / 1024.0);
	return;
}

void checker_printMemUsage(char *testName)
{
	printf("---------------------------\r\n");
	printf("Testing :%s\r\n", testName);
	checker_printMem("    max = ", DMEMS.maxNum * DMEM_BLOCK_SIZE);
	checker_printMem("    now = ", DMEMS.blk_num * DMEM_BLOCK_SIZE);
	printf("---------------------------\r\n");
}
void checker_memInfo(void)
{
	printf("---------------------------\r\n");
	printf("Memory pool info:\r\n");
	checker_printMem("	mem block size = ", DMEM_BLOCK_SIZE);
	checker_printMem("	mem block num = ", DMEM_BLOCK_NUM);
	checker_printMem("	mem pool size = ", sizeof(DMEMORY));
	checker_printMem("	mem state size = ", sizeof(DMEM_STATE));
	checker_printMem("	mem state size = ", (sizeof(DMEMORY) + sizeof(DMEM_STATE)));
	printf("---------------------------\r\n");
}
void checker_assertMemFree()
{
	if (0 == DMEMS.blk_num)
	{
		DMEMS.maxNum = 0;
		return;
	}
	printf("[Error]: Memory free error.\r\n");
	while (1)
		;
}

void checker_objMemChecker(void *NewFun, char *objName)
{
	{
		/* new root object */
		MimiObj *obj = newRootObj("obj", NewFun);
		char testName[256] = {0};
		sprintf(testName, "Root %s object", objName);
		checker_printMemUsage(testName);
		obj_deinit(obj);
		checker_assertMemFree();
	}

	{
		MimiObj *obj = New_TinyObj(NULL);
		char testName[256] = {0};
		sprintf(testName, "%s object", objName);
		checker_printMemUsage(testName);
		obj_deinit(obj);
		checker_assertMemFree();
	}
}

int32_t main()
{
	checker_objMemChecker(New_TinyObj, "tiny");
	checker_objMemChecker(New_BaseObj, "base");
	checker_objMemChecker(New_SysObj, "sys");
	{
		Arg *arg = New_arg(NULL);
		checker_printMemUsage("void arg");
		arg_deinit(arg);
		checker_assertMemFree();
	}
	{
		Arg *arg = New_arg(NULL);
		arg_setInt(arg, 0);
		checker_printMemUsage("int arg");
		arg_deinit(arg);
		checker_assertMemFree();
	}
	{
		Arg *arg = New_arg(NULL);
		arg_setFloat(arg, 0);
		checker_printMemUsage("float arg");
		arg_deinit(arg);
		checker_assertMemFree();
	}
	{
		Arg *arg = New_arg(NULL);
		arg_setStr(arg, "test string");
		checker_printMemUsage("str arg");
		arg_deinit(arg);
		checker_assertMemFree();
	}
	{
		Args *args = New_args(NULL);
		checker_printMemUsage("void args");
		args_deinit(args);
		checker_assertMemFree();
	}
	{
		Args *args = New_args(NULL);
		args_setInt(args, "testInt", 0);
		checker_printMemUsage("one int args");
		args_deinit(args);
		checker_assertMemFree();
	}
	{
		Args *args = New_args(NULL);
		args_setInt(args, "testInt1", 0);
		args_setInt(args, "testInt2", 0);
		checker_printMemUsage("two int args");
		args_deinit(args);
		checker_assertMemFree();
	}
	{
		Args *args = New_args(NULL);
		args_setFloat(args, "testFloat", 0);
		checker_printMemUsage("one float args");
		args_deinit(args);
		checker_assertMemFree();
	}
	{
		Args *args = New_args(NULL);
		args_setFloat(args, "testFloat", 0);
		args_setFloat(args, "testFLoat", 0);
		checker_printMemUsage("two float args");
		args_deinit(args);
		checker_assertMemFree();
	}
	checker_memInfo();
}
