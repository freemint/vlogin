/*
 * XaAES - XaAES Ain't the AES (c) 1992 - 1998 C.Graham
 *                                 1999 - 2003 H.Robbers
 *
 * A multitasking AES replacement for MiNT
 *
 * This file is part of XaAES.
 *
 * XaAES is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XaAES is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XaAES; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <mint/dcntl.h>
 
#include <fcntl.h>

#include <gem.h>

#include "moose.h"

int mouseDevHandle = 0;

static vdi_vec *svmotv = 0;
static vdi_vec *svbutv = 0;
static vdi_vec *svwhlv = 0;
/* static vdi_vec *svtimv = 0; */

char moose_name[] = "u:\\dev\\moose";


/*
 * Cleanup on exit
 */
void
moose_exit(short phandle)
{
	if (svmotv)
	{
		void *m, *b, *h;

		vex_motv(phandle, svmotv, &m);
		vex_butv(phandle, svbutv, &b);

		if (svwhlv)
			vex_wheelv(phandle, svwhlv, &h);
	}
}


/*
 * (Re)initialise the mouse device /dev/moose
 */
short
moose_init(short phandle)
{
	struct fs_info info;
	long major, minor;
	struct moose_vecsbuf vecs;
	unsigned short dclick_time;

	if (!mouseDevHandle)
	{
		mouseDevHandle = open(moose_name, O_RDWR);
		if (mouseDevHandle < 0)
		{
			// err = fdisplay(loghandle, true, "Can't open %s\n", moose_name);
			return 0;
		}
	}

	/* first check the xdd version */
	if (fcntl(mouseDevHandle, FS_INFO, &info) != 0)
	{
		// err = fdisplay(loghandle, true, "fcntl(FS_INFO) failed, do you use the right xdd?\n");
		return 0;
	}

	major = info.version >> 16;
	minor = info.version & 0xffffL;
	if (major != 0 || minor < 4)
	{
		// err = fdisplay(loghandle, true, ", do you use the right xdd?\n");
		return 0;
	}

	if (fcntl(mouseDevHandle, MOOSE_READVECS, &vecs) != 0)
	{
		// err = fdisplay(loghandle, true, "Moose set dclick time failed\n");
		return 0;
	}

	if (vecs.motv)
	{
		vex_motv(phandle, vecs.motv, (void **)(&svmotv));
		vex_butv(phandle, vecs.butv, (void **)(&svbutv));

		if (vecs.whlv)
		{
			vex_wheelv(phandle, vecs.whlv, (void **)(&svwhlv));
			// fdisplay(loghandle, true, "Wheel support present\n");
		}
		else {
			// fdisplay(loghandle, true, "No wheel support!!\n");
		}
	}

	dclick_time = 50; // lcfg.double_click_time;
	if (fcntl(mouseDevHandle, MOOSE_DCLICK, &dclick_time) != 0)
		;// err = fdisplay(loghandle, true, "Moose set dclick time failed\n");

	return 1;
}

void
reopen_moose(void)
{
	unsigned short dclick_time = 50;

	mouseDevHandle = open(moose_name, O_RDWR);

	if (mouseDevHandle >= 0)
	{
		if (fcntl(mouseDevHandle, MOOSE_DCLICK, &dclick_time) != 0)
		{
			// DIAG((D_mouse, 0, "moose set dclick time failed\n"));
		}
	}
}
