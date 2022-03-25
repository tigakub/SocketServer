#ifndef __EXCEPTION_HPP__
#define __EXCEPTION_HPP__

#include <iostream>

using namespace std;

typedef struct Exception {
    Exception(const string &iFile, int iLine, int iError, const string &iMsg)
    : file(iFile), line(iLine), msg(iMsg), err(iError) { }
    
    string file, msg;
    int line, err;
} Exception;

ostream &operator<<(ostream &os, Exception &e);

#endif
