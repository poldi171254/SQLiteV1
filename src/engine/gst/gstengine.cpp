/***************************************************************************
                       gstengine.cpp - GStreamer audio interface

begin                : Jan 02 2003
copyright            : (C) 2003 by Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "config/gstconfig.h"
#include "enginebase.h"
#include "gstengine.h"
#include "streamsrc.h"

#include <math.h>
#include <unistd.h>
#include <vector>

#include <qfile.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>

#include <gst/gst.h>

using std::vector;

AMAROK_EXPORT_PLUGIN( GstEngine )


static const uint
SCOPEBUF_SIZE = 260000; // 260kb

static const int
STREAMBUF_SIZE = 1000000; // 1MB

static const uint
STREAMBUF_MIN = 50000; // 50kb

static const int
STREAMBUF_MAX = STREAMBUF_SIZE - 50000;

GstEngine*
GstEngine::s_instance;


/////////////////////////////////////////////////////////////////////////////////////
// CALLBACKS
/////////////////////////////////////////////////////////////////////////////////////

void
GstEngine::eos_cb( GstElement*, GstElement* ) //static
{
    kdDebug() << k_funcinfo << endl;

    //this is the Qt equivalent to an idle function: delay the call until all events are finished,
    //otherwise gst will crash horribly
    QTimer::singleShot( 0, instance(), SLOT( endOfStreamReached() ) );
}


void
GstEngine::handoff_cb( GstElement*, GstBuffer* buf, gpointer ) //static
{
    instance()->m_mutexScope.lock();

    // Check for buffer overflow
    uint available = gst_adapter_available( instance()->m_gst_adapter );
    if ( available > SCOPEBUF_SIZE )
        gst_adapter_flush( instance()->m_gst_adapter, available - 10000 );

    gst_buffer_ref( buf );

    // Push buffer into adapter, where it's chopped into chunks
    gst_adapter_push( instance()->m_gst_adapter, buf );

    instance()->m_mutexScope.unlock();
}


void
GstEngine::candecode_handoff_cb( GstElement*, GstBuffer*, gpointer ) //static
{
    kdDebug() <<  k_funcinfo << endl;

    instance()->m_canDecodeSuccess = true;
}


void
GstEngine::error_cb( GstElement* /*element*/, GstElement* /*source*/, GError* error, gchar* debug, gpointer /*data*/ ) //static
{
    kdDebug() << k_funcinfo << endl;

    // Process error message in application thread
    emit instance()->sigGstError( error, debug );
}


void
GstEngine::kio_resume_cb() //static
{
    if ( instance()->m_transferJob && instance()->m_transferJob->isSuspended() ) {
        instance()->m_transferJob->resume();
        kdDebug() << "Gst-Engine: RESUMING kio transfer.\n";
    }
}


void
GstEngine::shutdown_cb() //static
{
    instance()->m_shutdown = true;
    kdDebug() << "[Gst-Engine] Thread is shut down.\n";
}


/////////////////////////////////////////////////////////////////////////////////////
// CLASS GSTENGINE
/////////////////////////////////////////////////////////////////////////////////////

GstEngine::GstEngine()
        : Engine::Base( /*StreamingMode*/ Engine::Signal, /*hasConfigure*/ true )
        , m_gst_thread( 0 )
        , m_streamBuf( new char[STREAMBUF_SIZE] )
        , m_transferJob( 0 )
        , m_fadeValue( 0.0 )
        , m_pipelineFilled( false )
        , m_shutdown( false )
{
    kdDebug() << k_funcinfo << endl;
}


GstEngine::~GstEngine()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << "bytes left in gst_adapter: " << gst_adapter_available( m_gst_adapter ) << endl;

    if ( m_pipelineFilled ) {
        g_signal_connect( G_OBJECT( m_gst_thread ), "shutdown", G_CALLBACK( shutdown_cb ), m_gst_thread );
        destroyPipeline();
        // Wait for pipeline to shut down properly
        while ( !m_shutdown ) ::usleep( 20000 ); // 20 msec
    }
    else
        destroyPipeline();

    delete[] m_streamBuf;

    m_mutexScope.lock();
    g_object_unref( G_OBJECT( m_gst_adapter ) );
    m_mutexScope.unlock();

    // Save configuration
    GstConfig::writeConfig();

    kdDebug() << "END " << k_funcinfo << endl;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

bool
GstEngine::init()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    s_instance = this;

    // GStreamer initilization
    if ( !gst_init_check( NULL, NULL ) ) {
        KMessageBox::error( 0,
            i18n( "<h3>GStreamer could not be initialized.</h3> "
                  "<p>Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3), and run <i>'gst-register'</i> afterwards.</p>"
                  "<p>For further assistance consult the GStreamer manual, and join #gstreamer on irc.freenode.net.</p>" ) );
        return false;
    }

    // Check if registry exists
    GstElement* dummy = gst_element_factory_make ( "fakesink", "fakesink" );
    if ( !dummy || !gst_scheduler_factory_make( NULL, GST_ELEMENT ( dummy ) ) ) {
        KMessageBox::error( 0,
            i18n( "<h3>GStreamer is missing a registry.</h3> "
                  "<p>Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3), and run <i>'gst-register'</i> afterwards.</p>"
                  "<p>For further assistance consult the GStreamer manual, and join #gstreamer on irc.freenode.net.</p>" ) );
        return false;
    }

    m_gst_adapter = gst_adapter_new();
    startTimer( TIMER_INTERVAL );
    connect( this, SIGNAL( sigGstError( GError*, gchar* ) ), SLOT( handleGstError( GError*, gchar* ) ) );

    kdDebug() << "END " << k_funcinfo << endl;
    return true;
}


bool
GstEngine::canDecode( const KURL &url ) const
{
    int count = 0;
    m_canDecodeSuccess = false;
    GstElement *pipeline, *filesrc, *spider, *fakesink;

    if ( !( pipeline = createElement( "pipeline" ) ) ) return false;
    if ( !( filesrc = createElement( "filesrc", pipeline ) ) ) return false;
    if ( !( spider = createElement( "spider", pipeline ) ) ) return false;
    if ( !( fakesink = createElement( "fakesink", pipeline ) ) ) return false;

    GstCaps* filtercaps = gst_caps_new_simple( "audio/x-raw-int", NULL );

    gst_element_link( filesrc, spider );
    gst_element_link_filtered( spider, fakesink, filtercaps );

    g_object_set( G_OBJECT( filesrc ), "location", (const char*) QFile::encodeName( url.path() ), NULL );
    g_object_set( G_OBJECT( fakesink ), "signal_handoffs", true, NULL );
    g_signal_connect( G_OBJECT( fakesink ), "handoff", G_CALLBACK( candecode_handoff_cb ), pipeline );

    gst_element_set_state( pipeline, GST_STATE_PLAYING );

    // Try to iterate over the bin until handoff gets triggered
    while ( gst_bin_iterate( GST_BIN( pipeline ) ) && !m_canDecodeSuccess && count < 1000 )
        count++;

    gst_element_set_state( pipeline, GST_STATE_NULL );
    gst_object_unref( GST_OBJECT( pipeline ) );

    return m_canDecodeSuccess;
}


uint
GstEngine::position() const
{
    if ( !m_pipelineFilled ) return 0;

    GstFormat fmt = GST_FORMAT_TIME;
    // Value will hold the current time position in nanoseconds. Must be initialized!
    gint64 value = 0;
    gst_element_query( m_gst_spider, GST_QUERY_POSITION, &fmt, &value );

    return static_cast<long>( ( value / GST_MSECOND ) ); // nanosec -> msec
}


Engine::State
GstEngine::state() const
{
    if ( !m_pipelineFilled ) return Engine::Empty;

    switch ( gst_element_get_state( m_gst_thread ) )
    {
        case GST_STATE_NULL:
            return Engine::Empty;
        case GST_STATE_READY:
            return Engine::Idle;
        case GST_STATE_PLAYING:
            return Engine::Playing;
        case GST_STATE_PAUSED:
            return Engine::Paused;

        default:
            return Engine::Empty;
    }
}


const Engine::Scope&
GstEngine::scope()
{
    m_mutexScope.lock();

    int channels = 2;
    uint bytes = 512 * channels * sizeof( gint16 );

    gint16* data = (gint16*) gst_adapter_peek( m_gst_adapter, bytes );

    if ( data )
    {
        for ( ulong i = 0; i < 512; i++, data += channels ) {
            long temp = 0;

            for ( int chan = 0; chan < channels; chan++ ) {
                // Add all channels together so we effectively get a mono scope
                temp += data[chan];
            }
            m_scope[i] = temp / channels;
        }

        gst_adapter_flush( m_gst_adapter, bytes );
    }

    m_mutexScope.unlock();
    return m_scope;
}


amaroK::PluginConfig*
GstEngine::configure() const
{
    kdDebug() << k_funcinfo << endl;

    return new GstConfigDialog( instance() );
}

/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

bool
GstEngine::load( const KURL& url, bool stream )  //SLOT
{
    destroyPipeline();
    Engine::Base::load( url, stream );
    kdDebug() << "[Gst-Engine] Loading url: " << url.url() << endl;

    if ( GstConfig::soundOutput().isEmpty() ) {
        errorNoOutput();
        return false;
    }
    kdDebug() << "Thread scheduling priority: " << GstConfig::threadPriority() << endl;
    kdDebug() << "Sound output method: " << GstConfig::soundOutput() << endl;
    kdDebug() << "CustomSoundDevice: " << ( GstConfig::useCustomSoundDevice() ? "true" : "false" ) << endl;
    kdDebug() << "Sound Device: " << GstConfig::soundDevice() << endl;
    kdDebug() << "CustomOutputParams: " << ( GstConfig::useCustomOutputParams() ? "true" : "false" ) << endl;
    kdDebug() << "Output Params: " << GstConfig::outputParams() << endl;

    /* create a new pipeline (thread) to hold the elements */
    if ( !( m_gst_thread = createElement( "thread" ) ) ) { return false; }
    g_object_set( G_OBJECT( m_gst_thread ), "priority", GstConfig::threadPriority(), NULL );

    // Let gst construct the output element from a string
    QCString output  = GstConfig::soundOutput().latin1();
    if ( GstConfig::useCustomOutputParams() ) {
        output += " ";
        output += GstConfig::outputParams().latin1();
    }
    GError* err;
    if ( !( m_gst_audiosink = gst_parse_launch( output, &err ) ) ) { return false; }
    gst_bin_add( GST_BIN( m_gst_thread ), m_gst_audiosink );

    /* setting device property for AudioSink*/
    if ( GstConfig::useCustomSoundDevice() && !GstConfig::soundDevice().isEmpty() )
        g_object_set( G_OBJECT ( m_gst_audiosink ), "device", GstConfig::soundDevice().latin1(), NULL );

    if ( !( m_gst_identity = createElement( "identity", m_gst_thread ) ) ) { return false; }
    if ( !( m_gst_volume = createElement( "volume", m_gst_thread ) ) ) { return false; }
    if ( !( m_gst_volumeFade = createElement( "volume", m_gst_thread ) ) ) { return false; }
    if ( !( m_gst_audioconvert = createElement( "audioconvert", m_gst_thread, "audioconvert" ) ) ) { return false; }
    if ( !( m_gst_audioscale = createElement( "audioscale", m_gst_thread ) ) ) { return false; }

    g_object_set( G_OBJECT( m_gst_volumeFade ), "volume", 1.0, NULL );
    g_signal_connect( G_OBJECT( m_gst_identity ), "handoff", G_CALLBACK( handoff_cb ), m_gst_thread );
    g_signal_connect( G_OBJECT( m_gst_audiosink ), "eos", G_CALLBACK( eos_cb ), m_gst_thread );
    g_signal_connect ( G_OBJECT( m_gst_thread ), "error", G_CALLBACK ( error_cb ), m_gst_thread );

    if ( url.isLocalFile() ) {
        // Use gst's filesrc element for local files, cause it's less overhead than KIO
        if ( !( m_gst_src = createElement( "filesrc", m_gst_thread ) ) ) { return false; }
        // Set file path
        g_object_set( G_OBJECT( m_gst_src ), "location", static_cast<const char*>( QFile::encodeName( url.path() ) ), NULL );
    }
    else {
        // Create our custom streamsrc element, which transports data into the pipeline
        m_gst_src = GST_ELEMENT( gst_streamsrc_new( m_streamBuf, &m_streamBufIndex, &m_streamBufStop ) );
        g_object_set( G_OBJECT( m_gst_src ), "buffer_min", STREAMBUF_MIN, NULL );
        gst_bin_add ( GST_BIN ( m_gst_thread ), m_gst_src );
        g_signal_connect( G_OBJECT( m_gst_src ), "kio_resume", G_CALLBACK( kio_resume_cb ), m_gst_thread );
    }

    if ( !( m_gst_spider = createElement( "spider", m_gst_thread ) ) ) { return false; }

    /* link elements */
    gst_element_link_many( m_gst_src, m_gst_spider, m_gst_volumeFade, m_gst_identity, m_gst_volume, m_gst_audioscale, m_gst_audioconvert, m_gst_audiosink, 0 );
    m_pipelineFilled = true;

    if ( !gst_element_set_state( m_gst_thread, GST_STATE_READY ) ) {
        destroyPipeline();
        return false;
    }
    setVolume( m_volume );

    if ( !url.isLocalFile()  ) {
        m_streamBufIndex = 0;
        m_streamBufStop = false;

        if ( !stream ) {
            // Use KIO for non-local files, except http, which is handled by TitleProxy
            m_transferJob = KIO::get( url, false, false );
            connect( m_transferJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ),
                              this,   SLOT( newKioData( KIO::Job*, const QByteArray& ) ) );
            connect( m_transferJob, SIGNAL( result( KIO::Job* ) ),
                              this,   SLOT( kioFinished() ) );
        }
    }
    return true;
}


bool
GstEngine::play( uint )  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !m_pipelineFilled ) return false;

    /* start playing */
    if ( !gst_element_set_state( m_gst_thread, GST_STATE_PLAYING ) )
        return false;

    emit stateChanged( Engine::Playing );
    return true;
}


void
GstEngine::stop()  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !m_pipelineFilled ) return ;

    // Is a fade running?
    if ( m_fadeValue == 0.0 ) {
        // Not fading --> start fade now
        m_fadeValue = 1.0;
    }
    else
        // Fading --> stop playback
        destroyPipeline();

    emit stateChanged( Engine::Empty );
}


void
GstEngine::pause()  //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !m_pipelineFilled ) return ;

    if ( state() == Engine::Paused )
        gst_element_set_state( m_gst_thread, GST_STATE_PLAYING );
    else
        gst_element_set_state( m_gst_thread, GST_STATE_PAUSED );

    emit stateChanged( state() );
}


void
GstEngine::seek( uint ms )  //SLOT
{
    if ( !m_pipelineFilled ) return ;

    if ( ms > 0 )
    {
        GstEvent* event = gst_event_new_seek( ( GstSeekType ) ( GST_FORMAT_TIME | GST_SEEK_METHOD_SET | GST_SEEK_FLAG_FLUSH ),
                                              ms * GST_MSECOND );

        gst_element_send_event( m_gst_audiosink, event );
    }
}


void
GstEngine::newStreamData( char* buf, int size )  //SLOT
{
    if ( m_streamBufIndex + size >= STREAMBUF_SIZE ) {
        m_streamBufIndex = 0;
        kdDebug() << "Gst-Engine: Stream buffer overflow!" << endl;
    }

    sendBufferStatus();

    // Copy data into stream buffer
    memcpy( m_streamBuf + m_streamBufIndex, buf, size );
    // Adjust index
    m_streamBufIndex += size;
}


/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

void
GstEngine::setVolumeSW( uint percent )  //SLOT
{
    if ( m_pipelineFilled )
        g_object_set( G_OBJECT( m_gst_volume ), "volume", (double) percent * 0.01, NULL );
}


void GstEngine::timerEvent( QTimerEvent* )
{
    // Volume fading:

    if ( m_fadeValue > 0.0 )
    // Are we currently fading?
    {
        m_fadeValue -= ( GstConfig::fadeoutDuration() ) ?  1.0 / GstConfig::fadeoutDuration() * TIMER_INTERVAL : 1.0;

        // Fade finished?
        if ( m_fadeValue <= 0.0 ) {
            // Fade transition has finished, stop playback
            kdDebug() << "[Gst-Engine] Fade-out finished.\n";
            destroyPipeline();
        }

        if ( m_pipelineFilled ) {
            // Set new value for fadeout volume element
            double value = 1.0 - log10( ( 1.0 - m_fadeValue ) * 9.0 + 1.0 );
            g_object_set( G_OBJECT( m_gst_volumeFade ), "volume", value, NULL );
        }
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
GstEngine::handleGstError( GError* error, gchar* debugmsg )  //SLOT
{
    kdError() << "[GStreamer Error] " << endl;
    kdError() << error->message << endl;
    kdError() << debugmsg << endl;

    emit statusText( "[GStreamer Error] " + QString( error->message ) );
}


void
GstEngine::endOfStreamReached()  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    if ( m_pipelineFilled )
        gst_element_set_state( m_gst_thread, GST_STATE_READY );

    // Stop fading
    m_fadeValue = 0.0;

    emit trackEnded();
}


void
GstEngine::newKioData( KIO::Job*, const QByteArray& array )  //SLOT
{
    int size = array.size();

    if ( m_streamBufIndex >= STREAMBUF_MAX ) {
        kdDebug() << "Gst-Engine: SUSPENDING kio transfer.\n";
        if ( m_transferJob ) m_transferJob->suspend();
    }

    if ( m_streamBufIndex + size >= STREAMBUF_SIZE ) {
        m_streamBufIndex = 0;
        kdDebug() << "Gst-Engine: Stream buffer overflow!" << endl;
    }

    sendBufferStatus();

    // Copy data into stream buffer
    memcpy( m_streamBuf + m_streamBufIndex, array.data(), size );
    // Adjust index
    m_streamBufIndex += size;
}


void
GstEngine::kioFinished()  //SLOT
{
    kdDebug() << k_funcinfo << endl;

    // KIO::Job deletes itself when finished, so we need to zero the pointer
    m_transferJob = 0;

    // Tell streamsrc: This is the end, my friend
    m_streamBufStop = true;
}


void
GstEngine::errorNoOutput() //SLOT
{
    KMessageBox::information( 0, i18n( "<p>Please select a GStreamer <u>output plugin</u> in the engine settings dialog.</p>" ) );

    // Show engine settings dialog
    showEngineConfigDialog();
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

QStringList
GstEngine::getPluginList( const QCString& classname ) const
{
    GList * pool_registries = NULL;
    GList* registries = NULL;
    GList* plugins = NULL;
    GList* features = NULL;
    QStringList results;

    pool_registries = gst_registry_pool_list ();
    registries = pool_registries;

    while ( registries ) {
        GstRegistry * registry = GST_REGISTRY ( registries->data );
        plugins = registry->plugins;

        while ( plugins ) {
            GstPlugin * plugin = GST_PLUGIN ( plugins->data );
            features = gst_plugin_get_feature_list ( plugin );

            while ( features ) {
                GstPluginFeature * feature = GST_PLUGIN_FEATURE ( features->data );

                if ( GST_IS_ELEMENT_FACTORY ( feature ) ) {
                    GstElementFactory * factory = GST_ELEMENT_FACTORY ( feature );

                    if ( g_strrstr ( factory->details.klass, classname ) )
                        results << g_strdup ( GST_OBJECT_NAME ( factory ) );
                }
                features = g_list_next ( features );
            }
            plugins = g_list_next ( plugins );
        }
        registries = g_list_next ( registries );
    }
    g_list_free ( pool_registries );
    pool_registries = NULL;

    return results;
}


GstElement*
GstEngine::createElement( const QCString& factoryName, GstElement* bin, const QCString& name ) const
{
    GstElement* element = gst_element_factory_make( factoryName, name );

    if ( element ) {
        if ( bin ) gst_bin_add( GST_BIN( bin ), element );
    }
    else {
        KMessageBox::error( 0,
            i18n( "<h3>GStreamer could not create the element: <i>%1</i></h3> "
                  "<p>Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3), and run <i>'gst-register'</i> afterwards.</p>"
                  "<p>For further assistance consult the GStreamer manual, and join #gstreamer on irc.freenode.net.</p>" ).arg( factoryName ) );
        gst_object_unref( GST_OBJECT( bin ) );
    }

    return element;
}


void
GstEngine::destroyPipeline()
{
    kdDebug() << k_funcinfo << endl;

    m_fadeValue = 0.0;

    if ( m_pipelineFilled ) {
        kdDebug() << "[Gst-Engine] Destroying pipeline.\n";

        // Destroy the pipeline
        gst_element_set_state( m_gst_thread, GST_STATE_NULL );
        gst_object_unref( GST_OBJECT( m_gst_thread ) );
        gst_adapter_clear( m_gst_adapter );
        m_pipelineFilled = false;
    }

    // Destroy KIO transmission job
    if ( m_transferJob ) {
        m_transferJob->kill();
        m_transferJob = 0;
    }
}


void
GstEngine::sendBufferStatus()
{
    int percent = (int) ( (float) m_streamBufIndex / STREAMBUF_MIN * 100 );

    if ( percent >= 100 && percent < 120 )
        percent = 100;

    if ( percent <= 100 )
        emit statusText( i18n( "Buffering.. %1%" ).arg( percent ) );
}


#include "gstengine.moc"


