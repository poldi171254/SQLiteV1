/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "progressslider.h"

#include "amarokconfig.h"
#include "debug.h"
#include "enginecontroller.h"
#include "meta/meta.h"
#include "meta/MetaUtility.h"
#include "timeLabel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPolygon>

#include <khbox.h>
#include <klocale.h>
#include <kpassivepopup.h>

//Class ProgressWidget
ProgressWidget *ProgressWidget::s_instance = 0;
ProgressWidget::ProgressWidget( QWidget *parent )
    : QWidget( parent )
    , EngineObserver( EngineController::instance() )
    , m_timeLength( 0 )
{
    s_instance = this;

    QHBoxLayout *box = new QHBoxLayout( this );
    setLayout( box );
    box->setMargin( 1 );
    box->setSpacing( 3 );

    m_slider = new Amarok::Slider( Qt::Horizontal, this );
    m_slider->setMouseTracking( true );
    // the two time labels. m_timeLabel is the left one,
    // m_timeLabel2 the right one.
    m_timeLabel = new TimeLabel( this );
    m_timeLabel->setToolTip( i18n( "The amount of time elapsed in current song" ) );

    m_timeLabel2 = new TimeLabel( this );
    m_timeLabel->setToolTip( i18n( "The amount of time remaining in current song" ) );

    m_timeLabel->hide();
    m_timeLabel2->hide();

    box->addSpacing( 3 );
    box->addWidget( m_timeLabel );
    box->addWidget( m_slider );
    box->addWidget( m_timeLabel2 );
#ifdef Q_WS_MAC
    // don't overlap the resize handle with the time display
    box->addSpacing( 12 );
#endif

    if( !AmarokConfig::leftTimeDisplayEnabled() )
        m_timeLabel->hide();

    engineStateChanged( Engine::Empty );

    connect( m_slider, SIGNAL(sliderReleased( int )), EngineController::instance(), SLOT(seek( int )) );
    connect( m_slider, SIGNAL(valueChanged( int )), SLOT(drawTimeDisplay( int )) );

}

void
ProgressWidget::drawTimeDisplay( int ms )  //SLOT
{
    int seconds = ms / 1000;
    int seconds2 = seconds; // for the second label.
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( !track )
        return;
    const uint trackLength = track->length();

    if( AmarokConfig::leftTimeDisplayEnabled() )
        m_timeLabel->show();
    else
        m_timeLabel->hide();

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = seconds;
        seconds = trackLength - seconds;
    // when the left label shows the remaining time and it's a stream
    } else if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = seconds;
        seconds = 0; // for streams
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        seconds2 = trackLength - seconds;
    // when the right label shows the remaining time and it's a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        seconds2 = 0;
    }

    //put Utility functions somewhere
    QString s1 = Meta::secToPrettyTime( seconds );
    QString s2 = Meta::secToPrettyTime( seconds2 );

    // when the left label shows the remaining time and it's not a stream
    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 ) {
        s1.prepend( '-' );
    // when the right label shows the remaining time and it's not a stream
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength > 0 )
    {
        s2.prepend( '-' );
    }

    if( m_timeLength > s1.length() )
        s1.prepend( QString( m_timeLength - s1.length(), ' ' ) );

    if( m_timeLength > s2.length() )
        s2.prepend( QString( m_timeLength - s2.length(), ' ' ) );

    s1 += ' ';
    s2 += ' ';

    m_timeLabel->setText( s1 );
    m_timeLabel2->setText( s2 );

    if( AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabel->setEnabled( false );
        m_timeLabel2->setEnabled( true );
    } else if( !AmarokConfig::leftTimeDisplayRemaining() && trackLength == 0 )
    {
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( false );
    } else
    {
        m_timeLabel->setEnabled( true );
        m_timeLabel2->setEnabled( true );
    }
}

void
ProgressWidget::engineTrackPositionChanged( long position, bool /*userSeek*/ )
{
    m_slider->setValue( position );

    if ( !m_slider->isEnabled() )
        drawTimeDisplay( position );
}

void
ProgressWidget::engineStateChanged( Engine::State state, Engine::State /*oldState*/ )
{
    switch ( state ) {
        case Engine::Empty:
            m_slider->setEnabled( false );
            m_slider->setMinimum( 0 ); //needed because setMaximum() calls with bogus values can change minValue
            m_slider->setMaximum( 0 );
//             m_timeLabel->setEnabled( false ); //must be done after the setValue() above, due to a signal connection
//             m_timeLabel2->setEnabled( false );
            m_timeLabel->hide();
            m_timeLabel2->hide();
            break;

        case Engine::Playing:
            m_timeLabel->show();
            m_timeLabel2->show();
        case Engine::Paused:
            DEBUG_LINE_INFO
            m_timeLabel->setEnabled( true );
            m_timeLabel2->setEnabled( true );
            break;

        case Engine::Idle:
            ; //just do nothing, idle is temporary and a limbo state
    }
}

void
ProgressWidget::engineTrackLengthChanged( long seconds )
{
    m_slider->setMinimum( 0 );
    m_slider->setMaximum( seconds * 1000 );
    m_slider->setEnabled( seconds > 0 );
    m_timeLength = Meta::secToPrettyTime( seconds ).length()+1; // account for - in remaining time
}

void
ProgressWidget::engineNewTrackPlaying()
{
    Meta::TrackPtr track = EngineController::instance()->currentTrack();
    if( !track )
        return;
    engineTrackLengthChanged( track->length() );
}

#include "progressslider.moc"
