//
//  Semaphore.hpp
//  SocketServer
//
//  Created by Edward Janne on 3/23/22.
//

#ifndef __SEMAPHORE_HPP__
#define __SEMAPHORE_HPP__

#include <mutex>
#include <condition_variable>

using namespace std;

class Semaphore {
    public:
        Semaphore(int iCount = 0)
        : count(iCount) { }
        
        void signal() {
            unique_lock<mutex> lock(mtx);
            count++;
            cv.notify_one();
        }
        
        void wait() {
            unique_lock<mutex> lock(mtx);
            while(!count) {
                cv.wait(lock);
            }
            count--;
        }
        
    protected:
        mutex mtx;
        condition_variable cv;
        int count;
};

#endif /* Semaphore_h */
