#include <stdio.h>
#include "PikaVM.h"

int main(int argc, char* argv[])
{
    PikaObj* pikaMain = NULL;
    pikaMain = pikaPythonInit();
    pikaPythonShell(pikaMain);

    while(1);
}
