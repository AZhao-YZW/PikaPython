#include "pikaScript.h"
#include "PikaVM.h"

int main(int argc, char *argv[])
{
    pika_platform_printf("======[pikapython packages installed]======\r\n");
    pika_printVersion();
    pika_platform_printf("PikaStdLib===v1.13.3\r\n");
    pika_platform_printf("===========================================\r\n");

    PikaObj* pikaMain = newRootObj("pikaMain", New_PikaMain);
    pikaVM_runFile(pikaMain, argv[1]);  // argv[1] is filename
    obj_deinit(pikaMain);
    return 0;
}
