/*
 *  THEC64 Mini
 *  Copyright (C) 2017 Chris Smith
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/reboot.h>

void
power_off()
{
	signal(SIGTERM,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	setpgrp();

	sync();

	kill(-1, SIGTERM); // Everything bar init (pid 1)
	sleep(1);
	sync();

	kill(-1, SIGKILL); // Everything bar init (pid 1)
	sleep(1);

	sync();

	reboot( LINUX_REBOOT_CMD_POWER_OFF );
}

void
power_restart()
{
	signal(SIGTERM,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	setpgrp();

	sync();

	kill(-1, SIGTERM); // Everything bar init (pid 1)
	sleep(1);
	sync();

	kill(-1, SIGKILL); // Everything bar init (pid 1)
	sleep(1);

	sync();

	reboot( LINUX_REBOOT_CMD_RESTART );
}
