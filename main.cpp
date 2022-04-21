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
#include "Server.hpp"
#include "Connection.hpp"
#include "LinearAlgebra.hpp"

using namespace std;
using namespace std::chrono;
using namespace ds;

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

#define toDegrees(x) (x * 180.0 / 3.141592653589793)

class MyBufferFunctor : public BufferFunctor
{
    virtual void operator()(Connection &cnx, Buffer &buf) {
        Header &header = buf.getHeader();
        float qi, qj, qk, qr;
        header.getRotation(qi, qj, qk, qr);
        // cout << "q(" << qi << ", " << qj << ", " << qk << ", " << qr << "), ";
        
        /*
        float tx = 2.0 * (qr * qj - qi * qk);
        tx = (tx > 1.0) ? 1.0 : tx;
        tx = (tx < -1.0) ? -1.0 : tx;
        float rx = toDegrees(asin(tx));
        float ry = toDegrees(atan2(2.0 * (qr * qk + qi * qj), (1.0 - 2.0 * (qj*qj + qk*qk))));
        float rz = toDegrees(atan2(2.0 * (qr * qi + qj * qk), (1.0 - 2.0 * (qi*qi + qj*qj))));
        cout << "q(" << qi << ", " << qj << ", " << qk << ", " << qr << "), e(" << rx << ", " << ry << ", " << rz << ")";
        */
        
        vertex3 i(0.0, -1.0, 0.0);
        vertex3 j(-1.0, 0.0, 0.0);
        vertex3 k(0.0, 0.0, -1.0);
        quaternion q(qi, qj, qk, qr);
        vertex3 ri(q * i);
        vertex3 rj(q * j);
        vertex3 rk(q * k);
        // vertex3 rk(ri | rj);
        cout << setprecision(2)
            << "i: (" << ri.x << ", " << ri.y << ", " << ri.z << "), "
            << "j: (" << rj.x << ", " << rj.y << ", " << rj.z << "), "
            << "k: (" << rk.x << ", " << rk.y << ", " << rk.z << ")" << endl;
        
        cnx.returnRecvBuffer(&buf);
    }
};

MyBufferFunctor functor;

Server Server::shared(functor);

int main(int argc, const char * argv[]) {
    // Intercept SIGINT
    newsa.sa_handler = sigIntHandler;
    sigemptyset(&newsa.sa_mask);
    newsa.sa_flags = 0;
    sigaction(SIGINT, NULL, &oldsa);
    sigaction(SIGINT, &newsa, NULL);
    
    /*
    quaternion u(0.0, 0.7071067812, 0.0, 0.7071067812);
    vertex4 v(1.0, 0.0, 0.0, 1.0);
    vertex4 r(u * v);
    cout << "(" << r.x << ", " << r.y << ", " << r.z << ")" << endl;
    */
    
    cout << "Setting up streaming server" << endl;
    
    try {
        string interface("en5");
        if(argc > 1) interface = argv[1];
        Server::shared.start(interface.c_str());

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
