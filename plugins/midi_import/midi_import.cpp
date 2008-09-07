/*
 * midi_import.cpp - support for importing MIDI-files
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtXml/QDomDocument>
#include <QtCore/QDir>
#include <QtGui/QApplication>
#include <QtGui/QProgressDialog>

#include "midi_import.h"
#include "track_container.h"
#include "instrument_track.h"
#include "pattern.h"
#include "automation_pattern.h"
#include "instrument.h"
#include "debug.h"
#include "embed.h"
#include "song.h"


#define makeID(_c0, _c1, _c2, _c3) \
		( 0 | \
		( ( _c0 ) | ( ( _c1 ) << 8 ) | ( ( _c2 ) << 16 ) | ( ( _c3 ) << 24 ) ) )



extern "C"
{

plugin::descriptor PLUGIN_EXPORT midiimport_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"MIDI Import",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"Filter for importing MIDI-files into LMMS" ),
	"Tobias Doerffel <tobydox/at/users/dot/sf/dot/net>",
	0x0100,
	plugin::ImportFilter,
	NULL,
	NULL,
	NULL
} ;

}


midiImport::midiImport( const QString & _file ) :
	importFilter( _file, &midiimport_plugin_descriptor ),
	m_events(),
	m_timingDivision( 0 )
{
}




midiImport::~midiImport()
{
}




bool midiImport::tryImport( trackContainer * _tc )
{
	if( openFile() == FALSE )
	{
		return( FALSE );
	}

	switch( readID() )
	{
		case makeID( 'M', 'T', 'h', 'd' ):
			printf( "midiImport::tryImport(): found MThd\n");
			return( readSMF( _tc ) );

		case makeID( 'R', 'I', 'F', 'F' ):
			printf( "midiImport::tryImport(): found RIFF\n");
			return( readRIFF( _tc ) );

		default:
			printf( "midiImport::tryImport(): not a Standard MIDI "
								"file\n" );
			return( FALSE );
	}
}




static inline unsigned long gcd( unsigned long a, unsigned long b )
{
	unsigned long r;
	if( a < b )
	{
		r = a;
		a = b;
		b = r;
	}
	while( ( r = a % b ) != 0 )
	{
		a = b;
		b = r;
	}
	return b;
}




bool midiImport::readSMF( trackContainer * _tc )
{
	// the curren position is immediately after the "MThd" id
	int header_len = readInt( 4 );
	if( header_len < 6 )
	{
invalid_format:
		printf( "midiImport::readSMF(): invalid file format\n" );
		return( FALSE );
	}

	int type = readInt( 2 );
	if( type != 0 && type != 1 )
	{
		printf( "midiImport::readSMF(): type %d format is not "
							"supported\n", type );
		return( FALSE );
	}

	int num_tracks = readInt( 2 );
	if( num_tracks < 1 || num_tracks > 1000 )
	{
		printf( "midiImport::readSMF(): invalid number of tracks (%d)\n",
								num_tracks );
		num_tracks = 0;
		return( FALSE );
	}
#ifdef LMMS_DEBUG
	printf( "tracks: %d\n", num_tracks );
#endif

	m_timingDivision = readInt( 2 );
	if( m_timingDivision < 0 )
	{
		goto invalid_format;
	}
#ifdef LMMS_DEBUG
    printf( "time-division: %d\n", m_timingDivision );
#endif

	QProgressDialog pd( trackContainer::tr( "Importing MIDI-file..." ),
				trackContainer::tr( "Cancel" ), 0, num_tracks );
	pd.setWindowTitle( trackContainer::tr( "Please wait..." ) );
	pd.show();

	// calculate some timing stuff
	int crotchet_time = 16*3;
	int divisor = m_timingDivision ? m_timingDivision : 96;
	int multiplier = crotchet_time;
	int g = gcd( crotchet_time, divisor );
	multiplier /= g;
	divisor /= g;

	// try to set default tempo
	automationPattern * tap = _tc->tempoAutomationPattern();
	tap->clear();
	tap->putValue( 0, 120 );

        // read tracks
	for( int i = 0; i < num_tracks; ++i )
	{
		pd.setValue( i );
		qApp->processEvents( QEventLoop::AllEvents, 100 );

		if( pd.wasCanceled() )
		{
			return( FALSE );
		}

		int len;

                // search for MTrk chunk
		while( 1 )
		{
			Sint32 id = readID();
			len = readInt( 4 );
			if( file().atEnd() )
			{
				printf( "midiImport::readSMF(): unexpected end "
								"of file\n" );
				return( FALSE );
			}
			if( len < 0 || len >= 0x10000000 )
			{
				printf( "midiImport::readSMF(): invalid chunk "
							"length %d\n", len );
				return( FALSE );
			}
			if( id == makeID( 'M', 'T', 'r', 'k' ) )
			{
				break;
			}
			skip( len );
		}
		if( len <= 0 )
		{
			continue;
		}

		QString track_name = "";

		if( !readTrack( file().pos() + len, track_name ) )
		{
			return( FALSE );
		}
		if( i == 0 )
		{
			if( tap == NULL )
			{
				continue;
			}
			for( eventVector::const_iterator it = m_events.begin();
							it != m_events.end(); ++it )
			{
				const int tick = it->first;
				const midiEvent & ev = it->second;
				if( ev.m_type == MidiMetaEvent )
				{
					switch( ev.m_data.m_param[0] )
					{
						case MidiSetTempo:
						{
	tap->putValue( midiTime( ( tick * multiplier ) / divisor ),
						ev.m_data.m_param[1], FALSE );
							break;
						}
						default:
							break;
					}
				}
			}
			continue;
		}

		// now create new instrument-track for reading in track
		instrumentTrack * it = dynamic_cast<instrumentTrack *>(
						track::create(
							track::InstrumentTrack,
							_tc ) );
#ifdef LMMS_DEBUG
		assert( it != NULL );
#endif
		// TODO: setup program, channel etc.
		instrument * it_inst = it->loadInstrument( "patman" );
		bool sample_loaded = FALSE;

		// TODO: track_name.trimmed().isEmpty() (Qt4)
		if( !track_name.isEmpty() )
		{
			it->setName( track_name );
		}

		// now create pattern to store notes in
		pattern * p = dynamic_cast<pattern *>( it->createTCO( 0 ) );
#ifdef LMMS_DEBUG
		assert( p != NULL );
#endif
		it->addTCO( p );

		// init keys
		int keys[NumKeys][2];
		for( int j = 0; j < NumKeys; ++j )
		{
			keys[j][0] = -1;
		}

		// now process every event
		for( eventVector::const_iterator it = m_events.begin();
						it != m_events.end(); ++it )
		{
			const int tick = it->first;
			const midiEvent & ev = it->second;
			switch( ev.m_type )
			{
				case MidiNoteOn:
					if( ev.key() >= NumKeys )
					{
						continue;
					}
					if( ev.velocity() > 0 )
					{
						keys[ev.key()][0] = tick;
						keys[ev.key()][1] =
								ev.velocity();
						break;
					}

				case MidiNoteOff:
					if( ev.key() < NumKeys &&
							keys[ev.key()][0] >= 0 )
					{
			note n( midiTime( ( ( tick - keys[ev.key()][0] ) * multiplier ) / divisor ),
				midiTime( ( keys[ev.key()][0] * multiplier ) / divisor ),
				ev.key(), keys[ev.key()][1] * 100 / 128 );
						p->addNote( n, FALSE );
						keys[ev.key()][0] = -1;
					}
					break;

				case MidiProgramChange:
				{
					const QString num = QString::number( ev.key() );
					const QString filter = QString().fill( '0', 3 - num.length() ) + num + "*.pat";
					const QString dir = "/usr/share/midi/"
							"freepats/Tone_000/";
					const QStringList files = QDir( dir ).
						entryList( QStringList(
								filter ) );
					if( it_inst && !files.empty() && !sample_loaded )
					{
						it_inst->loadFile( dir+files.front() );
						sample_loaded = TRUE;
					}
					break;
				}

				default:
				/*	printf( "Unhandled event: %#x\n",
								(int) ev.m_type );*/
					break;
			}
		}
        }
        return( TRUE );
}




bool midiImport::readRIFF( trackContainer * _tc )
{
        // skip file length
	skip( 4 );

        // check file type ("RMID" = RIFF MIDI)
        if( readID() != makeID( 'R', 'M', 'I', 'D' ) )
	{
invalid_format:
                printf( "midiImport::readRIFF(): invalid file format\n" );
                return( FALSE );
        }
        // search for "data" chunk
        while( 1 )
	{
                int id = readID();
                int len = read32LE();
                if( file().atEnd() )
		{
data_not_found:
                        printf( "midiImport::readRIFF(): data chunk not "
								"found\n" );
                        return( FALSE );
                }
                if( id == makeID( 'd', 'a', 't', 'a' ) )
		{
                        break;
		}
                if( len < 0 )
		{
                        goto data_not_found;
		}
                skip( ( len + 1 ) & ~1 );
        }
        // the "data" chunk must contain data in SMF format
        if( readID() != makeID( 'M', 'T', 'h', 'd' ) )
	{
                goto invalid_format;
	}
        return( readSMF( _tc ) );
}




bool midiImport::readTrack( int _track_end, QString & _track_name )
{
        int tick = 0;
        unsigned char last_cmd = 0;
//        unsigned char port = 0;

	m_events.clear();
        // the current file position is after the track ID and length
        while( (int) file().pos() < _track_end )
	{
		unsigned char cmd;
		int len;

		int delta_ticks = readVar();
		if( delta_ticks < 0 )
		{
			break;
		}
		tick += delta_ticks;

		int c = readByte();
		if( c < 0 )
		{
			break;
		}
		if( c & 0x80 )
		{
			// have command
			cmd = c;
			if( cmd < 0xf0 )
			{
				last_cmd = cmd;
			}
		}
		else
		{
			// running status
			ungetChar( c );
			cmd = last_cmd;
			if( !cmd )
			{
				error();
				return( FALSE );
			}
		}
                switch( cmd & 0xF0 )
		{
			// channel msg with 2 parameter bytes
			case MidiNoteOff:
			case MidiNoteOn:
			case MidiKeyPressure:
			case MidiControlChange:
			case MidiPitchBend:
			{
				int data1 = readByte() & 0x7F;
				int data2 = readByte() & 0x7F;
				m_events.push_back( qMakePair( tick,
					midiEvent( static_cast<MidiEventTypes>(
								cmd & 0xF0 ),
							cmd & 0x0F,
							data1,
							data2 ) ) );
				break;
			}
			// channel msg with 1 parameter byte
			case MidiProgramChange:
			case MidiChannelPressure:
				m_events.push_back( qMakePair( tick,
					midiEvent( static_cast<MidiEventTypes>(
								cmd & 0xF0 ),
							cmd & 0x0F,
							readByte() & 0x7F ) ) );
				break;

			case MidiSysEx:
				switch( cmd )
				{
					case MidiSysEx:
					case MidiEOX:
					{
						len = readVar();
						if( len < 0 )
						{
							error();
							return( FALSE );
						}
						if( cmd == MidiSysEx )
						{
							++len;
						}
						char * data = new char[len];
						if( cmd == MidiSysEx )
						{
							data[0] = MidiSysEx;
						}
						for( ; c < len; ++c )
						{
							data[c] = readByte();
						}
						m_events.push_back(
							qMakePair( tick,
				midiEvent( MidiSysEx, data, len ) ) );
						break;
					}

					case MidiMetaEvent:
						c = readByte();
						len = readVar();
/*						if( len < 0 )
						{
							error();
							return( FALSE );
						}*/
						switch( c )
						{
						case MidiTrackName:
							if( len > 0 )
							{
								char * n = new char[len+1];
								readBlock( n, len );
								n[len] = 0;
								_track_name += n;
								delete[] n;
							}
							break;
						case MidiPortNumber:
							if( len < 1 )
							{
								error();
								return( FALSE );
							}
/*							port = readByte() %
								port_count;
							skip( len - 1 );*/
							skip( len );
							break;

						case MidiEOT:
						//track->end_tick = tick;
							skip( _track_end -
								file().pos() );
							return( TRUE );

						case MidiSetTempo: // tempo
						{
							if( len < 3 )
							{
								error();
								return( FALSE );
							}
							int tempo = readByte() << 16;
							tempo |= readByte() << 8;
							tempo |= readByte();
							tempo = ( 60*1000*1000 ) / tempo;
	m_events.push_back( qMakePair( tick, midiEvent( MidiMetaEvent, 0, MidiSetTempo, tempo ) ) );
							break;
						}
						case MidiTimeSignature:
						{
							int nominator = readByte();
							int denominator = 1 << (int) readByte();
							// clocks
							readByte();
							// notes
							readByte();
							if( nominator == 0 )
							{
								nominator = 4;
							}
							if( denominator == 0 )
							{
								denominator = 4;
							}
							engine::getSong()->getTimeSigModel().setNumerator( nominator );
							engine::getSong()->getTimeSigModel().setDenominator( denominator );
#ifdef LMMS_DEBUG
							printf("nom:%d  denom:%d\n",nominator,denominator);
#endif
							break;
						}
						default:// ignore all other
							// meta events
#ifdef LMMS_DEBUG
							printf("meta event %d\n", (int) c ); 
#endif
							skip( len );
							break;
						}
						break;

					default: // invalid Fx command
						error();
						return( FALSE );
				}
				break;

			default: // cannot happen
                	        error();
				return( FALSE );
                }
        }
	error();
	return( FALSE );
}




void midiImport::error( void )
{
	printf( "midiImport::readTrack(): invalid MIDI data (offset %#x)\n",
						(unsigned int) file().pos() );
}



extern "C"
{

// neccessary for getting instance out of shared lib
plugin * PLUGIN_EXPORT lmms_plugin_main( model *, void * _data )
{
	return( new midiImport( static_cast<const char *>( _data ) ) );
}


}


#undef pos
#undef setValue
