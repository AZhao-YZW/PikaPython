#include "pikaScript.h"

int main(int argc, char *argv[])
{
    PikaObj* root = pikaPythonInit();
    pikaPythonShell(root);
    obj_deinit(root);
    return 0;
}
