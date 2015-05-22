/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "CoverFoundDialog"

#include "CoverFoundDialog.h"

#include "SvgHandler.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "covermanager/CoverViewDialog.h"
#include "statusbar/KJobProgressBar.h"
#include "widgets/AlbumBreadcrumbWidget.h"
#include "widgets/PixmapViewer.h"

#include <KComboBox>
#include <KConfigGroup>
#include <KFileDialog>
#include <KLineEdit>
#include <KListWidget>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPushButton>
#include <KSaveFile>
#include <KStandardDirs>

#include <QCloseEvent>
#include <QDir>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QMenu>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QMimeDatabase>
#include <QMimeType>

CoverFoundDialog::CoverFoundDialog( const CoverFetchUnit::Ptr unit,
                                    const CoverFetch::Metadata &data,
                                    QWidget *parent )
    : KDialog( parent )
    , m_album( unit->album() )
    , m_isSorted( false )
    , m_sortEnabled( false )
    , m_unit( unit )
    , m_queryPage( 0 )
{
    DEBUG_BLOCK
    setButtons( KDialog::Ok | KDialog::Cancel |
                KDialog::User1 ); // User1: clear icon view

    setButtonGuiItem( KDialog::User1, KStandardGuiItem::clear() );
    connect( button( KDialog::User1 ), SIGNAL(clicked()), SLOT(clearView()) );

    m_save = button( KDialog::Ok );

    QSplitter *splitter = new QSplitter( this );
    m_sideBar = new CoverFoundSideBar( m_album, splitter );

    KVBox *vbox = new KVBox( splitter );
    vbox->setSpacing( 4 );

    KHBox *breadcrumbBox = new KHBox( vbox );
    QLabel *breadcrumbLabel = new QLabel( i18n( "Finding cover for" ), breadcrumbBox );
    AlbumBreadcrumbWidget *breadcrumb = new AlbumBreadcrumbWidget( m_album, breadcrumbBox );

    QFont breadcrumbLabelFont;
    breadcrumbLabelFont.setBold( true );
    breadcrumbLabel->setFont( breadcrumbLabelFont );
    breadcrumbLabel->setIndent( 4 );

    connect( breadcrumb, SIGNAL(artistClicked(QString)), SLOT(addToCustomSearch(QString)) );
    connect( breadcrumb, SIGNAL(albumClicked(QString)), SLOT(addToCustomSearch(QString)) );

    KHBox *searchBox = new KHBox( vbox );
    vbox->setSpacing( 4 );

    QStringList completionNames;
    QString firstRunQuery( m_album->name() );
    completionNames << firstRunQuery;
    if( m_album->hasAlbumArtist() )
    {
        const QString &name = m_album->albumArtist()->name();
        completionNames << name;
        firstRunQuery += ' ' + name;
    }
    m_query = firstRunQuery;
    m_album->setSuppressImageAutoFetch( true );

    m_search = new KComboBox( searchBox );
    m_search->setEditable( true ); // creates a KLineEdit for the combobox
    m_search->setTrapReturnKey( true );
    m_search->setInsertPolicy( QComboBox::NoInsert ); // insertion is handled by us
    m_search->setCompletionMode( KGlobalSettings::CompletionPopup );
    m_search->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );
    qobject_cast<KLineEdit*>( m_search->lineEdit() )->setClickMessage( i18n( "Enter Custom Search" ) );
    m_search->completionObject()->setOrder( KCompletion::Insertion );
    m_search->completionObject()->setIgnoreCase( true );
    m_search->completionObject()->setItems( completionNames );
    m_search->insertItem( 0, KStandardGuiItem::find().icon(), QString() );
    m_search->insertSeparator( 1 );
    m_search->insertItem( 2, KIcon("filename-album-amarok"), m_album->name() );
    if( m_album->hasAlbumArtist() )
        m_search->insertItem( 3, KIcon("filename-artist-amarok"), m_album->albumArtist()->name() );

    m_searchButton = new KPushButton( KStandardGuiItem::find(), searchBox );
    KPushButton *sourceButton = new KPushButton( KStandardGuiItem::configure(), searchBox );
    updateSearchButton( firstRunQuery );

    QMenu *sourceMenu = new QMenu( sourceButton );
    QAction *lastFmAct = new QAction( i18n( "Last.fm" ), sourceMenu );
    QAction *googleAct = new QAction( i18n( "Google" ), sourceMenu );
    QAction *discogsAct = new QAction( i18n( "Discogs" ), sourceMenu );
    lastFmAct->setCheckable( true );
    googleAct->setCheckable( true );
    discogsAct->setCheckable( true );
    connect( lastFmAct, SIGNAL(triggered()), this, SLOT(selectLastFm()) );
    connect( googleAct, SIGNAL(triggered()), this, SLOT(selectGoogle()) );
    connect( discogsAct, SIGNAL(triggered()), this, SLOT(selectDiscogs()) );

    m_sortAction = new QAction( i18n( "Sort by size" ), sourceMenu );
    m_sortAction->setCheckable( true );
    connect( m_sortAction, SIGNAL(triggered(bool)), this, SLOT(sortingTriggered(bool)) );

    QActionGroup *ag = new QActionGroup( sourceButton );
    ag->addAction( lastFmAct );
    ag->addAction( googleAct );
    ag->addAction( discogsAct );
    sourceMenu->addActions( ag->actions() );
    sourceMenu->addSeparator();
    sourceMenu->addAction( m_sortAction );
    sourceButton->setMenu( sourceMenu );

    connect( m_search, SIGNAL(returnPressed(QString)), SLOT(insertComboText(QString)) );
    connect( m_search, SIGNAL(returnPressed(QString)), SLOT(processQuery(QString)) );
    connect( m_search, SIGNAL(returnPressed(QString)), SLOT(updateSearchButton(QString)) );
    connect( m_search, SIGNAL(editTextChanged(QString)), SLOT(updateSearchButton(QString)) );
    connect( m_search->lineEdit(), SIGNAL(clearButtonClicked()), SLOT(clearQueryButtonClicked()));
    connect( m_searchButton, SIGNAL(clicked()), SLOT(processQuery()) );

    m_view = new KListWidget( vbox );
    m_view->setAcceptDrops( false );
    m_view->setContextMenuPolicy( Qt::CustomContextMenu );
    m_view->setDragDropMode( QAbstractItemView::NoDragDrop );
    m_view->setDragEnabled( false );
    m_view->setDropIndicatorShown( false );
    m_view->setMovement( QListView::Static );
    m_view->setGridSize( QSize( 140, 150 ) );
    m_view->setIconSize( QSize( 120, 120 ) );
    m_view->setSpacing( 4 );
    m_view->setViewMode( QListView::IconMode );
    m_view->setResizeMode( QListView::Adjust );

    connect( m_view, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
             this,   SLOT(currentItemChanged(QListWidgetItem*,QListWidgetItem*)) );
    connect( m_view, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this,   SLOT(itemDoubleClicked(QListWidgetItem*)) );
    connect( m_view, SIGNAL(customContextMenuRequested(QPoint)),
             this,   SLOT(itemMenuRequested(QPoint)) );

    splitter->addWidget( m_sideBar );
    splitter->addWidget( vbox );
    setMainWidget( splitter );

    const KConfigGroup config = Amarok::config( "Cover Fetcher" );
    const QString source = config.readEntry( "Interactive Image Source", "LastFm" );
    m_sortEnabled = config.readEntry( "Sort by Size", false );
    m_sortAction->setChecked( m_sortEnabled );
    m_isSorted = m_sortEnabled;
    restoreDialogSize( config ); // call this after setMainWidget()

    if( source == "LastFm" )
        lastFmAct->setChecked( true );
    else if( source == "Discogs" )
        discogsAct->setChecked( true );
    else
        googleAct->setChecked( true );

    typedef CoverFetchArtPayload CFAP;
    const CFAP *payload = dynamic_cast< const CFAP* >( unit->payload() );
    if( !m_album->hasImage() )
        m_sideBar->setPixmap( QPixmap::fromImage( m_album->image(190 ) ) );
    else if( payload )
        add( m_album->image(), data, payload->imageSize() );
    else
        add( m_album->image(), data );
    m_view->setCurrentItem( m_view->item( 0 ) );
    updateGui();
    
    connect( The::networkAccessManager(), SIGNAL(requestRedirected(QNetworkReply*,QNetworkReply*)),
             this, SLOT(fetchRequestRedirected(QNetworkReply*,QNetworkReply*)) );
}

CoverFoundDialog::~CoverFoundDialog()
{
    m_album->setSuppressImageAutoFetch( false );

    const QList<QListWidgetItem*> &viewItems = m_view->findItems( QChar('*'), Qt::MatchWildcard );
    qDeleteAll( viewItems );
    delete m_dialog.data();
}

void CoverFoundDialog::hideEvent( QHideEvent *event )
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    saveDialogSize( config );
    event->accept();
}

void CoverFoundDialog::add( const QImage &cover,
                            const CoverFetch::Metadata &metadata,
                            const CoverFetch::ImageSize imageSize )
{
    if( cover.isNull() )
        return;

    if( !contains( metadata ) )
    {
        CoverFoundItem *item = new CoverFoundItem( cover, metadata, imageSize );
        addToView( item );
    }
}

void CoverFoundDialog::addToView( CoverFoundItem *item )
{
    const CoverFetch::Metadata &metadata = item->metadata();

    if( m_sortEnabled && metadata.contains( "width" ) && metadata.contains( "height" ) )
    {
        if( m_isSorted )
        {
            const int size = metadata.value( "width" ).toInt() * metadata.value( "height" ).toInt();
            QList< int >::iterator i = qLowerBound( m_sortSizes.begin(), m_sortSizes.end(), size );
            m_sortSizes.insert( i, size );
            const int index = m_sortSizes.count() - m_sortSizes.indexOf( size ) - 1;
            m_view->insertItem( index, item );
        }
        else
        {
            m_view->addItem( item );
            sortCoversBySize();
        }
    }
    else
    {
        m_view->addItem( item );
    }
    updateGui();
}

bool CoverFoundDialog::contains( const CoverFetch::Metadata &metadata ) const
{
    for( int i = 0, count = m_view->count(); i < count; ++i )
    {
        CoverFoundItem *item = static_cast<CoverFoundItem*>( m_view->item(i) );
        if( item->metadata() == metadata )
            return true;
    }
    return false;
}

void CoverFoundDialog::addToCustomSearch( const QString &text )
{
    const QString &query = m_search->currentText();
    if( !text.isEmpty() && !query.contains( text ) )
    {
        QStringList q;
        if( !query.isEmpty() )
            q << query;
        q << text;
        const QString result = q.join( QChar( ' ' ) );
        qobject_cast<KLineEdit*>( m_search->lineEdit() )->setText( result );
    }
}

void CoverFoundDialog::clearQueryButtonClicked()
{
    m_query.clear();
    m_queryPage = 0;
    updateGui();
}

void CoverFoundDialog::clearView()
{
    m_view->clear();
    m_sideBar->clear();
    m_sortSizes.clear();
    updateGui();
}

void CoverFoundDialog::insertComboText( const QString &text )
{
    if( text.isEmpty() )
        return;

    if( m_search->contains( text ) )
    {
        m_search->setCurrentIndex( m_search->findText( text ) );
        return;
    }
    m_search->completionObject()->addItem( text );
    m_search->insertItem( 0, KStandardGuiItem::find().icon(), text );
    m_search->setCurrentIndex( 0 );
}

void CoverFoundDialog::currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous )
{
    Q_UNUSED( previous )
    if( !current )
        return;
    CoverFoundItem *it = static_cast< CoverFoundItem* >( current );
    QImage image = it->hasBigPix() ? it->bigPix() : it->thumb();
    m_image = image;
    m_sideBar->setPixmap( QPixmap::fromImage(image), it->metadata() );
}

void CoverFoundDialog::itemDoubleClicked( QListWidgetItem *item )
{
    Q_UNUSED( item )
    slotButtonClicked( KDialog::Ok );
}

void CoverFoundDialog::itemMenuRequested( const QPoint &pos )
{
    const QPoint globalPos = m_view->mapToGlobal( pos );
    QModelIndex index = m_view->indexAt( pos );

    if( !index.isValid() )
        return;

    CoverFoundItem *item = static_cast< CoverFoundItem* >( m_view->item( index.row() ) );
    item->setSelected( true );

    QMenu menu( this );
    QAction *display = new QAction( KIcon("zoom-original"), i18n("Display Cover"), &menu );
    connect( display, SIGNAL(triggered()), this, SLOT(display()) );

    QAction *save = new QAction( KIcon("document-save"), i18n("Save As"), &menu );
    connect( save, SIGNAL(triggered()), this, SLOT(saveAs()) );

    menu.addAction( display );
    menu.addAction( save );
    menu.exec( globalPos );
}

void CoverFoundDialog::saveAs()
{
    CoverFoundItem *item = static_cast< CoverFoundItem* >( m_view->currentItem() );
    if( !item->hasBigPix() && !fetchBigPix() )
        return;

    Meta::TrackList tracks = m_album->tracks();
    if( tracks.isEmpty() )
    {
        warning() << "no tracks associated with album" << m_album->name();
        return;
    }

    KFileDialog dlg( tracks.first()->playableUrl().directory(), QString(), this );
    dlg.setCaption( i18n("Cover Image Save Location") );
    dlg.setMode( KFile::File | KFile::LocalOnly );
    dlg.setOperationMode( KFileDialog::Saving );
    dlg.setConfirmOverwrite( true );
    dlg.setSelection( "cover.jpg" );

    QStringList supportedMimeTypes;
    supportedMimeTypes << "image/jpeg";
    supportedMimeTypes << "image/png";
    dlg.setMimeFilter( supportedMimeTypes );

    KUrl saveUrl;
    int res = dlg.exec();
    switch( res )
    {
    case QDialog::Accepted:
        saveUrl = dlg.selectedUrl();
        break;
    case QDialog::Rejected:
        return;
    }

    KSaveFile saveFile( saveUrl.path() );
    if( !saveFile.open() )
    {
        KMessageBox::detailedError( this,
                                    i18n("Sorry, the cover could not be saved."),
                                    saveFile.errorString() );
        return;
    }

    const QImage &image = item->bigPix();
    const QString &ext = db.suffixForFileName( saveUrl.path() ).toLower();
    if( ext == "jpg" || ext == "jpeg" )
        image.save( &saveFile, "JPG" );
    else if( ext == "png" )
        image.save( &saveFile, "PNG" );
    else
        image.save( &saveFile );

    if( (saveFile.size() == 0) || !saveFile.finalize() )
    {
        KMessageBox::detailedError( this,
                                    i18n("Sorry, the cover could not be saved."),
                                    saveFile.errorString() );
        saveFile.remove();
    }
}

void CoverFoundDialog::slotButtonClicked( int button )
{
    if( button == KDialog::Ok )
    {
        CoverFoundItem *item = dynamic_cast< CoverFoundItem* >( m_view->currentItem() );
        if( !item )
        {
            reject();
            return;
        }

        bool gotBigPix( true );
        if( !item->hasBigPix() )
            gotBigPix = fetchBigPix();

        if( gotBigPix )
        {
            m_image = item->bigPix();
            accept();
        }
    }
    else
    {
        KDialog::slotButtonClicked( button );
    }
}

void CoverFoundDialog::fetchRequestRedirected( QNetworkReply *oldReply,
                                               QNetworkReply *newReply )
{
    KUrl oldUrl = oldReply->request().url();
    KUrl newUrl = newReply->request().url();

    // Since we were redirected we have to check if the redirect
    // was for one of our URLs and if the new URL is not handled
    // already.
    if( m_urls.contains( oldUrl ) && !m_urls.contains( newUrl ) )
    {
        // Get the unit for the old URL.
        CoverFoundItem *item = m_urls.value( oldUrl );

        // Add the unit with the new URL and remove the old one.
        m_urls.insert( newUrl, item );
        m_urls.remove( oldUrl );
    }
}

void CoverFoundDialog::handleFetchResult( const KUrl &url, QByteArray data,
                                          NetworkAccessManagerProxy::Error e )
{
    CoverFoundItem *item = m_urls.take( url );
    QImage image;
    if( item && e.code == QNetworkReply::NoError && image.loadFromData( data ) )
    {
        item->setBigPix( image );
        m_sideBar->setPixmap( QPixmap::fromImage( image ) );
        if( m_dialog )
            m_dialog.data()->accept();
    }
    else
    {
        QStringList errors;
        errors << e.description;
        KMessageBox::errorList( this, i18n("Sorry, the cover image could not be retrieved."), errors );
        if( m_dialog )
            m_dialog.data()->reject();
    }
}

bool CoverFoundDialog::fetchBigPix()
{
    DEBUG_BLOCK
    CoverFoundItem *item = static_cast< CoverFoundItem* >( m_view->currentItem() );
    const KUrl url( item->metadata().value( "normalarturl" ) );
    if( !url.isValid() )
        return false;

    QNetworkReply *reply = The::networkAccessManager()->getData( url, this,
                           SLOT(handleFetchResult(KUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
    m_urls.insert( url, item );

    if( !m_dialog )
    {
        m_dialog = new KProgressDialog( this );
        m_dialog.data()->setCaption( i18n( "Fetching Large Cover" ) );
        m_dialog.data()->setLabelText( i18n( "Download Progress" ) );
        m_dialog.data()->setModal( true );
        m_dialog.data()->setAllowCancel( true );
        m_dialog.data()->setAutoClose( false );
        m_dialog.data()->setAutoReset( true );
        m_dialog.data()->progressBar()->setMinimum( 0 );
        m_dialog.data()->setMinimumWidth( 300 );
        connect( reply, SIGNAL(downloadProgress(qint64,qint64)),
                        SLOT(downloadProgressed(qint64,qint64)) );
    }
    int result = m_dialog.data()->exec();
    bool success = (result == QDialog::Accepted) && !m_dialog.data()->wasCancelled();
    The::networkAccessManager()->abortGet( url );
    if( !success )
        m_urls.remove( url );
    m_dialog.data()->deleteLater();
    return success;
}

void CoverFoundDialog::downloadProgressed( qint64 bytesReceived, qint64 bytesTotal )
{
    if( m_dialog )
    {
        m_dialog.data()->progressBar()->setMaximum( bytesTotal );
        m_dialog.data()->progressBar()->setValue( bytesReceived );
    }
}

void CoverFoundDialog::display()
{
    CoverFoundItem *item = static_cast< CoverFoundItem* >( m_view->currentItem() );
    const bool success = item->hasBigPix() ? true : fetchBigPix();
    if( !success )
        return;

    const QImage &image = item->hasBigPix() ? item->bigPix() : item->thumb();
    QWeakPointer<CoverViewDialog> dlg = new CoverViewDialog( image, this );
    dlg.data()->show();
    dlg.data()->raise();
    dlg.data()->activateWindow();
}

void CoverFoundDialog::processQuery()
{
    const QString text = m_search->currentText();
    processQuery( text );
}

void CoverFoundDialog::processQuery( const QString &input )
{
    const bool inputEmpty( input.isEmpty() );
    const bool mQueryEmpty( m_query.isEmpty() );

    QString q;
    if( inputEmpty && !mQueryEmpty )
    {
        q = m_query;
    }
    else if( !inputEmpty || !mQueryEmpty )
    {
        q = input;
        if( m_query != input )
        {
            m_query = input;
            m_queryPage = 0;
        }
    }

    if( !q.isEmpty() )
    {
        emit newCustomQuery( m_album, q, m_queryPage );
        updateSearchButton( q );
        m_queryPage++;
    }
}

void CoverFoundDialog::selectDiscogs()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Discogs" );
    m_sortAction->setEnabled( true );
    m_queryPage = 0;
    processQuery();
    debug() << "Select Discogs as source";
}

void CoverFoundDialog::selectLastFm()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "LastFm" );
    m_sortAction->setEnabled( false );
    m_queryPage = 0;
    processQuery();
    debug() << "Select Last.fm as source";
}

void CoverFoundDialog::selectGoogle()
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Interactive Image Source", "Google" );
    m_sortAction->setEnabled( true );
    m_queryPage = 0;
    processQuery();
    debug() << "Select Google as source";
}

void CoverFoundDialog::setQueryPage( int page )
{
    m_queryPage = page;
}

void CoverFoundDialog::sortingTriggered( bool checked )
{
    KConfigGroup config = Amarok::config( "Cover Fetcher" );
    config.writeEntry( "Sort by Size", checked );
    m_sortEnabled = checked;
    m_isSorted = false;
    if( m_sortEnabled )
        sortCoversBySize();
    debug() << "Enable sorting by size:" << checked;
}

void CoverFoundDialog::sortCoversBySize()
{
    DEBUG_BLOCK

    m_sortSizes.clear();
    QList< QListWidgetItem* > viewItems = m_view->findItems( QChar('*'), Qt::MatchWildcard );
    QMultiMap<int, CoverFoundItem*> sortItems;

    // get a list of cover items sorted (automatically by qmap) by size
    foreach( QListWidgetItem *viewItem, viewItems  )
    {
        CoverFoundItem *coverItem = dynamic_cast<CoverFoundItem*>( viewItem );
        const CoverFetch::Metadata &meta = coverItem->metadata();
        const int itemSize = meta.value( "width" ).toInt() * meta.value( "height" ).toInt();
        sortItems.insert( itemSize, coverItem );
        m_sortSizes << itemSize;
    }

    // take items from the view and insert into a temp list in the sorted order
    QList<CoverFoundItem*> coverItems = sortItems.values();
    QList<CoverFoundItem*> tempItems;
    for( int i = 0, count = sortItems.count(); i < count; ++i )
    {
        CoverFoundItem *item = coverItems.value( i );
        const int itemRow = m_view->row( item );
        QListWidgetItem *itemFromRow = m_view->takeItem( itemRow );
        if( itemFromRow )
            tempItems << dynamic_cast<CoverFoundItem*>( itemFromRow );
    }

    // add the items back to the view in descending order
    foreach( CoverFoundItem* item, tempItems )
        m_view->insertItem( 0, item );

    m_isSorted = true;
}

void CoverFoundDialog::updateSearchButton( const QString &text )
{
    const bool isNewSearch = ( text != m_query ) ? true : false;
    m_searchButton->setGuiItem( isNewSearch ? KStandardGuiItem::find() : KStandardGuiItem::cont() );
    m_searchButton->setToolTip( isNewSearch ? i18n( "Search" ) : i18n( "Search For More Results" ) );
}

void CoverFoundDialog::updateGui()
{
    updateTitle();

    if( !m_search->hasFocus() )
        setButtonFocus( KDialog::Ok );
    update();
}

void CoverFoundDialog::updateTitle()
{
    const int itemCount = m_view->count();
    const QString caption = ( itemCount == 0 )
                          ? i18n( "No Images Found" )
                          : i18np( "1 Image Found", "%1 Images Found", itemCount );
    setCaption( caption );
}

CoverFoundSideBar::CoverFoundSideBar( const Meta::AlbumPtr album, QWidget *parent )
    : KVBox( parent )
    , m_album( album )
{
    m_cover = new QLabel( this );
    m_tabs  = new QTabWidget( this );
    m_notes = new QLabel;
    QScrollArea *metaArea = new QScrollArea;
    m_metaTable = new QWidget( metaArea );
    m_metaTable->setLayout( new QFormLayout );
    m_metaTable->setMinimumSize( QSize( 150, 200 ) );
    metaArea->setFrameShape( QFrame::NoFrame );
    metaArea->setWidget( m_metaTable );
    m_notes->setAlignment( Qt::AlignLeft | Qt::AlignTop );
    m_notes->setMargin( 4 );
    m_notes->setOpenExternalLinks( true );
    m_notes->setTextFormat( Qt::RichText );
    m_notes->setTextInteractionFlags( Qt::TextBrowserInteraction );
    m_notes->setWordWrap( true );
    m_cover->setAlignment( Qt::AlignCenter );
    m_tabs->addTab( metaArea, i18n( "Information" ) );
    m_tabs->addTab( m_notes, i18n( "Notes" ) );
    setMaximumWidth( 200 );
    setPixmap( QPixmap::fromImage( m_album->image( 190 ) ) );
    clear();
}

CoverFoundSideBar::~CoverFoundSideBar()
{
}

void CoverFoundSideBar::clear()
{
    clearMetaTable();
    m_notes->clear();
    m_metadata.clear();
}

void CoverFoundSideBar::setPixmap( const QPixmap &pixmap, CoverFetch::Metadata metadata )
{
    m_metadata = metadata;
    updateNotes();
    setPixmap( pixmap );
}

void CoverFoundSideBar::setPixmap( const QPixmap &pixmap )
{
    m_pixmap = pixmap;
    QPixmap scaledPix = pixmap.scaled( QSize( 190, 190 ), Qt::KeepAspectRatio );
    QPixmap prettyPix = The::svgHandler()->addBordersToPixmap( scaledPix, 5, QString(), true );
    m_cover->setPixmap( prettyPix );
    updateMetaTable();
}

void CoverFoundSideBar::updateNotes()
{
    bool enableNotes( false );
    if( m_metadata.contains( "notes" ) )
    {
        const QString notes = m_metadata.value( "notes" );
        if( !notes.isEmpty() )
        {
            m_notes->setText( notes );
            enableNotes = true;
        }
        else
            enableNotes = false;
    }
    else
    {
        m_notes->clear();
        enableNotes = false;
    }
    m_tabs->setTabEnabled( m_tabs->indexOf( m_notes ), enableNotes );
}

void CoverFoundSideBar::updateMetaTable()
{
    clearMetaTable();

    QFormLayout *layout = static_cast< QFormLayout* >( m_metaTable->layout() );
    layout->setSizeConstraint( QLayout::SetMinAndMaxSize );

    CoverFetch::Metadata::const_iterator mit = m_metadata.constBegin();
    while( mit != m_metadata.constEnd() )
    {
        const QString &value = mit.value();
        if( !value.isEmpty() )
        {
            const QString &tag = mit.key();
            QString name;

            #define TAGHAS(s) (tag.compare(QLatin1String(s)) == 0)
            if( TAGHAS("artist") )        name = i18nc( "@item::intable", "Artist" );
            else if( TAGHAS("country") )  name = i18nc( "@item::intable", "Country" );
            else if( TAGHAS("date") )     name = i18nc( "@item::intable", "Date" );
            else if( TAGHAS("format") )   name = i18nc( "@item::intable File Format", "Format" );
            else if( TAGHAS("height") )   name = i18nc( "@item::intable Image Height", "Height" );
            else if( TAGHAS("name") )     name = i18nc( "@item::intable Album Title", "Title" );
            else if( TAGHAS("type") )     name = i18nc( "@item::intable Release Type", "Type" );
            else if( TAGHAS("released") ) name = i18nc( "@item::intable Release Date", "Released" );
            else if( TAGHAS("size") )     name = i18nc( "@item::intable File Size", "Size" );
            else if( TAGHAS("source") )   name = i18nc( "@item::intable Cover Provider", "Source" );
            else if( TAGHAS("title") )    name = i18nc( "@item::intable Album Title", "Title" );
            else if( TAGHAS("width") )    name = i18nc( "@item::intable Image Width", "Width" );
            #undef TAGHAS

            if( !name.isEmpty() )
            {
                QLabel *label = new QLabel( value, 0 );
                label->setToolTip( value );
                layout->addRow( QString("<b>%1:</b>").arg(name), label );
            }
        }
        ++mit;
    }

    QString refUrl;

    const QString source = m_metadata.value( "source" );
    if( source == "Last.fm" || source == "Discogs" )
    {
        refUrl = m_metadata.value( "releaseurl" );
    }
    else if( source == "Google" )
    {
        refUrl = m_metadata.value( "imgrefurl" );
    }

    if( !refUrl.isEmpty() )
    {
        QFont font;
        QFontMetrics qfm( font );
        const QString &toolUrl = refUrl;
        const QString &tooltip = qfm.elidedText( toolUrl, Qt::ElideMiddle, 350 );
        const QString &decoded = QUrl::fromPercentEncoding( refUrl.toLocal8Bit() );
        const QString &url     = QString( "<a href=\"%1\">%2</a>" )
                                    .arg( decoded )
                                    .arg( i18nc("@item::intable URL", "link") );

        QLabel *label = new QLabel( url, 0 );
        label->setOpenExternalLinks( true );
        label->setTextInteractionFlags( Qt::TextBrowserInteraction );
        label->setToolTip( tooltip );
        layout->addRow( QString( "<b>%1:</b>" ).arg( i18nc("@item::intable", "URL") ), label );
    }
}

void CoverFoundSideBar::clearMetaTable()
{
    QFormLayout *layout = static_cast< QFormLayout* >( m_metaTable->layout() );
    int count = layout->count();
    while( --count >= 0 )
    {
        QLayoutItem *child = layout->itemAt( 0 );
        layout->removeItem( child );
        delete child->widget();
        delete child;
    }
}

CoverFoundItem::CoverFoundItem( const QImage &cover,
                                const CoverFetch::Metadata &data,
                                const CoverFetch::ImageSize imageSize,
                                QListWidget *parent )
    : QListWidgetItem( parent )
    , m_metadata( data )
{
    switch( imageSize )
    {
    default:
    case CoverFetch::NormalSize:
        m_bigPix = cover;
        break;
    case CoverFetch::ThumbSize:
        m_thumb = cover;
        break;
    }

    QPixmap scaledPix = QPixmap::fromImage(cover.scaled( QSize( 120, 120 ), Qt::KeepAspectRatio ));
    QPixmap prettyPix = The::svgHandler()->addBordersToPixmap( scaledPix, 5, QString(), true );
    setSizeHint( QSize( 140, 150 ) );
    setIcon( prettyPix );
    setCaption();
    setFont( KGlobalSettings::smallestReadableFont() );
    setTextAlignment( Qt::AlignHCenter | Qt::AlignTop );
}

CoverFoundItem::~CoverFoundItem()
{
}

bool CoverFoundItem::operator==( const CoverFoundItem &other ) const
{
    return m_metadata == other.m_metadata;
}

bool CoverFoundItem::operator!=( const CoverFoundItem &other ) const
{
    return !( *this == other );
}

void CoverFoundItem::setCaption()
{
    QStringList captions;
    const QString &width = m_metadata.value( QLatin1String("width") );
    const QString &height = m_metadata.value( QLatin1String("height") );
    if( !width.isEmpty() && !height.isEmpty() )
        captions << QString( "%1 x %2" ).arg( width ).arg( height );

    int size = m_metadata.value( QLatin1String("size") ).toInt();
    if( size )
    {
        const QString source = m_metadata.value( QLatin1String("source") );

        captions << ( QString::number( size ) + QLatin1Char('k') );
    }

    if( !captions.isEmpty() )
        setText( captions.join( QLatin1String( " - " ) ) );
}

