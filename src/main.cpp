#include "OptionsParse.h"
#include "Runner.h"

int main(int argc, char *argv[]) {
    parseArgs(argc, argv);
    Runner::run();
    return 0;
}
