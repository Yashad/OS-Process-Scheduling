#define EXPDISTSCHED 1
#define LINUXSCHED 2

extern int getschedclass();
extern void setschedclass(int sched_class);
extern double expdev(double lambda);
extern double log(double x);
