#include "g_local.h"
#include "c_motd.h"

void LoadMOTD(void)
{
	FILE* fp;
	char file[MAX_QPATH];
	char line[80];
	size_t i = 0;

	Com_strcpy(file, sizeof file, "./");
	Com_strcat(file, sizeof file, game_dir->string);
	Com_strcat(file, sizeof file, "/motd.txt");

	if ((fp = fopen(file, "r")) == NULL)
	{
		gi.cprintf(NULL, PRINT_HIGH, "Could not find file \"%s\".\n\n", file, strerror(errno));
		return;
	}

	while ((!feof(fp)) && (i < sizeof motd))
	{
		size_t	len;
		if (fgets(line, sizeof line, fp) == NULL) {
			if (!feof(fp))
				gi.dprintf("%s problem reading %s\n", __func__, file);
		}
		len = strlen(line);
		while (line[len] == '\n' || line[len] == '\r')
			len--;

		if ((i + len) < sizeof motd)
			strncpy(motd + i, line, len);
		else
			gi.dprintf("MOTD is too long (> %u chars), truncated\n", sizeof motd);
		i += len;
	}
	fclose(fp);
}