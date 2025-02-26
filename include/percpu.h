#ifndef PERCPU_H
#define PERCPU_H

#include "utils.h"

template <class T>
class PerCPU
{
private:
    T data[4];

public:
    inline T &forCPU(int id)
    {
        return data[id];
    }

    inline T &mine()
    {
        return forCPU(getCoreID());
    }
};

#endif // PERCPU_H