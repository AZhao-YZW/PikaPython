#include "pikaScript.h"
#include "PikaVM.h"

int main(int argc, char *argv[])
{
    PikaObj* pikaMain = newRootObj("pikaMain", New_PikaMain);
    pika_platform_printf("======[pikascript packages installed]======\r\n");
    pika_printVersion();
    pika_platform_printf("===========================================\r\n");
    pikaVM_runFile(pikaMain, argv[1]);  // argv[1] is filename
    obj_deinit(pikaMain);
    return 0;
}
