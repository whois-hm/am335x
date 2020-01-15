#include "define.h"
extern void startApplication(int argc, char *argv[]);
extern void endApplication();

int main(int argc, char *argv[])
{
    livemedia_pp::ref();


    startApplication(argc, argv);
    endApplication();
    livemedia_pp::ref(false);
}
