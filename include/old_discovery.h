/* Copyright (C)
* 2015 - John Melton, G0ORX/N6LYT
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*
*/

#ifndef _OLD_DISCOVERY_H
#define _OLD_DISCOVERY_H

void old_discovery(void);

#ifdef _WIN32
static void discover(int ifNum, u_long ifAddr, u_long ifNet_mask, gboolean tcp);
#else
static void discover(struct ifaddrs *iface);
#endif

#ifdef STEMLAB_DISCOVERY
int  stemlab_get_info(int id);
#endif

#endif
