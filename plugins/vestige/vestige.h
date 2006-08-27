/*
 * vestige.h - instrument VeSTige for hosting VST-plugins
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


#ifndef _VESTIGE_H
#define _VESTIGE_H

#include "instrument.h"
#include "midi.h"

#ifdef QT4

#include <QtCore/QMutex>

#else

#include <qmutex.h>

#endif


#include "spc_bg_hndl_widget.h"


class pixmapButton;
class QPushButton;
class remoteVSTPlugin;
class QPixmap;


class vestigeInstrument : public instrument, public specialBgHandlingWidget
{
	Q_OBJECT
public:
	vestigeInstrument( instrumentTrack * _channel_track );
	virtual ~vestigeInstrument();

	virtual void play( void );

	virtual void FASTCALL playNote( notePlayHandle * _n );
	virtual void FASTCALL deleteNotePluginData( notePlayHandle * _n );


	virtual void FASTCALL saveSettings( QDomDocument & _doc,
							QDomElement & _parent );
	virtual void FASTCALL loadSettings( const QDomElement & _this );

	virtual QString nodeName( void ) const;

	virtual void FASTCALL setParameter( const QString & _param,
						const QString & _value );


protected slots:
	void openPlugin( void );
	void toggleGUI( void );
	void noteOffAll( void );
	void changeTempo( bpm_t _new_tempo );


protected:
	virtual void paintEvent( QPaintEvent * _pe );


private:
	void closePlugin( void );

	static QPixmap * s_artwork;


	remoteVSTPlugin * m_plugin;
	QMutex m_pluginMutex;


	pixmapButton * m_openPluginButton;
	QPushButton * m_toggleGUIButton;

	QString m_pluginDLL;


} ;


#endif
