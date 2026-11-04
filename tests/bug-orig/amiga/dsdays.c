/*
 * dsdays - print what dos.library's DateStamp gives, for the self test of
 * inject's datestamp= option (tests/bug-orig/inject-datestamp.cmake):
 *
 *   days=N minute=M tick=T
 *
 * usage: dsdays        (rc 0)
 */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    struct DateStamp ds;
    char line[80];

    DateStamp(&ds);
    sprintf(line, "days=%ld minute=%ld tick=%ld\n", (long)ds.ds_Days, (long)ds.ds_Minute,
            (long)ds.ds_Tick);
    Write(Output(), line, strlen(line));
    return 0;
}
