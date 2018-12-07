#ifndef DELAYEDWORK_H_
#define DELAYEDWORK_H_

#include <time.h>
#include <pthread.h>
#include <queue>

class WorkDelayer
{
public:
	virtual void doWork(unsigned long s, void *p) = 0;
};

struct WorkItem
{
	struct timespec ts;
	WorkDelayer *wd;
	unsigned long sel;
	void *param;
	bool operator<(const WorkItem &wi) const;
};

class DelayedWork
{
	pthread_t tid;
	pthread_mutex_t mtx;
	pthread_cond_t cond;
	std::priority_queue<WorkItem> wq;
	bool running;
	static void *run_th(void *p);
public:
	DelayedWork();
	~DelayedWork();
	int start();
	void stop();
	void put(WorkDelayer *o, unsigned long s, void *p, int sec, int nsec = 0);
	void run();
	void runms(int ms);
};

#endif /*DELAYEDWORK_H_*/
