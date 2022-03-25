#include "Exception.hpp"

ostream &operator<<(ostream &os, Exception &e) {
    os << "Exception caught in file \"" << e.file << "\", on line " << e.line << ", with error " << e.err << ": " << e.msg;
    return os;
}

