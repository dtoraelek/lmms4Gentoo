/*
 * micro_timer.h - simple high-precision timer
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _MICRO_TIMER
#define _MICRO_TIMER

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif


class microTimer
{
public:
	inline microTimer( void )
	{
		reset();
	}

	inline ~microTimer()
	{
	}

	inline void reset( void )
	{
		gettimeofday( &begin, NULL );
	}

	inline Uint32 elapsed( void ) const
	{
		struct timeval now;
		gettimeofday( &now, NULL );
		return( ( now.tv_sec - begin.tv_sec ) * 1000 * 1000 +
					( now.tv_usec - begin.tv_usec ) );
	}


private:
	struct timeval begin;

} ;


#endif
