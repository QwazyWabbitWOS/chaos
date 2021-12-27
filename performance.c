// **********************************************
//        Windows high performance counter
// **********************************************
//lifted from r1q2 :)

// this version outputs to the debugger window
// instead of to the console log.

#include "g_local.h"
#include "performance.h"

#ifdef _WIN32
#ifdef _DEBUG

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>

LARGE_INTEGER start;
double totalTime = 0;

void _START_PERFORMANCE_TIMER(void)
{
	QueryPerformanceCounter(&start);
}

void _STOP_PERFORMANCE_TIMER(char* str)
{
	double res;
	LARGE_INTEGER stop;
	__int64 diff;
	LARGE_INTEGER freq;
	static char string[64];

	QueryPerformanceCounter(&stop);
	QueryPerformanceFrequency(&freq);
	diff = stop.QuadPart - start.QuadPart;
	res = ((double)((double)diff / (double)freq.QuadPart));
	Com_sprintf(string, sizeof string,
		"%s executed in %.9f secs.\n", str, res);
	OutputDebugString(string);
	//	Com_Printf (string);
	totalTime += res;
}
#endif
#endif

//QW//
/*
Use this function to trace execution or whatever.
This improves upon OutputDebugString a bit to allow var_args instead of static text.
Outputs to the debugger. WinDbg or VS.
Uses Quake 2's gi.dprintf to output to the Quake 2 console.
*/
void DbgPrintf(char* msg, ...)
{
	//To use: DbgPrintf("%s was called.\n", __func__);
	//In Linux, this function becomes a call to gi.dprintf but
	//it outputs only if developer cvar is set.

	va_list	argptr;
	char	text[1024];//QW// keep within protocol limits

	va_start(argptr, msg);
	vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

#if defined _WIN32
	OutputDebugString(text);
#else // Not WIN32
	if (developer->value)
		gi.dprintf(text);
#endif /* _WIN32 */
}
