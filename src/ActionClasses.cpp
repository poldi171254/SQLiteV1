/***************************************************************************
 *   Copyright (C) 2004 by Max Howell <max.howell@methylblue.com>          *
 *   Copyright (C) 2008 by Mark Kretschmann <kretschmann@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ActionClasses.h"

#include <config-amarok.h>               //HAVE_LIBVISUAL definition

#include "Amarok.h"
#include "amarokconfig.h"
#include "App.h"
#include "Debug.h"
#include "covermanager/CoverManager.h"
#include "EngineController.h"
#include "k3bexporter.h"
#include "MainWindow.h"
#include "playlist/PlaylistModel.h"
//#include "mediumPluginManager.h"

#include "socketserver.h"       //Vis::Selector::showInstance()
#include "StatusBar.h"

#include <KAuthorized>
#include <KHBox>
#include <KHelpMenu>
#include <KLineEdit>
#include <KLocale>
#include <KPushButton>
#include <KToolBar>
#include <KUrl>

#include <QPixmap>
#include <QToolTip>


namespace Amarok
{
    bool repeatNone()     { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Off; }
    bool repeatTrack()    { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Track; }
    bool repeatAlbum()    { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Album; }
    bool repeatPlaylist() { return AmarokConfig::repeat() == AmarokConfig::EnumRepeat::Playlist; }
    bool randomOff()      { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Off; }
    bool randomTracks()   { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Tracks; }
    bool randomAlbums()   { return AmarokConfig::randomMode() == AmarokConfig::EnumRandomMode::Albums; }
    bool favorNone()      { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::Off; }
    bool favorScores()    { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::HigherScores; }
    bool favorRatings()   { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::HigherRatings; }
    bool favorLastPlay()  { return AmarokConfig::favorTracks() == AmarokConfig::EnumFavorTracks::LessRecentlyPlayed; }

    bool entireAlbums()   { return repeatAlbum()  || randomAlbums(); }
    bool repeatEnabled()  { return repeatTrack()  || repeatAlbum() || repeatPlaylist(); }
    bool randomEnabled()  { return randomTracks() || randomAlbums(); }
}

using namespace Amarok;

KHelpMenu *Menu::s_helpMenu = 0;

static void
safePlug( KActionCollection *ac, const char *name, QWidget *w )
{
    if( ac )
    {
        KAction *a = (KAction*) ac->action( name );
        if( a && w ) w->addAction( a );
    }
}


//////////////////////////////////////////////////////////////////////////////////////////
// MenuAction && Menu
// KActionMenu doesn't work very well, so we derived our own
//////////////////////////////////////////////////////////////////////////////////////////

MenuAction::MenuAction( KActionCollection *ac )
  : KAction( 0 )
{
    setText(i18n( "Amarok Menu" ));
    ac->addAction("amarok_menu", this);
    setShortcutConfigurable ( false ); //FIXME disabled as it doesn't work, should use QCursor::pos()
}


K_GLOBAL_STATIC( Menu, s_menu )

Menu::Menu()
{
    qAddPostRoutine( s_menu.destroy );  // Ensure that the dtor gets called when QCoreApplication destructs
    
    KActionCollection *ac = Amarok::actionCollection();

    safePlug( ac, "repeat", this );
    safePlug( ac, "random_mode", this );

    addSeparator();

    safePlug( ac, "playlist_playmedia", this );
    safePlug( ac, "play_audiocd", this );
    safePlug( ac, "lastfm_play", this );

    addSeparator();

    safePlug( ac, "cover_manager", this );
    safePlug( ac, "queue_manager", this );
    safePlug( ac, "visualizations", this );
    safePlug( ac, "equalizer", this );
    safePlug( ac, "script_manager", this );
    safePlug( ac, "statistics", this );

    addSeparator();

    safePlug( ac, "update_collection", this );
    safePlug( ac, "rescan_collection", this );

#ifndef Q_WS_MAC
    addSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::ShowMenubar), this );
#endif

    addSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::ConfigureToolbars), this );
    safePlug( ac, KStandardAction::name(KStandardAction::KeyBindings), this );
//    safePlug( ac, "options_configure_globals", this ); //we created this one
    safePlug( ac, KStandardAction::name(KStandardAction::Preferences), this );

    addSeparator();

    addMenu( helpMenu( this ) );

    addSeparator();

    safePlug( ac, KStandardAction::name(KStandardAction::Quit), this );

    #ifdef HAVE_LIBVISUAL
    Amarok::actionCollection()->action( "visualizations" )->setEnabled( false );
    #endif
}

Menu*
Menu::instance()
{
    return s_menu;
}

KMenu*
Menu::helpMenu( QWidget *parent ) //STATIC
{
    extern KAboutData aboutData;

    if ( s_helpMenu == 0 )
        s_helpMenu = new KHelpMenu( parent, &aboutData, Amarok::actionCollection() );

    return s_helpMenu->menu();
}

//////////////////////////////////////////////////////////////////////////////////////////
// PlayPauseAction
//////////////////////////////////////////////////////////////////////////////////////////

PlayPauseAction::PlayPauseAction( KActionCollection *ac )
        : KToggleAction( 0 )
        , EngineObserver( The::engineController() )
{
    setObjectName( "play-pause" );
    setText( i18n( "Play/Pause" ) );
    setShortcut( Qt::Key_Space );
    setGlobalShortcut( KShortcut( Qt::META + Qt::Key_C ) );
    ac->addAction( "play_pause", this );
    PERF_LOG( "PlayPauseAction: before engineStateChanged" )
    engineStateChanged( The::engineController()->state() );
    PERF_LOG( "PlayPauseAction: after engineStateChanged" )

    connect( this, SIGNAL(triggered()), The::engineController(), SLOT(playPause()) );
}

void
PlayPauseAction::engineStateChanged( Phonon::State state,  Phonon::State /*oldState*/ )
{
    switch( state ) {
    case Phonon::PlayingState:
        setChecked( false );
        setIcon( KIcon("media-playback-pause-amarok") );
        break;
    case Phonon::PausedState:
        setChecked( true );
        setIcon( KIcon("media-playback-pause-amarok") );
        break;
    case Phonon::StoppedState:
    case Phonon::LoadingState:
        setChecked( false );
        setIcon( KIcon("media-playback-start-amarok") );
        break;
    case Phonon::ErrorState:
    case Phonon::BufferingState:
        break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
// ToggleAction
//////////////////////////////////////////////////////////////////////////////////////////

ToggleAction::ToggleAction( const QString &text, void ( *f ) ( bool ), KActionCollection* const ac, const char *name )
        : KToggleAction( 0 )
        , m_function( f )
{
    setText(text);
    ac->addAction(name, this);
}

void ToggleAction::setChecked( bool b )
{
    const bool announce = b != isChecked();

    m_function( b );
    KToggleAction::setChecked( b );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit toggled( b ); //KToggleAction doesn't do this for us. How gay!
}

void ToggleAction::setEnabled( bool b )
{
    const bool announce = b != isEnabled();

    if( !b )
        setChecked( false );
    KToggleAction::setEnabled( b );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit QAction::triggered( b );
}

//////////////////////////////////////////////////////////////////////////////////////////
// SelectAction
//////////////////////////////////////////////////////////////////////////////////////////

SelectAction::SelectAction( const QString &text, void ( *f ) ( int ), KActionCollection* const ac, const char *name )
        : KSelectAction( 0 )
        , m_function( f )
{
    PERF_LOG( "In SelectAction" );
    setText(text);
    ac->addAction(name, this);
}

void SelectAction::setCurrentItem( int n )
{
    const bool announce = n != currentItem();

    debug() << "setCurrentItem: " << n;

    m_function( n );
    KSelectAction::setCurrentItem( n );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit triggered( n );
}

void SelectAction::actionTriggered( QAction *a )
{
    m_function( currentItem() );
    AmarokConfig::self()->writeConfig();
    KSelectAction::actionTriggered( a );
}

void SelectAction::setEnabled( bool b )
{
    const bool announce = b != isEnabled();

    if( !b )
        setCurrentItem( 0 );
    KSelectAction::setEnabled( b );
    AmarokConfig::self()->writeConfig(); //So we don't lose the setting when crashing
    if( announce ) emit QAction::triggered( b );
}

void SelectAction::setIcons( QStringList icons )
{
    m_icons = icons;
    foreach( QAction *a, selectableActionGroup()->actions() )
    {
        a->setIcon( KIcon(icons.takeFirst()) );
    }
}

QStringList SelectAction::icons() const { return m_icons; }

QString SelectAction::currentIcon() const
{
    if( m_icons.count() )
        return m_icons.at( currentItem() );
    return QString();
}

QString SelectAction::currentText() const {
    return KSelectAction::currentText() + "<br /><br />" + i18n("Click to change");
}

//////////////////////////////////////////////////////////////////////////////////////////
// RandomAction
//////////////////////////////////////////////////////////////////////////////////////////
RandomAction::RandomAction( KActionCollection *ac ) :
    SelectAction( i18n( "Ra&ndom" ), &AmarokConfig::setRandomMode, ac, "random_mode" )
{
    QStringList items;
    items << i18nc( "State, as in disabled", "&Off" )
          << i18nc( "Items, as in music"   , "&Tracks" )
          << i18n( "&Albums" );
    setItems( items );

    setCurrentItem( AmarokConfig::randomMode() );

    QStringList icons;
    icons << "media-playlist-shuffle-off-amarok"
          << "media-playlist-shuffle-amarok"
          << "media-album-shuffle-amarok";
    setIcons( icons );

    connect( this, SIGNAL( triggered( int ) ), The::playlistModel(), SLOT( playlistModeChanged() ) );
}

void
RandomAction::setCurrentItem( int n )
{
    // Porting
    //if( KAction *a = parentCollection()->action( "favor_tracks" ) )
    //    a->setEnabled( n );
    SelectAction::setCurrentItem( n );
}


//////////////////////////////////////////////////////////////////////////////////////////
// FavorAction
//////////////////////////////////////////////////////////////////////////////////////////
FavorAction::FavorAction( KActionCollection *ac ) :
    SelectAction( i18n( "&Favor" ), &AmarokConfig::setFavorTracks, ac, "favor_tracks" )
{
    setItems( QStringList() << i18nc( "State, as in disabled", "Off" )
                            << i18n( "Higher &Scores" )
                            << i18n( "Higher &Ratings" )
                            << i18n( "Not Recently &Played" ) );

    setCurrentItem( AmarokConfig::favorTracks() );
    setEnabled( AmarokConfig::randomMode() );
}

//////////////////////////////////////////////////////////////////////////////////////////
// RepeatAction
//////////////////////////////////////////////////////////////////////////////////////////
RepeatAction::RepeatAction( KActionCollection *ac ) :
    SelectAction( i18n( "&Repeat" ), &AmarokConfig::setRepeat, ac, "repeat" )
{
    setItems( QStringList() << i18nc( "State, as in, disabled", "&Off" ) << i18nc( "Item, as in, music", "&Track" )
                            << i18n( "&Album" ) << i18n( "&Playlist" ) );
    setIcons( QStringList() << "media-playlist-repeat-off-amarok" << "media-track-repeat-amarok" << "media-album-repeat-amarok" << "media-playlist-repeat-amarok" );
    setCurrentItem( AmarokConfig::repeat() );

    connect( this, SIGNAL( triggered( int ) ), The::playlistModel(), SLOT( playlistModeChanged() ) );
}

//////////////////////////////////////////////////////////////////////////////////////////
// BurnMenuAction
//////////////////////////////////////////////////////////////////////////////////////////
BurnMenuAction::BurnMenuAction( KActionCollection *ac )
  : KAction( 0 )
{
    setText(i18n( "Burn" ));
    ac->addAction("burn_menu", this);
}

QWidget*
BurnMenuAction::createWidget( QWidget *w )
{
    KToolBar *bar = dynamic_cast<KToolBar*>(w);

    if( bar && KAuthorized::authorizeKAction( objectName() ) )
    {
        //const int id = KAction::getToolButtonID();

        //addContainer( bar, id );
        w->addAction( this );
        connect( bar, SIGNAL( destroyed() ), SLOT( slotDestroyed() ) );

        //bar->insertButton( QString::null, id, true, i18n( "Burn" ), index );

        //KToolBarButton* button = bar->getButton( id );
        //button->setPopup( Amarok::BurnMenu::instance() );
        //button->setObjectName( "toolbutton_burn_menu" );
        //button->setIcon( "k3b" );

        //return associatedWidgets().count() - 1;
        return 0;
    }
    //else return -1;
    else return 0;
}


K_GLOBAL_STATIC( BurnMenu, s_burnMenu )

BurnMenu::BurnMenu()
{
    qAddPostRoutine( s_burnMenu.destroy );  // Ensure that the dtor gets called when QCoreApplication destructs

    addAction( i18n("Current Playlist"), this, SLOT( slotBurnCurrentPlaylist() ) );
    addAction( i18n("Selected Tracks"), this, SLOT( slotBurnSelectedTracks() ) );
    //TODO add "album" and "all tracks by artist"
}

KMenu*
BurnMenu::instance()
{
    return s_burnMenu;
}

void
BurnMenu::slotBurnCurrentPlaylist() //SLOT
{
    K3bExporter::instance()->exportCurrentPlaylist();
}

void
BurnMenu::slotBurnSelectedTracks() //SLOT
{
    K3bExporter::instance()->exportSelectedTracks();
}


//////////////////////////////////////////////////////////////////////////////////////////
// StopAction
//////////////////////////////////////////////////////////////////////////////////////////

StopAction::StopAction( KActionCollection *ac )
  : KAction( 0 )
  , EngineObserver( The::engineController() )
{
    setText( i18n( "Stop" ) );
    setIcon( KIcon("media-playback-stop-amarok") );
    setObjectName( "stop" );
    setGlobalShortcut( KShortcut( Qt::META + Qt::Key_V ) );
    connect( this, SIGNAL( triggered() ), The::engineController(), SLOT( stop() ) );
    ac->addAction( "stop", this );
    setEnabled( false );  // Disable action at startup
}

void
StopAction::engineStateChanged( Phonon::State state,  Phonon::State /*oldState*/ )
{
    switch( state ) {
    case Phonon::PlayingState:
    case Phonon::PausedState:
        setEnabled( true );
        break;
    case Phonon::StoppedState:
    case Phonon::LoadingState:
        setDisabled( true );
        break;
    case Phonon::ErrorState:
    case Phonon::BufferingState:
        break;
    }
}


#include "ActionClasses.moc"

