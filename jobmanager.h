#ifndef JOBMANAGER_H
#define JOBMANAGER_H
#include "jobworker.h"
class Jobmanager
{
public:
    Jobmanager();
    Jobmanager(int _jobworker_count);

    std::vector<Jobworker> jobworkers;

    int jobworker_num = 8;

};

#endif // JOBMANAGER_H
