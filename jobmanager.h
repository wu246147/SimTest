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

    float MissRate();

    float FPR();

    int Total();

    bool result();

    bool labelResult();

    void statistics();
    void resetStatistics();

    //统计结果
    int tp = 0;
    int tn = 0;
    int fp = 0;
    int fn = 0;


};

#endif // JOBMANAGER_H
