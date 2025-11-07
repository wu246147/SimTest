#include "jobmanager.h"

Jobmanager::Jobmanager()
{

}

Jobmanager::Jobmanager(int _jobworker_count)
{
    jobworker_num = _jobworker_count;
    for(int i = 0; i < jobworker_num; i++)
    {
        Jobworker jobworker;
        jobworkers.push_back(jobworker);
    }
}
