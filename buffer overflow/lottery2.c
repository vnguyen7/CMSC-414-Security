/* lottery2.c */

#include <stdio.h>    /* for printf() */
#include <stdlib.h>   /* for rand() and srand() */
#include <sys/time.h> /* for gettimeofday() */

int your_fcn()
{
    /* Provide three different versions of this 
     * that each win the "lottery" in main().
     * Submit COPIES of this file (lottery1.c, 
     * lottery2.c, and lottery3.c). */
  char buffer[5];
  int *ret;
  ret = buffer + 13;
  *ret += 34;
    return 0;
}

int main()
{
    /* Seed the random number generator */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);

    const char *sad = ":(";
    const char *happy = ":)";
    int rv;
    rv = your_fcn();

    /* Lottery time */
    if(rv != rand())
        printf("You lose %s\n", sad);
    else
        printf("You win! %s\n", happy);

    return EXIT_SUCCESS;
}
