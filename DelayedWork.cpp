
#include "DelayedWork.h"

#define TSCMP(ts1, ts2) ((ts1.tv_sec < ts2.tv_sec || (ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec < ts2.tv_nsec)))
#define TSADD(ts1, s, ns) do { \
	unsigned long long _aux = (((unsigned long long)ts1.tv_sec + s) * (int)1e9) + ts1.tv_nsec + ns; \
	ts1.tv_sec = _aux / (int)1e9; \
	ts1.tv_nsec = _aux % (int)1e9; \
}while(0)

bool WorkItem::operator<(const WorkItem &wi) const
{
	return TSCMP(wi.ts, ts); 
}

DelayedWork::DelayedWork()
{
	pthread_mutex_init(&mtx, NULL);
	pthread_cond_init(&cond, NULL);
}

DelayedWork::~DelayedWork()
{
	stop();
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mtx);
}

int DelayedWork::start()
{
	return pthread_create(&tid, NULL, run_th, this);
}

void DelayedWork::stop()
{
	if(running)
	{
		pthread_mutex_lock(&mtx);
		running = false;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mtx);
		pthread_join(tid, NULL);
	}
}

void DelayedWork::put(WorkDelayer *o, unsigned long s, void *p, int sec, int nsec)
{
	WorkItem wi;
	wi.wd = o;
	wi.sel = s;
	wi.param = p;
	clock_gettime(CLOCK_REALTIME, &wi.ts);
	TSADD(wi.ts, sec, nsec);
	pthread_mutex_lock(&mtx);
	wq.push(wi);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mtx);
}

void DelayedWork::runms(int ms)
{
	struct timespec ets;
	clock_gettime(CLOCK_REALTIME, &ets);
	TSADD(ets, (ms/1000), ((ms%1000) * 1000000));
	for(;;)
	{
		const struct timespec *nts = &ets;
		pthread_mutex_lock(&mtx);
		if(!wq.empty() && TSCMP(wq.top().ts, ets))
		{
			nts = &wq.top().ts;
		}
		pthread_cond_timedwait(&cond, &mtx, nts);
		struct timespec cts;
		clock_gettime(CLOCK_REALTIME, &cts);
		if(!wq.empty() && TSCMP(wq.top().ts, cts))
		{
			WorkItem wi = wq.top();
			wq.pop();
			pthread_mutex_unlock(&mtx);
			wi.wd->doWork(wi.sel, wi.param);
			continue;
		}
		pthread_mutex_unlock(&mtx);
		if(TSCMP(ets, cts)) break;
	}
}

#define LONG_SLEEP 1000000

void DelayedWork::run()
{
	running = true;
	while(running)
	{
		struct timespec tts;
		pthread_mutex_lock(&mtx);
		if(wq.empty())
		{
			clock_gettime(CLOCK_REALTIME, &tts);
			tts.tv_sec += LONG_SLEEP;
		}
		else
		{
			tts.tv_sec = wq.top().ts.tv_sec;
			tts.tv_nsec = wq.top().ts.tv_nsec;
		}
		pthread_cond_timedwait(&cond, &mtx, &tts);
		clock_gettime(CLOCK_REALTIME, &tts);
		if(!wq.empty() && TSCMP(wq.top().ts, tts))
		{
			WorkItem wi = wq.top();
			wq.pop();
			pthread_mutex_unlock(&mtx);
			wi.wd->doWork(wi.sel, wi.param);
			continue;
		}
		pthread_mutex_unlock(&mtx);
	}
}

void *DelayedWork::run_th(void *p)
{
	((DelayedWork*)p)->run();
}


