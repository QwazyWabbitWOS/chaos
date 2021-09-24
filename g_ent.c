#include "g_local.h"

static char* ReadEntFile(char* filename)
{
	FILE* fp;
	char* filestring = NULL;
	long int	i = 0;
	int			ch;

	for (;;)
	{
		if ((fp = fopen(filename, "r")) == NULL)
		{
			gi.cprintf(NULL, PRINT_HIGH, "Could not find file \"%s\".\n\n", filename, strerror(errno));
			break;
		}

		for (i = 0; (ch = fgetc(fp)) != EOF; i++);

		filestring = gi.TagMalloc(i + 1, TAG_LEVEL);
		if (!filestring) break;

		fseek(fp, 0, SEEK_SET);
		for (i = 0; (ch = fgetc(fp)) != EOF; i++)
			filestring[i] = ch;
		filestring[i] = '\0';

		break;
	}

	if (fp)
		fclose(fp);

	return(filestring);
}

char* LoadEntFile(char* mapname, char* entities)
{
	char	entfilename[MAX_QPATH] = "";
	char* newentities;
	int		i;
	Com_strcpy(entfilename, sizeof entfilename, "./");
	Com_strcat(entfilename, sizeof entfilename, game_dir->string);
	Com_strcat(entfilename, sizeof entfilename, "/ent/");
	Com_strcat(entfilename, sizeof entfilename, level.mapname);
	Com_strcat(entfilename, sizeof entfilename, ".ent");

	/* convert string to all lowercase (for Linux) */
	for (i = 0; entfilename[i]; i++)
		entfilename[i] = tolower(entfilename[i]);

	newentities = ReadEntFile(entfilename);

	if (newentities)
	{   /*leave these dprints active they show up in the server init console section */
		gi.dprintf("Entity file %s.ent found\n", mapname);
		return(newentities);	/* reassign the ents */
	}
	else
	{
		gi.dprintf("No .ent File for %s.bsp\n", mapname);
		return(entities);
	}
}