/*************************************************************************
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

/*
Parts of the code stem from xboot <http://xboot.org>

*/

#ifndef __VSTDLIB_H__
#define __VSTDLIB_H__

#include <stdarg.h>
#include <stddef.h>

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

extern int uvsnprintf(char * buf, size_t n, const char * fmt, va_list ap);
extern int usprintf(char *pcBuf, const char *pcString, ...);
extern int usnprintf(char *pcBuf, unsigned long ulSize, const char *pcString, ...);
extern size_t ustrlen(const char * s);
extern char * ustrncpy(char * dest, const char * src, size_t n);
extern unsigned long ustrtoul(const char * nptr, char ** endptr, int base);
extern char * ustrstr(const char * s1, const char * s2);
extern int ustrncmp(const char * s1, const char * s2, size_t n);
extern char * ustrncat(char *s1, const char * s2, size_t n);

#ifdef __cplusplus
}
#endif

#endif // __VSTDLIB_H__
