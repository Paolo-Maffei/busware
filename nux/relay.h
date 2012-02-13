/*****************************************************************************
Copyright (C) 2011  busware

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*************************************************************************/
#ifndef __RELAY_H__
#define __RELAY_H__
	void relay_init(unsigned short module_idx);
	unsigned long relay_read(unsigned short module_idx, unsigned short pin_idx);
	unsigned short pin_exists(unsigned short module_idx, unsigned short pin_idx);
	void relay_write(unsigned short module_idx, unsigned short pin_idx, unsigned char value);

#endif /* __RELAY_H__ */