#include <stdio.h>
#include <stdint.h>
#include "provmon_proto.h"

int main(int argc, char *argv[])
{
	FILE *f;
	struct provmsg msg;
	long ofs;
	int rv;

	f = fopen(argv[1], "r+");
	if (!f)
		return 1;

	for (;;) {
		ofs = ftell(f);
		if (fread(&msg, sizeof(msg), 1, f) < 1)
			break;
		if (msg.type == PROVMSG_LINK) {
			fseek(f, ofs + sizeof(struct provmsg_link), SEEK_SET);
			rv = 0;
			while (rv < 4) {
				char c;
				if (fread(&c, sizeof(c), 1, f) < 1)
					break;
				if (c == '\0')
					rv++;
				else
					rv = 0;
			}
			if (rv == 4)
				msg_initlen(&msg, ftell(f) - 8 - ofs);
			else
				msg_initlen(&msg, ftell(f) - ofs);
			fseek(f, ofs, SEEK_SET);
			fwrite(&msg, 3, 1, f);
		}
		fseek(f, ofs + msg_getlen(&msg), SEEK_SET);
	}
	rv = ferror(f);

	fclose(f);
	return rv;
}
