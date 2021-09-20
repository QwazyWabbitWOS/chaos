/* b_log.c
   blinky
   Created: 2000/05
   Edited:  2000/07/12
*/

#include "g_local.h"

/* where to write log messages */
static const char* f_szLogFile = "chaos/logs/Chaos DM Lives.log";

/* throttling
   we allow up to f_limit messages for every epoch
   an epoch is EPOCH_SECONDS long in seconds
   f_msgs is how many messages we've written in the current epoch
   (and f_currentEpoch keeps track of our current epoch) */
static time_t f_currentEpoch = 0;
static int f_msgs = 0;
static int f_limit = 200;
/* we'll use an epoch of 3600 seconds (one hour) */
#define EPOCH_SECONDS 3600

/* CheckThrottle will return false if we need to throttle (suppress)
   the current message - we do count how many we throttle, and we'll
   write that count out at the beginning of the next epoch */
static int CheckThrottle(time_t* now);

/* this the routine that actually writes to the log file */
static void LogWrite(const char* format, va_list* pmarker, int dated)
{
	time_t now;
	char timestring[64];
	FILE* fp = NULL;
	fp = fopen(f_szLogFile, "a+");
	if (!fp)
		return;
	if (dated)
	{
		time(&now);
		/*	MrG{DRGN} destination safe strcpy replacement */
		Com_strcpy(timestring, sizeof(timestring), ctime(&now));
		Com_strcpy(&timestring[strlen(timestring) - 1], sizeof(&timestring[strlen(timestring) - 1]), ")) ");
		/* MrG{DRGN} END */
		fprintf(fp, "%s", timestring); //QW added %s for security
	}

	vfprintf(fp, format, *pmarker);
	fclose(fp);
}

/* special write function just used for sending out the "Throttling" messages
   this is different from LogWrite because this one uses varargs that haven't
   been captured into marker yet */
static void LogSpecial(const char* format, ...)
{
	time_t now;
	char timestring[64];
	FILE* fp = NULL;

	fp = fopen(f_szLogFile, "a+");

	if (!fp)
		return;

	time(&now);
	/*	MrG{DRGN} destination safe strcpy replacement*/
	Com_strcpy(timestring, sizeof(timestring), ctime(&now));
	Com_strcpy(&timestring[strlen(timestring) - 1], sizeof(&timestring[strlen(timestring) - 1]), ")) ");
	/* MrG{DRGN} END */
	fprintf(fp, "%s", timestring); //QW added %s for security
	va_list marker;
	va_start(marker, format);
	vfprintf(fp, format, marker);
	va_end(marker);
	fclose(fp);
}

void Log_Printf(const char* format, ...)
{
	time_t now;
	va_list marker;
	va_start(marker, format);
	time(&now);
	if (CheckThrottle(&now))
		LogWrite(format, &marker, 1);
	va_end(marker);
}

void Log_PrintfNoDate(const char* format, ...)
{
	time_t now;
	va_list marker;

	time(&now);
	if (!CheckThrottle(&now))
		return;

	va_start(marker, format);

	LogWrite(format, &marker, 0);
}

void Log_PrintfWithLocation(const char* File, int Line, const char* msg)
{
	Log_Printf("%s (%d): %s", File, Line, msg);
}

/* return 0 if we should ignore the current message (because we need to throttle it) */
static int CheckThrottle(time_t* now)
{
	time_t epoch = (*now) / EPOCH_SECONDS;
	if (epoch != f_currentEpoch)
	{
		if (f_msgs > f_limit)
			LogSpecial("Throttled %d messages\n", f_msgs - f_limit);
		/* set new epoch, clear count, return happy */
		f_currentEpoch = epoch;
		f_msgs = 0;
		return 1;
	}
	f_msgs++;
	if (f_msgs < f_limit)
		return 1;
	if (f_msgs == f_limit)
		LogSpecial("Throttling messages\n");
	return 0;
}
