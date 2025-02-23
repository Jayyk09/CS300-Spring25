#include <stdio.h>
#include <stdlib.h>
 
int mytime (int left, int right)
{
	int time = 0;
	time = left + rand()%(right - left);	
	// printf("random time is %d sec\n", time);
	return time;
}
