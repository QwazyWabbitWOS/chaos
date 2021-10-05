#ifndef PERFORMANCE_H
#define PERFORMANCE_H

// **********************************************
//        Windows high performance counter
// **********************************************

/* How to use these macros:
 * Use only one instance at a time. This uses
 * a single global variable for the start time.
 *
 * Put START_PERFORMANCE_TIMER at the point in the
 * function you want to start timing.
 *
 * Put STOP_PERFORMANCE_TIMER at the end of the timed
 * section. Semi-colons are not needed.
 *
 * Read the execution times in the debugger's Output
 * window in VS or in WinDbg attached to the process.
 * On Linux the timing is output to the server console.
 * If you prefer output to server console on Windows
 * alter the _STOP_PERFORMANCE_TIMER function to use
 * Com_Printf or gi.dprintf as needed.
*/

#ifdef _WIN32
#ifdef _DEBUG
void _STOP_PERFORMANCE_TIMER (char* str);
void _START_PERFORMANCE_TIMER (void);
#define START_PERFORMANCE_TIMER _START_PERFORMANCE_TIMER();
#define STOP_PERFORMANCE_TIMER _STOP_PERFORMANCE_TIMER(__func__);
#else
#define START_PERFORMANCE_TIMER
#define STOP_PERFORMANCE_TIMER
#endif
#endif

#endif /* PERFORMANCE_H */
void DbgPrintf(char* msg, ...);
