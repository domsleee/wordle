#pragma once
#include <vector>
#include <string>
#include "string.h"


struct ArgHelper {
    ArgHelper(std::vector<std::string> args)
      : args(args),
        argv(getArgv()) {}

    std::pair<int, char**> getArgcArgv() { return {args.size(), argv}; };
    
    ~ArgHelper() {
        for (std::size_t i = 0; i < args.size(); ++i) free(argv[i]);
        free(argv);
    }
private:
    const std::vector<std::string> args;
    char** argv;

    char **getArgv() {
        char **myArgv = (char **)malloc(sizeof(char*) * args.size());
        for (std::size_t i = 0; i < args.size(); ++i) {
            myArgv[i] = strdup(args[i].c_str());
        }
        return myArgv;
    }
};
