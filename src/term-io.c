/****************************************************************************
**
** Copyright 2024 Jiamu Sun
** Contact: barroit@linux.com
**
** This file is part of livaut.
**
** livaut is free software: you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation, either version 3 of the License, or (at your
** option) any later version.
**
** livaut is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with livaut. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include <string.h>
#include <stdio.h>

void fputs_wp(const char *txt, FILE *stream, const char *pfx, size_t wrap)
{
	size_t pfxlen = strlen(pfx), txtlen = strlen(txt), len = pfxlen;

	/* usually ‘pfx’ will not exceed ‘wrap’ */
	fputs(pfx, stream);

	while (39) {
		size_t room = wrap - len;

		fprintf(stream, "%.*s\n", (int)room, txt);

		if (txtlen <= room)
			break;

		txt += room;
		txtlen -= room;
		len = fprintf(stream, "%*s", (int)pfxlen, "");
	}
}
