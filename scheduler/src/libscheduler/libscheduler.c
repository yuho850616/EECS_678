/** @file libscheduler.c
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements.
*/
typedef struct _job_t
{
  int id;
	int arrivaltime;
	int period;
	int precedence;
	int timeneeded;
	int wait;
	int laststart;
	int responsetime;
	int turnover;
	int RR;
	int core_id;
	struct _job_t *nextptr;
} job_t;

int m_jobcount;
int m_jobtotal;
int m_corecounts;
job_t *m_jarr;
int *m_schedulerptr;
priqueue_t *jobqueue;
scheme_t sobject;

int comparesjf(const void * a, const void * b)
{
	if( ((job_t*)a)->timeneeded < ((job_t*)b)->timeneeded )
  {
		return -1;
	}
  else if( ((job_t*)a)->timeneeded > ((job_t*)b)->timeneeded )
  {
		return 1;
	}
  else
  {
     return (((job_t*)a)->id - ((job_t*)b)->id);
  }
}

int comparepriority(const void * a, const void * b)
{
	if( ((job_t*)a)->precedence < ((job_t*)b)->precedence )
  {
		return -1;
	}
  else if( ((job_t*)a)->precedence > ((job_t*)b)->precedence )
  {
		return 1;
	}
  else
  {
    return ( ((job_t*)a)->id - ((job_t*)b)->id );
  }
}

int comparefifo(const void * a, const void * b)
{
  return(1);
}

/**
  Initalizes the scheduler.

  Assumptions:
    - You may assume this will be the first scheduler function called.
    - You may assume this function will be called once once.
    - You may assume that cores is a positive, non-zero number.
    - You may assume that scheme is a valid scheduling scheme.

  @param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
  @param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
  m_schedulerptr = malloc(cores * sizeof(int));
  jobqueue = malloc(sizeof(priqueue_t));
  sobject = scheme;
  m_corecounts = cores;
  for( int i = 0; i < cores; i++)
  {
    m_schedulerptr[i] = 0;
  }
  switch(sobject)
  {
    case FCFS  :
      priqueue_init(jobqueue, comparefifo);
      break;

    case SJF  :
      priqueue_init(jobqueue, comparesjf);
      break;

    case PSJF  :
      priqueue_init(jobqueue, comparesjf);
      break;

    case PRI  :
      priqueue_init(jobqueue, comparepriority);
      break;

    case PPRI  :
      priqueue_init(jobqueue, comparepriority);
      break;

    case RR  :
      priqueue_init(jobqueue, comparefifo);
      break;
  }
}


/**
  Called when a new job arrives.

  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
  Assumptions:
    - You may assume that every job wil have a unique arrival time.

  @param job_number a globally unique identification number of the job arriving.
  @param time the current time of the simulator.
  @param running_time the total number of time units this job will run before it will be finished.
  @param priority the priority of the job. (The lower the value, the higher the priority.)
  @return index of core job should be scheduled on
  @return -1 if no scheduling changes should be made.

 */
int scheduler_new_job(int job_number, int time, int running_time, int priority)
{
  bool stop = false;
	m_jobcount++;
	m_jobtotal++;
	job_t *jobptr;
	job_t *jobptr1;
	if(job_number == 0)
  {
		m_jarr = malloc(sizeof(job_t));
	  jobptr = m_jarr;
	}
  else if(job_number == 1)
  {
		jobptr = malloc(sizeof(job_t));
		m_jarr->nextptr = jobptr;
	}
  else
  {
		jobptr = m_jarr;
		for(int i = 0; i < (m_jobtotal-2); i++)
    {
			jobptr = jobptr->nextptr;
		}
		jobptr1 = malloc(sizeof(job_t));
		jobptr->nextptr = jobptr1;
		jobptr = jobptr1;
	}
	jobptr->id = job_number;
	jobptr->arrivaltime = time;
	jobptr->period = running_time;
	jobptr->precedence = priority;
	jobptr->timeneeded = running_time;
	jobptr->laststart = -1;
	jobptr->responsetime = -1;
	jobptr->core_id = -1;
	int newcore_id = -1;
	int position = -1;
	for(int i = 0; i < priqueue_size(jobqueue); i++)
  {
		job_t *ptr = (job_t*)priqueue_at(jobqueue,i);
		if(ptr->core_id != -1)
    {
			ptr->timeneeded = ptr->period-(time-ptr->wait-ptr->arrivaltime);
		}
	}
	for(int i = 0; i < m_corecounts; i++)
  {
		if(m_schedulerptr[i] == 0)
    {
      jobptr->responsetime = 0;
      jobptr->wait = 0;
			newcore_id = i;
      jobptr->core_id = i;
			if(sobject == RR)
      {
				jobptr->RR = priqueue_size(jobqueue);
			}
      m_schedulerptr[i] = 1;
      position = priqueue_offer(jobqueue, jobptr);
      stop = true;
			break;
		}
	}
	if(stop == false && (sobject == PSJF || sobject == PPRI))
  {
		position = priqueue_offer(jobqueue, jobptr);
		if(position < m_corecounts)
    {
			for(int i = (priqueue_size(jobqueue)-1); i >= 0; i--)
      {
				if(((job_t*)priqueue_at(jobqueue,i))->core_id != -1)
        {
					job_t *ptr = (job_t*)priqueue_at(jobqueue,i);
					newcore_id = ptr->core_id;
					m_schedulerptr[newcore_id] = 1;
					ptr->core_id = -1;
					ptr->laststart = time;
					ptr->timeneeded = ptr->period-(time-ptr->wait-ptr->arrivaltime);
					if(ptr->timeneeded == ptr->period)
          {
						ptr->responsetime = -1;
						ptr->laststart = -1;
						ptr->wait = -1;
					}
					break;
				}
			}
			jobptr->core_id = newcore_id;
			jobptr->wait = 0;
			jobptr->responsetime = 0;
			stop = true;
		}
	}
  if(stop == false && sobject != PSJF && sobject != PPRI)
  {
    if(sobject == RR)
    {
		    jobptr->RR = priqueue_size(jobqueue);
		}
		 position = priqueue_offer(jobqueue, jobptr);
	}
	return (newcore_id);
}


/**
  Called when a job has completed execution.

  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
 */
int scheduler_job_finished(int core_id, int job_number, int time)
{
  job_t *ptr;
  m_jobcount--;
	int idlenum;
	m_schedulerptr[core_id] = 0;
	if(sobject!=RR)
  {
    for(int i = 0; i < priqueue_size(jobqueue); i++)
    {
			ptr = (job_t*)priqueue_at(jobqueue,i);
			if(ptr->id == job_number)
      {
        ptr = (job_t*)priqueue_remove_at(jobqueue,i);
        idlenum = core_id;
        ptr->timeneeded = ptr->period-(time-ptr->wait-ptr->arrivaltime);
				ptr->turnover = time -ptr->arrivaltime;
				ptr->core_id = -1;
			}
		}
		ptr = (job_t*)priqueue_peek(jobqueue);
		if(ptr != NULL)
    {
      int i = 0;
      while(ptr->core_id != -1 && i < priqueue_size(jobqueue)-1)
      {
        i++;
        ptr = (job_t*)priqueue_at(jobqueue,i);
      }
      if(ptr->laststart != -1)
      {
        ptr->wait = ptr->wait + (time - ptr->laststart);
        ptr->laststart = -1;
      }
      if(ptr->core_id !=-1)
      {
        return -1;
      }
      if(ptr->responsetime == -1)
      {
        ptr->wait = time - ptr->arrivaltime;
        ptr->responsetime = time-ptr->arrivaltime;
      }
      ptr->core_id = idlenum;
      m_schedulerptr[core_id] = 1;
      return ptr->id;
		}
    else
    {
      return -1;
		}
	}
  else
  {
    for(int i = 0; i < priqueue_size(jobqueue); i++)
    {
			ptr = (job_t*)priqueue_at(jobqueue,i);
			if(ptr->id == job_number)
      {
        ptr = (job_t*)priqueue_remove_at(jobqueue,i);
				ptr->turnover = time-ptr->arrivaltime;
				ptr->timeneeded = ptr->period-(time-ptr->wait-ptr->arrivaltime);
        ptr->RR = -1;
				ptr->core_id = -1;
				idlenum = core_id;
			}
		}
		for(int i = 0; i < priqueue_size(jobqueue); i++)
    {
			ptr = (job_t*)priqueue_at(jobqueue,i);
			ptr->RR = i;
		}
		ptr = (job_t*)priqueue_peek(jobqueue);
		if(ptr != NULL)
    {
      int count = 0;
      while(count < (priqueue_size(jobqueue)-1) && ptr->core_id != -1)
      {
        count++;
        ptr = (job_t*)priqueue_at(jobqueue,count);
      }
      if(ptr->laststart != -1)
      {
        ptr->wait = ptr->wait+(time-ptr->laststart);
        ptr->laststart = -1;
      }
			if(ptr->core_id != -1)
      {
				return -1;
			}
			if(ptr->responsetime == -1)
      {
        ptr->wait = time-ptr->arrivaltime;
				ptr->responsetime = time-ptr->arrivaltime;
			}
			ptr->core_id = idlenum;
			m_schedulerptr[core_id] = 1;
			return ptr->id;
		}
    else
    {
      return -1;
		}
	}
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.

  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
 */
int scheduler_quantum_expired(int core_id, int time)
{
  job_t *old;
  job_t *ptr;
  int count = 0;
  for(int i = 0; i < priqueue_size(jobqueue); i++)
  {
    old = (job_t*)priqueue_at(jobqueue,i);
    if(old->core_id == core_id)
    {
      old->RR = priqueue_size(jobqueue);
      old = (job_t*)priqueue_remove_at(jobqueue,i);
      m_schedulerptr[core_id] = -1;
      old->laststart = time;
      old->core_id = -1;
      old->timeneeded = old->period-(time-old->wait-old->arrivaltime);
      break;
    }
  }
  priqueue_offer(jobqueue, old);
  ptr = (job_t*)priqueue_peek(jobqueue);
  if(ptr != NULL)
  {
    count = 0;
    while(count < (priqueue_size(jobqueue)-1) && ptr->core_id != -1)
    {
      count++;
      ptr = (job_t*)priqueue_at(jobqueue,count);
    }
    if(ptr->laststart != -1)
    {
      ptr->wait = ptr->wait + (time - ptr->laststart);
      ptr->laststart = -1;
    }
    if(ptr->core_id != -1)
    {
      return -1;
    }
    if(ptr->responsetime == -1)
    {
      ptr->wait = time-ptr->arrivaltime;
      ptr->responsetime = time-ptr->arrivaltime;
    }
    ptr->core_id = core_id;
    m_schedulerptr[core_id] = 1;
    return ptr->id;
  }
  else
  {
    return -1;
  }
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average waiting time of all jobs scheduled.
 */
float scheduler_average_waiting_time()
{
	job_t *ptr;
  float average;
	int totalWaitTime = 0;
	ptr = m_jarr;
	for(int i = 0; i < m_jobtotal; i++)
  {
		totalWaitTime = totalWaitTime + ptr->wait;
		ptr = ptr->nextptr;
	}
	average = (float)totalWaitTime / (float)m_jobtotal;
	return average;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average turnaround time of all jobs scheduled.
 */
float scheduler_average_turnaround_time()
{
  int turnaround = 0;
	job_t *ptr;
  float average;
	ptr = m_jarr;
	for(int i = 0; i < m_jobtotal; i++)
  {
		turnaround = turnaround + ptr->turnover;
		ptr = ptr->nextptr;
	}
	average = (float)turnaround / (float)m_jobtotal;
	return (average);
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

  Assumptions:
    - This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
  @return the average response time of all jobs scheduled.
 */
float scheduler_average_response_time()
{
  float average;
	int response = 0;
  job_t *ptr;
	ptr = m_jarr;
	for(int i = 0; i < m_jobtotal; i++)
  {
		response = response + ptr->responsetime;
		ptr = ptr->nextptr;
	}
	average = (float)response / (float)m_jobtotal;
	return (average);
}


/**
  Free any memory associated with your scheduler.

  Assumptions:
    - This function will be the last function called in your library.
*/
void scheduler_clean_up()
{
  //free
}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

    2(-1) 4(0) 1(-1)

  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
 */
void scheduler_show_queue()
{
  //leave it blank
}
