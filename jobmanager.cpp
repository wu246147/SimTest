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

float Jobmanager::MissRate()
{
    if(tp + fn + tn + fp == 0)
    {
        return 0;
    }
    //正常漏检应该是fn / (tp + fn)
    return (float)fn / (tp + fn + tn + fp);
}

float Jobmanager::FPR()
{
    if(tp + fn + tn + fp == 0)
    {
        return 0;
    }
    //正常误检应该是fn / (tn + fp)
    return (float)fp / (tp + fn + tn + fp);
}

int Jobmanager::Total()
{
    return tp + fn + tn + fp;
}


bool Jobmanager::result()
{
    for(int id = 0; id < jobworkers.size(); ++id)
    {
        if(!jobworkers[id].result())
        {
            return false;
        }
    }
    return true;
}

bool Jobmanager::labelResult()
{
    for(int id = 0; id < jobworkers.size(); ++id)
    {
        if(!jobworkers[id].labelResult)
        {
            return false;
        }
    }
    return true;
}

void Jobmanager::statistics()
{
    if(labelResult())
    {
        if(result())
        {
            tn += 1;
        }
        else
        {
            fp += 1;
        }
    }
    else
    {
        if(result())
        {
            fn += 1;
        }
        else
        {
            tp += 1;
        }
    }
}

void Jobmanager::resetStatistics()
{
    tp = 0;
    tn = 0;
    fp = 0;
    fn = 0;
}
