/*
***************
* b_log.c *
***************
*/
void Log_Printf(const char* format, ...);
void Log_PrintfNoDate(const char* format, ...);
void Log_PrintfWithLocation(const char* File, int Line, const char* msg);
#define LOGPRINTF(QQ) Log_Printf("%s(%d): %s", __FILE__, __LINE__, QQ)
