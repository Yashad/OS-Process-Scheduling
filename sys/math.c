#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include<lab1.h>
#include <q.h>
#define RAND_MAX 32767

double log(double );
double expdev(double);


double expdev(double lambda) {
    double dummy;
    do
        dummy= (double) rand() / RAND_MAX;
    while (dummy == 0.0);
    return -log(dummy) / lambda;
}

double log(double x)
{   
    double x1 = (x - 1) / (x + 1);
	double frac = x1;
    double x2 = x1 * x1;
    double D = 1.0;
   
    double t = frac;         
    double sum = t;
	double old_sum = 0.0;
    while(sum!=old_sum)
    {  
     old_sum = sum;
     D += 2.0;
     frac *= x2;
     sum += frac / D;
    }
    return 2.0 * sum;
}