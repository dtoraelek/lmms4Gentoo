/*
 * triple_oscillator.cpp - powerful instrument with three oscillators
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 

#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>

#else

#include <qbitmap.h>
#include <qpainter.h>
#include <qdom.h>
#include <qwhatsthis.h>

#define setChecked setOn

#endif


#include "triple_oscillator.h"
#include "song_editor.h"
#include "instrument_track.h"
#include "note_play_handle.h"
#include "knob.h"
#include "buffer_allocator.h"
#include "debug.h"
#include "tooltip.h"
#include "sample_buffer.h"
#include "automatable_button.h"
#include "pixmap_button.h"


#undef SINGLE_SOURCE_COMPILE
#include "embed.cpp"
#include "volume_knob.h"


extern "C"
{

plugin::descriptor tripleoscillator_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"TripleOscillator",
	QT_TRANSLATE_NOOP( "pluginBrowser",
				"three powerful oscillators you can modulate "
				"in several ways" ),
	"Tobias Doerffel <tobydox/at/users.sf.net>",
	0x0100,
	plugin::INSTRUMENT,
	new QPixmap( PLUGIN_NAME::getIconPixmap( "logo" ) )
} ;

}

 
tripleOscillator::tripleOscillator( instrumentTrack * _channel_track ) :
	instrument( _channel_track, &tripleoscillator_plugin_descriptor ),
	m_modulationAlgo1( oscillator::MIX ),
	m_modulationAlgo2( oscillator::MIX ),
	m_modulationAlgo3( oscillator::MIX )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		m_osc[i].m_sampleBuffer = new sampleBuffer( eng() );
	}

#ifdef QT4
	setAutoFillBackground( TRUE );
	QPalette pal;
	pal.setBrush( backgroundRole(),
				PLUGIN_NAME::getIconPixmap( "artwork" ) );
	setPalette( pal );
#else
	setErasePixmap( PLUGIN_NAME::getIconPixmap( "artwork" ) );
#endif

	pixmapButton * pm_osc1_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	pm_osc1_btn->move( 80, 50 );
	pm_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"pm_active" ) );
	pm_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"pm_inactive" ) );
	pm_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( pm_osc1_btn, tr( "use phase modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	pixmapButton * am_osc1_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	am_osc1_btn->move( 120, 50 );
	am_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"am_active" ) );
	am_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	am_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( am_osc1_btn, tr( "use amplitude modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	pixmapButton * mix_osc1_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	mix_osc1_btn->move( 160, 50 );
	mix_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	mix_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( mix_osc1_btn, tr( "mix output of oscillator 1 & 2" ) );

	pixmapButton * sync_osc1_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	sync_osc1_btn->move( 200, 50 );
	sync_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	sync_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( sync_osc1_btn, tr( "synchronize oscillator 1 with "
							"oscillator 2" ) );

	pixmapButton * fm_osc1_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	fm_osc1_btn->move( 330, 50 );
	fm_osc1_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"fm_active" ) );
	fm_osc1_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	fm_osc1_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( fm_osc1_btn, tr( "use frequency modulation for "
					"modulating oscillator 2 with "
					"oscillator 1" ) );

	m_mod1BtnGrp = new automatableButtonGroup( this,
						tr( "Modulation type 1" ),
						eng(), _channel_track );
	m_mod1BtnGrp->addButton( pm_osc1_btn );
	m_mod1BtnGrp->addButton( am_osc1_btn );
	m_mod1BtnGrp->addButton( mix_osc1_btn );
	m_mod1BtnGrp->addButton( sync_osc1_btn );
	m_mod1BtnGrp->addButton( fm_osc1_btn );
	m_mod1BtnGrp->setInitValue( m_modulationAlgo1 );

	connect( m_mod1BtnGrp, SIGNAL( valueChanged( int ) ),
						this, SLOT( mod1Ch( int ) ) );


	pixmapButton * pm_osc2_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	pm_osc2_btn->move( 80, 70 );
	pm_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"pm_active" ) );
	pm_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"pm_inactive" ) );
	pm_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( pm_osc2_btn, tr( "use phase modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	pixmapButton * am_osc2_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	am_osc2_btn->move( 120, 70 );
	am_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"am_active" ) );
	am_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"am_inactive" ) );
	am_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( am_osc2_btn, tr( "use amplitude modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	pixmapButton * mix_osc2_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	mix_osc2_btn->move( 160, 70 );
	mix_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_active" ) );
	mix_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"mix_inactive" ) );
	mix_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( mix_osc2_btn, tr("mix output of oscillator 2 & 3" ) );

	pixmapButton * sync_osc2_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	sync_osc2_btn->move( 200, 70 );
	sync_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_active" ) );
	sync_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"sync_inactive" ) );
	sync_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap(
					"btn_mask" ).createHeuristicMask() ) );
	toolTip::add( sync_osc2_btn, tr( "synchronize oscillator 2 with "
							"oscillator 3" ) );

	pixmapButton * fm_osc2_btn = new pixmapButton( this, NULL, eng(),
									NULL );
	fm_osc2_btn->move( 330, 70 );
	fm_osc2_btn->setActiveGraphic( PLUGIN_NAME::getIconPixmap(
								"fm_active" ) );
	fm_osc2_btn->setInactiveGraphic( PLUGIN_NAME::getIconPixmap(
							"fm_inactive" ) );
	fm_osc2_btn->setMask( QBitmap( PLUGIN_NAME::getIconPixmap( "btn_mask" ).
						createHeuristicMask() ) );
	toolTip::add( fm_osc2_btn, tr( "use frequency modulation for "
					"modulating oscillator 3 with "
					"oscillator 2" ) );

	m_mod2BtnGrp = new automatableButtonGroup( this,
						tr( "Modulation type 2" ),
						eng(), _channel_track );
	m_mod2BtnGrp->addButton( pm_osc2_btn );
	m_mod2BtnGrp->addButton( am_osc2_btn );
	m_mod2BtnGrp->addButton( mix_osc2_btn );
	m_mod2BtnGrp->addButton( sync_osc2_btn );
	m_mod2BtnGrp->addButton( fm_osc2_btn );
	m_mod2BtnGrp->setInitValue( m_modulationAlgo2 );

	connect( m_mod2BtnGrp, SIGNAL( valueChanged( int ) ),
						this, SLOT( mod2Ch( int ) ) );


	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		// reset current m_osc-structure
		m_osc[i].waveShape = oscillator::SIN_WAVE;
		
		// setup volume-knob
		m_osc[i].volKnob = new volumeKnob( knobSmall_17, this, tr(
				"Osc %1 volume" ).arg( i+1 ), eng(),
							_channel_track );
		m_osc[i].volKnob->setData( i );
		m_osc[i].volKnob->move( 6, 104+i*50 );
		m_osc[i].volKnob->setRange( MIN_VOLUME, MAX_VOLUME, 1.0f );
		m_osc[i].volKnob->setInitValue( DEFAULT_VOLUME
							/ NUM_OF_OSCILLATORS );
		m_osc[i].volKnob->setHintText( tr( "Osc %1 volume:" ).arg(
							i+1 ) + " ", "%" );
#ifdef QT4
		m_osc[i].volKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].volKnob,
#endif
			tr( "With this knob you can set the volume of "
				"oscillator %1. When setting a value of 0 the "
				"oscillator is turned off. Otherwise you can "
				"hear the oscillator as loud as you set it "
				"here.").arg( i+1 ) );

		// setup panning-knob
		m_osc[i].panKnob = new knob( knobSmall_17, this,
				tr( "Osc %1 panning" ).arg( i + 1 ), eng(),
							_channel_track );
		m_osc[i].panKnob->setData( i );
		m_osc[i].panKnob->move( 33, 104+i*50 );
		m_osc[i].panKnob->setRange( PANNING_LEFT, PANNING_RIGHT, 1.0f );
		m_osc[i].panKnob->setInitValue( DEFAULT_PANNING );
		m_osc[i].panKnob->setHintText( tr("Osc %1 panning:").arg( i+1 )
						+ " ", "" );
#ifdef QT4
		m_osc[i].panKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].panKnob,
#endif
			tr( "With this knob you can set the panning of the "
				"oscillator %1. A value of -100 means 100% "
				"left and a value of 100 moves oscillator-"
				"output right.").arg( i+1 ) );

		// setup coarse-knob
		m_osc[i].coarseKnob = new knob( knobSmall_17, this,
				tr( "Osc %1 coarse detuning" ).arg( i + 1 ),
							eng(), _channel_track );
		m_osc[i].coarseKnob->setData( i );
		m_osc[i].coarseKnob->move( 66, 104 + i * 50 );
		m_osc[i].coarseKnob->setRange( -2 * NOTES_PER_OCTAVE,
						2 * NOTES_PER_OCTAVE, 1.0f );
		m_osc[i].coarseKnob->setInitValue( 0.0f );
		m_osc[i].coarseKnob->setHintText( tr( "Osc %1 coarse detuning:"
							).arg( i + 1 ) + " ",
						" " + tr( "semitones" ) );
#ifdef QT4
		m_osc[i].coarseKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].coarseKnob,
#endif
			tr( "With this knob you can set the coarse detuning of "
				"oscillator %1. You can detune the oscillator "
				"12 semitones (1 octave) up and down. This is "
				"useful for creating sounds with a chord." ).
				arg( i + 1 ) );

		// setup knob for left fine-detuning
		m_osc[i].fineLKnob = new knob( knobSmall_17, this,
				tr( "Osc %1 fine detuning left" ).arg( i+1 ),
							eng(), _channel_track );
		m_osc[i].fineLKnob->setData( i );
		m_osc[i].fineLKnob->move( 90, 104 + i * 50 );
		m_osc[i].fineLKnob->setRange( -100.0f, 100.0f, 1.0f );
		m_osc[i].fineLKnob->setInitValue( 0.0f );
		m_osc[i].fineLKnob->setHintText( tr( "Osc %1 fine detuning "
							"left:" ).arg( i + 1 )
							+ " ", " " +
							tr( "cents" ) );
#ifdef QT4
		m_osc[i].fineLKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].fineLKnob,
#endif
			tr( "With this knob you can set the fine detuning of "
				"oscillator %1 for the left channel. The fine-"
				"detuning is ranged between -100 cents and "
				"+100 cents. This is useful for creating "
				"\"fat\" sounds." ).arg( i + 1 ) );

		// setup knob for right fine-detuning
		m_osc[i].fineRKnob = new knob( knobSmall_17, this,
						tr( "Osc %1 fine detuning right"
							).arg( i + 1 ),
							eng(), _channel_track );
		m_osc[i].fineRKnob->setData( i );
		m_osc[i].fineRKnob->move( 110, 104 + i * 50 );
		m_osc[i].fineRKnob->setRange( -100.0f, 100.0f, 1.0f );
		m_osc[i].fineRKnob->setInitValue( 0.0f );
		m_osc[i].fineRKnob->setHintText( tr( "Osc %1 fine detuning "
							"right:").arg( i + 1 ) +
						" ", " " + tr( "cents" ) );
#ifdef QT4
		m_osc[i].fineRKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].fineRKnob,
#endif
			tr( "With this knob you can set the fine detuning of "
				"oscillator %1 for the right channel. The "
				"fine-detuning is ranged between -100 cents "
				"and +100 cents. This is useful for creating "
				"\"fat\" sounds." ).arg( i+1 ) );

		// setup phase-offset-knob
		m_osc[i].phaseOffsetKnob = new knob( knobSmall_17, this,
							tr( "Osc %1 phase-"
							"offset" ).arg( i+1 ),
							eng(),
							_channel_track );
		m_osc[i].phaseOffsetKnob->setData( i );
		m_osc[i].phaseOffsetKnob->move( 142, 104 + i * 50 );
		m_osc[i].phaseOffsetKnob->setRange( 0.0f, 360.0f, 1.0f );
		m_osc[i].phaseOffsetKnob->setInitValue( 0.0f );
		m_osc[i].phaseOffsetKnob->setHintText( tr( "Osc %1 phase-"
								"offset:" ).
								arg( i + 1 ) +
						" ", " " + tr( "degrees" ) );
#ifdef QT4
		m_osc[i].phaseOffsetKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].phaseOffsetKnob,
#endif
			tr( "With this knob you can set the phase-offset of "
				"oscillator %1. That means you can move the "
				"point within an oscillation where the "
				"oscillator begins to oscillate. For example "
				"if you have a sine-wave and have a phase-"
				"offset of 180 degrees the wave will first go "
				"down. It's the same with a square-wave."
				).arg( i+1 ) );

		// setup stereo-phase-detuning-knob
		m_osc[i].stereoPhaseDetuningKnob = new knob( knobSmall_17, this,
						tr( "Osc %1 stereo phase-"
							"detuning" ).arg( i+1 ),
							eng(),
							_channel_track );
		m_osc[i].stereoPhaseDetuningKnob->setData( i );
		m_osc[i].stereoPhaseDetuningKnob->move( 166, 104 + i * 50 );
		m_osc[i].stereoPhaseDetuningKnob->setRange( 0.0f, 360.0f,
									1.0f );
		m_osc[i].stereoPhaseDetuningKnob->setInitValue( 0.0f );
		m_osc[i].stereoPhaseDetuningKnob->setHintText( tr("Osc %1 "
								"stereo phase-"
								"detuning:" ).
								arg( i + 1 ) +
								" ", " " +
							tr( "degrees" ) );
#ifdef QT4
		m_osc[i].stereoPhaseDetuningKnob->setWhatsThis(
#else
		QWhatsThis::add( m_osc[i].stereoPhaseDetuningKnob,
#endif
			tr( "With this knob you can set the stereo phase-"
				"detuning of oscillator %1. The stereo phase-"
				"detuning specifies the size of the difference "
				"between the phase-offset of left and right "
				"channel. This is very good for creating wide "
				"stereo-sounds." ).arg( i+1 ) );

		// Connect knobs with oscillators' inputs
		connect( m_osc[i].volKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this, SLOT( updateVolume( const QVariant & ) ) );
		connect( m_osc[i].panKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this, SLOT( updateVolume( const QVariant & ) ) );
		updateVolume( i );

		connect( m_osc[i].coarseKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this, SLOT( updateDetuningLeft( const QVariant & ) ) );
		connect( m_osc[i].coarseKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this, SLOT( updateDetuningRight( const QVariant & ) ) );
		connect( m_osc[i].fineLKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this, SLOT( updateDetuningLeft( const QVariant & ) ) );
		connect( m_osc[i].fineRKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this, SLOT( updateDetuningRight( const QVariant & ) ) );
		updateDetuningLeft( i );
		updateDetuningRight( i );

		connect( m_osc[i].phaseOffsetKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this,
			SLOT( updatePhaseOffsetLeft( const QVariant & ) ) );
		connect( m_osc[i].phaseOffsetKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this,
			SLOT( updatePhaseOffsetRight( const QVariant & ) ) );
		connect( m_osc[i].stereoPhaseDetuningKnob,
				SIGNAL( valueChanged( const QVariant & ) ),
			this,
			SLOT( updatePhaseOffsetLeft( const QVariant & ) ) );
		updatePhaseOffsetLeft( i );
		updatePhaseOffsetRight( i );

		pixmapButton * sin_wave_btn = new pixmapButton( this, NULL,
								eng(), NULL );
		sin_wave_btn->move( 188, 105 + i * 50 );
		sin_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"sin_wave_active" ) );
		sin_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"sin_wave_inactive" ) );
		sin_wave_btn->setChecked( TRUE );
		toolTip::add( sin_wave_btn,
				tr( "Click here if you want a sine-wave for "
						"current oscillator." ) );

		pixmapButton * triangle_wave_btn = new pixmapButton( this, NULL,
								eng(), NULL );
		triangle_wave_btn->move( 203, 105 + i * 50 );
		triangle_wave_btn->setActiveGraphic(
			embed::getIconPixmap( "triangle_wave_active" ) );
		triangle_wave_btn->setInactiveGraphic(
			embed::getIconPixmap( "triangle_wave_inactive" ) );
		toolTip::add( triangle_wave_btn,
				tr( "Click here if you want a triangle-wave "
						"for current oscillator." ) );

		pixmapButton * saw_wave_btn = new pixmapButton( this, NULL,
								eng(), NULL );
		saw_wave_btn->move( 218, 105 + i * 50 );
		saw_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"saw_wave_active" ) );
		saw_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"saw_wave_inactive" ) );
		toolTip::add( saw_wave_btn,
				tr( "Click here if you want a saw-wave for "
						"current oscillator." ) );

		pixmapButton * sqr_wave_btn = new pixmapButton( this, NULL,
								eng(), NULL );
		sqr_wave_btn->move( 233, 105 + i * 50 );
		sqr_wave_btn->setActiveGraphic( embed::getIconPixmap(
						"square_wave_active" ) );
		sqr_wave_btn->setInactiveGraphic( embed::getIconPixmap(
						"square_wave_inactive" ) );
		toolTip::add( sqr_wave_btn,
				tr( "Click here if you want a square-wave for "
						"current oscillator." ) );

		pixmapButton * moog_saw_wave_btn = new pixmapButton( this, NULL,
								eng(), NULL );
		moog_saw_wave_btn->move( 188, 120+i*50 );
		moog_saw_wave_btn->setActiveGraphic(
			embed::getIconPixmap( "moog_saw_wave_active" ) );
		moog_saw_wave_btn->setInactiveGraphic(
			embed::getIconPixmap( "moog_saw_wave_inactive" ) );
		toolTip::add( moog_saw_wave_btn,
				tr( "Click here if you want a moog-saw-wave "
						"for current oscillator." ) );

		pixmapButton * exp_wave_btn = new pixmapButton( this, NULL,
								eng(), NULL );
		exp_wave_btn->move( 203, 120+i*50 );
		exp_wave_btn->setActiveGraphic( embed::getIconPixmap(
							"exp_wave_active" ) );
		exp_wave_btn->setInactiveGraphic( embed::getIconPixmap(
							"exp_wave_inactive" ) );
		toolTip::add( exp_wave_btn,
				tr( "Click here if you want an exponential "
					"wave for current oscillator." ) );

		pixmapButton * white_noise_btn = new pixmapButton( this, NULL,
								eng(), NULL );
		white_noise_btn->move( 218, 120+i*50 );
		white_noise_btn->setActiveGraphic(
			embed::getIconPixmap( "white_noise_wave_active" ) );
		white_noise_btn->setInactiveGraphic(
			embed::getIconPixmap( "white_noise_wave_inactive" ) );
		toolTip::add( white_noise_btn,
				tr( "Click here if you want a white-noise for "
						"current oscillator." ) );

		m_osc[i].usrWaveBtn = new pixmapButton( this, NULL, eng(),
									NULL );
		m_osc[i].usrWaveBtn->move( 233, 120+i*50 );
		m_osc[i].usrWaveBtn->setActiveGraphic( embed::getIconPixmap(
							"usr_wave_active" ) );
		m_osc[i].usrWaveBtn->setInactiveGraphic( embed::getIconPixmap(
							"usr_wave_inactive" ) );
		toolTip::add( m_osc[i].usrWaveBtn,
				tr( "Click here if you want a user-defined "
				"wave-shape for current oscillator." ) );

		m_osc[i].waveBtnGrp = new automatableButtonGroup( this,
					tr( "Osc %1 wave shape" ).arg( i + 1 ),
					eng(), _channel_track );
		m_osc[i].waveBtnGrp->addButton( sin_wave_btn );
		m_osc[i].waveBtnGrp->addButton( triangle_wave_btn );
		m_osc[i].waveBtnGrp->addButton( saw_wave_btn );
		m_osc[i].waveBtnGrp->addButton( sqr_wave_btn );
		m_osc[i].waveBtnGrp->addButton( moog_saw_wave_btn );
		m_osc[i].waveBtnGrp->addButton( exp_wave_btn );
		m_osc[i].waveBtnGrp->addButton( white_noise_btn );
		m_osc[i].waveBtnGrp->addButton( m_osc[i].usrWaveBtn );

		if( i == 0 )
		{
			connect( m_osc[i].waveBtnGrp,
						SIGNAL( valueChanged( int ) ),
				this, SLOT( osc0WaveCh( int ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( doubleClicked() ), this,
					SLOT( osc0UserDefWaveDblClick() ) );
		}
		else if( i == 1 )
		{
			connect( m_osc[i].waveBtnGrp,
						SIGNAL( valueChanged( int ) ),
				this, SLOT( osc1WaveCh( int ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( doubleClicked() ), this,
					SLOT( osc1UserDefWaveDblClick() ) );
		}
		else if( i == 2 )
		{
			connect( m_osc[i].waveBtnGrp,
						SIGNAL( valueChanged( int ) ),
				this, SLOT( osc2WaveCh( int ) ) );
			connect( m_osc[i].usrWaveBtn,
					SIGNAL( doubleClicked() ), this,
					SLOT( osc2UserDefWaveDblClick() ) );
		}
	}

	connect( eng()->getMixer(), SIGNAL( sampleRateChanged() ),
			this, SLOT( updateAllDetuning() ) );
}




tripleOscillator::~tripleOscillator()
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		delete m_osc[i].m_sampleBuffer;
	}
}




void tripleOscillator::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	m_mod1BtnGrp->saveSettings( _doc, _this, "modalgo1" );
	m_mod2BtnGrp->saveSettings( _doc, _this, "modalgo2" );

	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		QString is = QString::number( i );
		m_osc[i].volKnob->saveSettings( _doc, _this, "vol" + is );
		m_osc[i].panKnob->saveSettings( _doc, _this, "pan" + is );
		m_osc[i].coarseKnob->saveSettings( _doc, _this, "coarse" + is );
		m_osc[i].fineLKnob->saveSettings( _doc, _this, "finel" + is );
		m_osc[i].fineRKnob->saveSettings( _doc, _this, "finer" + is );
		m_osc[i].phaseOffsetKnob->saveSettings( _doc, _this,
							"phoffset" + is );
		m_osc[i].stereoPhaseDetuningKnob->saveSettings( _doc, _this,
							"stphdetun" + is );
		m_osc[i].waveBtnGrp->saveSettings( _doc, _this,
							"wavetype" + is );
		_this.setAttribute( "userwavefile" + is,
					m_osc[i].m_sampleBuffer->audioFile() );
	}
}




void tripleOscillator::loadSettings( const QDomElement & _this )
{
	m_mod1BtnGrp->loadSettings( _this, "modalgo1" );
	m_mod2BtnGrp->loadSettings( _this, "modalgo2" );

	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		QString is = QString::number( i );
		m_osc[i].volKnob->loadSettings( _this, "vol" + is );
		m_osc[i].panKnob->loadSettings( _this, "pan" + is );
		m_osc[i].coarseKnob->loadSettings( _this, "coarse" + is );
		m_osc[i].fineLKnob->loadSettings( _this, "finel" + is );
		m_osc[i].fineRKnob->loadSettings( _this, "finer" + is );
		m_osc[i].phaseOffsetKnob->loadSettings( _this,
							"phoffset" + is );
		m_osc[i].stereoPhaseDetuningKnob->loadSettings( _this,
							"stphdetun" + is );
		m_osc[i].m_sampleBuffer->setAudioFile( _this.attribute(
							"userwavefile" + is ) );
		m_osc[i].waveBtnGrp->loadSettings( _this, "wavetype" + is );
	}
}




void tripleOscillator::setParameter( const QString & _param,
							const QString & _value )
{
	if( _param == "samplefile" )
	{
		for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
		{
			m_osc[i].m_sampleBuffer->setAudioFile( _value );
		}
	}
}




QString tripleOscillator::nodeName( void ) const
{
	return( tripleoscillator_plugin_descriptor.name );
}




void tripleOscillator::playNote( notePlayHandle * _n )
{
	if( _n->totalFramesPlayed() == 0 )
	{
		oscillator * oscs_l[NUM_OF_OSCILLATORS];
		oscillator * oscs_r[NUM_OF_OSCILLATORS];

		for( Sint8 i = NUM_OF_OSCILLATORS-1; i >= 0; --i )
		{

			// the third oscs needs no sub-oscs...
			if( i == 2 )
			{
				oscs_l[i] = new oscillator(
						&m_osc[i].waveShape,
						&m_modulationAlgo3,
						&_n->m_frequency,
						&m_osc[i].detuningLeft,
						&m_osc[i].phaseOffsetLeft,
						&m_osc[i].volumeLeft );
				oscs_r[i] = new oscillator(
						&m_osc[i].waveShape,
						&m_modulationAlgo3,
						&_n->m_frequency,
						&m_osc[i].detuningRight,
						&m_osc[i].phaseOffsetRight,
						&m_osc[i].volumeRight );
			}
			else
			{
				oscs_l[i] = new oscillator(
						&m_osc[i].waveShape,
						getModulationAlgo( i + 1 ),
						&_n->m_frequency,
						&m_osc[i].detuningLeft,
						&m_osc[i].phaseOffsetLeft,
						&m_osc[i].volumeLeft,
							oscs_l[i + 1] );
				oscs_r[i] = new oscillator(
						&m_osc[i].waveShape,
						getModulationAlgo( i + 1 ),
						&_n->m_frequency,
						&m_osc[i].detuningRight,
						&m_osc[i].phaseOffsetRight,
						&m_osc[i].volumeRight,
								oscs_r[i + 1] );
			}

			oscs_l[i]->setUserWave( m_osc[i].m_sampleBuffer );
			oscs_r[i]->setUserWave( m_osc[i].m_sampleBuffer );

		}

		_n->m_pluginData = new oscPtr;
		static_cast<oscPtr *>( _n->m_pluginData )->oscLeft = oscs_l[0];
		static_cast< oscPtr *>( _n->m_pluginData )->oscRight =
								oscs_r[0];
	}

	oscillator * osc_l = static_cast<oscPtr *>( _n->m_pluginData )->oscLeft;
	oscillator * osc_r = static_cast<oscPtr *>( _n->m_pluginData
								)->oscRight;

	const fpab_t frames = eng()->getMixer()->framesPerAudioBuffer();
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );
	
	osc_l->update( buf, frames, 0 );
	osc_r->update( buf, frames, 1 );

	getInstrumentTrack()->processAudioBuffer( buf, frames, _n );

	bufferAllocator::free( buf );
}




void tripleOscillator::deleteNotePluginData( notePlayHandle * _n )
{
	if( _n->m_pluginData == NULL )
	{
		return;
	}
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscLeft );
	delete static_cast<oscillator *>( static_cast<oscPtr *>(
						_n->m_pluginData )->oscRight );
	delete static_cast<oscPtr *>( _n->m_pluginData );
}




void tripleOscillator::osc0WaveCh( int _n )
{
	m_osc[0].waveShape = static_cast<oscillator::waveShapes>( _n );
}




void tripleOscillator::osc1WaveCh( int _n )
{
	m_osc[1].waveShape = static_cast<oscillator::waveShapes>( _n );
}




void tripleOscillator::osc2WaveCh( int _n )
{
	m_osc[2].waveShape = static_cast<oscillator::waveShapes>( _n );
}




void tripleOscillator::mod1Ch( int _n )
{
	m_modulationAlgo1 = static_cast<oscillator::modulationAlgos>( _n );
}




void tripleOscillator::mod2Ch( int _n )
{
	m_modulationAlgo2 = static_cast<oscillator::modulationAlgos>( _n );
}




void tripleOscillator::osc0UserDefWaveDblClick( void )
{
	QString af = m_osc[0].m_sampleBuffer->openAudioFile();
	if( af != "" )
	{
		m_osc[0].m_sampleBuffer->setAudioFile( af );
		toolTip::add( m_osc[0].usrWaveBtn,
					m_osc[0].m_sampleBuffer->audioFile() );
	}
}



void tripleOscillator::osc1UserDefWaveDblClick( void )
{
	QString af = m_osc[1].m_sampleBuffer->openAudioFile();
	if( af != "" )
	{
		m_osc[1].m_sampleBuffer->setAudioFile( af );
		toolTip::add( m_osc[1].usrWaveBtn,
					m_osc[1].m_sampleBuffer->audioFile() );
	}
}



void tripleOscillator::osc2UserDefWaveDblClick( void )
{
	QString af = m_osc[2].m_sampleBuffer->openAudioFile();
	if( af != "" )
	{
		m_osc[2].m_sampleBuffer->setAudioFile( af );
		toolTip::add( m_osc[2].usrWaveBtn,
					m_osc[2].m_sampleBuffer->audioFile() );
	}
}




void tripleOscillator::updateVolume( const QVariant & _data )
{
	const int _i = _data.toInt();
	float panningFactorLeft;
	float panningFactorRight;

	if( m_osc[_i].panKnob->value() >= 0.0f )
	{
		panningFactorLeft = 1.0f - m_osc[_i].panKnob->value()
						/ (float)PANNING_RIGHT;
		panningFactorRight = 1.0f;
	}
	else
	{
		panningFactorLeft = 1.0f;
		panningFactorRight = 1.0f + m_osc[_i].panKnob->value()
						/ (float)PANNING_RIGHT;
	}

	m_osc[_i].volumeLeft = panningFactorLeft * m_osc[_i].volKnob->value()
								/ 100.0f;
	m_osc[_i].volumeRight = panningFactorRight * m_osc[_i].volKnob->value()
								/ 100.0f;
}




void tripleOscillator::updateDetuningLeft( const QVariant & _data )
{
	const int _i = _data.toInt();
	m_osc[_i].detuningLeft = powf( 2.0f, (
			(float)m_osc[_i].coarseKnob->value() * 100.0f +
			(float)m_osc[_i].fineLKnob->value() ) / 1200.0f )
			/ static_cast<float>( eng()->getMixer()->sampleRate() );
}




void tripleOscillator::updateDetuningRight( const QVariant & _data )
{
	const int _i = _data.toInt();
	m_osc[_i].detuningRight = powf( 2.0f, (
			(float)m_osc[_i].coarseKnob->value() * 100.0f +
			(float)m_osc[_i].fineRKnob->value() ) / 1200.0f )
			/ static_cast<float>( eng()->getMixer()->sampleRate() );
}




void tripleOscillator::updateAllDetuning( void )
{
	for( int i = 0; i < NUM_OF_OSCILLATORS; ++i )
	{
		updateDetuningLeft( i );
		updateDetuningRight( i );
	}
}




void tripleOscillator::updatePhaseOffsetLeft( const QVariant & _data )
{
	const int _i = _data.toInt();
	m_osc[_i].phaseOffsetLeft = ( m_osc[_i].phaseOffsetKnob->value() +
			m_osc[_i].stereoPhaseDetuningKnob->value() ) / 360.0f;
}




void tripleOscillator::updatePhaseOffsetRight( const QVariant & _data )
{
	const int _i = _data.toInt();
	m_osc[_i].phaseOffsetRight = m_osc[_i].phaseOffsetKnob->value()
								/ 360.0f;
}




oscillator::modulationAlgos * tripleOscillator::getModulationAlgo( int _n )
{
	if( _n == 1 )
	{
		return( &m_modulationAlgo1 );
	}
	else
	{
		return( &m_modulationAlgo2 );
	}
}





extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new tripleOscillator(
				static_cast<instrumentTrack *>( _data ) ) );
}

}


#undef setChecked


#include "triple_oscillator.moc"

