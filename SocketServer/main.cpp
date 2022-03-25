//
//  main.cpp
//  SocketServer
//
//  Created by Edward Janne on 3/23/22.
//

#include <iostream>
#include <iomanip>
#include <chrono> // For steady_clock and time_point
#include <math.h> // For fabs()

#include <termios.h>
#include <signal.h>

#include "Exception.hpp"
#include "Networking.hpp"

using namespace std;
using namespace std::chrono;

static struct termios stdTerm, rawTerm;

void setTermRaw(bool echo = true) {
    tcgetattr(0, &stdTerm);
    rawTerm = stdTerm;
    rawTerm.c_lflag &= ~(ICANON | ECHO);
    if(echo) rawTerm.c_lflag |= ECHO;
    tcsetattr(0, TCSANOW, &rawTerm);
}

void resetTerm() {
    tcsetattr(0, TCSANOW, &stdTerm);
}

atomic_bool quit(false);
struct sigaction oldsa, newsa;
void sigIntHandler(int iSig) {
    quit = true;
}

class MyBufferFunctor : public Connection::BfrFunctor
{
    virtual void operator()(Connection &cnx, Connection::Bfr &buf) {
        cout << ".";
    }
};

MyBufferFunctor functor;

Server Server::shared("en0", functor);

int main(int argc, const char * argv[]) {
    // Intercept SIGINT
    newsa.sa_handler = sigIntHandler;
    sigemptyset(&newsa.sa_mask);
    newsa.sa_flags = 0;
    sigaction(SIGINT, NULL, &oldsa);
    sigaction(SIGINT, &newsa, NULL);
    
    cout << "Setting up streaming server" << endl;
    
    try {
        Server::shared.start();

        // Attempt to retrieve the local hostname and ip address
        char hostBuffer[256];
        int hostname;
        hostname = gethostname(hostBuffer, sizeof(hostBuffer));
        if(hostname != -1) {
            cout << hostBuffer << ":3060 (" << Server::shared.ip() << ":3060)" << endl;
        } else {
            cerr << "Unable to retrieve hostname" << endl;
        }
    } catch(Exception &e) {
        cerr << e << endl;
        return -1;
    } catch(...) {
        cerr << "Unknown exception while attempting to start streaming server" << endl;
        return -1;
    }

    // Set console to unbuffered mode
    setTermRaw(false);
    
    int key = 0;
    
    do {

        // Is data available on stdin?
        struct pollfd fds;
        int ret = 0;
        fds.fd = STDIN_FILENO;
        fds.events = POLLIN;
        ret = poll(&fds, 1, 0);
        if(ret) {
            key = getc(stdin);
            switch(key) {
                case 27:
                case 'Q':
                case 'q':
                    quit = true;
                    break;
            }
        }

    // Loop endlessly
    } while(!quit);

    cout << "Tearing down streaming server" << endl;
    
    Server::shared.stop();
    
    sigaction(SIGINT, &oldsa, NULL);
    
    resetTerm();
    
    return 0;
}
