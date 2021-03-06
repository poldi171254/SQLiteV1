/****************************************************************************************
 * Copyright (c) 2019 Leo List <leo@zudiewiener.com>                                    *
 * *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version. *
 * *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 * *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "SQLiteStorage"

#include "SQLiteStorage.h"

#include <amarokconfig.h>
#include <core/support/Amarok.h>
#include <core/support/Debug.h>

#include <QDir>
#include <QVarLengthArray>
#include <QVector>
#include <QAtomicInt>


SQLiteStorage::SQLiteStorage()
    : SqlStorage ()
    , m_db( nullptr )
    , m_debugIdent( "SQLite" )

{
    DatabaseType = "SQLite";
}

SQLiteStorage::~SQLiteStorage()
{
    if(sqliteDB){
         sqliteDB->~Database();
    }
}

bool
SQLiteStorage::init( const QString &storageLocation )
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage init";
    debug() << "SQliteC++ version " << SQLITECPP_VERSION ;


    // -- figuring out and setting the database path.
    QString storagePath = storageLocation;
    QString databaseDir;
    QDir dir(storagePath);
    // TODO: the following logic is not explained in the comments.
    //  tests use a different directory then the real run
    if( storagePath.isEmpty() )
    {
        storagePath = Amarok::saveLocation();
        databaseDir = Amarok::config( "SQLite" ).readEntry( "SQLiteDatabase", QString(storagePath + "sqlite") );
        dir.mkpath(databaseDir);
    }
    else
    {
        // QDir dir( storagePath );
        dir.mkpath( "." );  //ensure directory exists
        databaseDir = dir.absolutePath() + QDir::separator() + "sqlite";
    }

    if(!QFile::exists(databaseDir)){
       debug() << " SQLite directory issues";
       return false;
    }

    QString sqlitedb = databaseDir + "/amarok.db3";

    if (QFileInfo::exists(sqlitedb) && QFileInfo(sqlitedb).isFile()){
        try {
            sqliteDB = new SQLite::Database(sqlitedb.toUtf8().data(), SQLite::OPEN_READWRITE);
            debug() << "Existing SQLite DB opened";
        } catch (std::exception& e) {
            reportError("SQLite DB access problem" );
            debug() << "SQLite DB access problem"  << e.what();
            return false;
        }
        }
    else {
        try {
            sqliteDB = new SQLite::Database(sqlitedb.toUtf8().data(), SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);

            // Since the DB was created, we need to add the tables for the collection
            createSQLiteTables();
        } catch (std::exception& e) {
            reportError("SQLite DB creation problem" );
            debug() << "SQLite DB could not be created" << e.what();
            return false;
        }
    }

    //TODO: This is only for initial debugging
    // Inspect a database via SQLite header information
    try
    {
       const SQLite::Header header = SQLite::Database::getHeaderInfo(sqlitedb.toUtf8().data());

       // Print values for all header fields
       // Official documentation for fields can be found here: https://www.sqlite.org/fileformat.html#the_database_header
        debug() << "Magic header string: " << header.headerStr;
        debug()  << "Page size bytes: " << header.pageSizeBytes;
        debug()  << "File format write version: " << (int)header.fileFormatWriteVersion;
        debug()  << "File format read version: " << (int)header.fileFormatReadVersion;
        debug()  << "Reserved space bytes: " << (int)header.reservedSpaceBytes;
        debug()  << "Max embedded payload fraction " << (int)header.maxEmbeddedPayloadFrac;
        debug()  << "Min embedded payload fraction: " << (int)header.minEmbeddedPayloadFrac;
        debug() << "Leaf payload fraction: " << (int)header.leafPayloadFrac;
        debug()  << "File change counter: " << header.fileChangeCounter;
        debug() << "Database size pages: " << header.databaseSizePages;
        debug()  << "First freelist trunk page: " << header.firstFreelistTrunkPage;
        debug()  << "Total freelist trunk pages: " << header.totalFreelistPages;
        debug()  << "Schema cookie: " << header.schemaCookie;
        debug()  << "Schema format number: " << header.schemaFormatNumber;
        debug()  << "Default page cache size bytes: " << header.defaultPageCacheSizeBytes;
        debug()  << "Largest B tree page number: " << header.largestBTreePageNumber;
        debug()  << "Database text encoding: " << header.databaseTextEncoding;
        debug()  << "User version: " << header.userVersion;
        debug()  << "Incremental vaccum mode: " << header.incrementalVaccumMode;
        debug()  << "Application ID: " << header.applicationId;
        debug()  << "Version valid for: " << header.versionValidFor;
       debug() << "SQLite version: " << header.sqliteVersion;
    }
    catch (std::exception& e)
    {
        debug() << "SQLite DB Header could not be extracted" << e.what();
    }


    return true;
}

QStringList
SQLiteStorage::query(const QString &m_statement)
{
    DEBUG_BLOCK
    debug() << "[ATTN!]SQLiteStorage::query( " << m_statement << ")";

    QStringList values;

    bool finished = false;
    bool moreRows = true;

    try {
        SQLite::Statement statement(*sqliteDB, m_statement.toUtf8().data() );
        while (statement.executeStep()) {
            // Check how many columns we have
            int cols = statement.getColumnCount();
            finished = statement.isDone();
            if (finished){
                debug() << "The result of the query " << m_statement << " is: " << values.count();
                return values;
            }
            else {
                for (int idx=0;idx<cols;idx++) {
                     SQLite::Column col = statement.getColumn(idx);
                    values << QString::fromUtf8( (const char*) col );
                }
            }
        }
    } catch (std::exception& e) {
        reportError( m_statement );
        debug() << "SQLite cannot get column " << e.what();
    }

    return values;
}

int
SQLiteStorage::insert(const QString &m_statement, const QString & /* table */)
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage insert " << m_statement;
    try {
        SQLite::Transaction transaction(*sqliteDB);
        int stat = sqliteDB->exec(m_statement.toUtf8().data());
        if (stat <= 0){
            reportError( m_statement );
        }
        transaction.commit();
        int res = sqliteDB->getLastInsertRowid();
        return res;
    } catch (std::exception& e) {
        reportError( m_statement );
        debug() << "SQLite cannot INSERT " << e.what();
        return 0;
    }

}

QString
SQLiteStorage::escape(const QString &text) const
{
    // DEBUG_BLOCK

    // TODO Need to find a way of doing this properly
    QByteArray utfText = text.toUtf8();
    QByteArray utftxt = text.toUtf8();

    int escTextIdx = 0;
    int araysz = utfText.size();

    utftxt.replace("'","''");

    return QString::fromUtf8(utftxt);
}

QString
SQLiteStorage::randomFunc() const
{
    return "RAND()";
}

QString
SQLiteStorage::boolTrue() const
{
    return "1";
}

QString
SQLiteStorage::boolFalse() const
{
    return "0";
}

QString
SQLiteStorage::idType() const
{
    return "INTEGER PRIMARY KEY AUTOINCREMENT";
}

QString
SQLiteStorage::textColumnType( int length ) const
{
    return QStringLiteral( "VARCHAR(%1)" ).arg( length );
}

QString
SQLiteStorage::exactTextColumnType( int length ) const
{
    return textColumnType( length );
}

QString
SQLiteStorage::exactIndexableTextColumnType( int length ) const
{
    return textColumnType( length );
}

QString
SQLiteStorage::longTextColumnType() const
{
    return "TEXT";
}

QStringList
SQLiteStorage::getLastErrors() const
{

    return m_lastErrors;
}

void
SQLiteStorage::clearLastErrors()
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage clearlastErrors";
    m_lastErrors.clear();
}

void
SQLiteStorage::reportError( const QString& message )
{

    QString errorMessage;
    if( m_db )
        errorMessage = m_debugIdent + " query failed! " + message;
    else
        errorMessage = m_debugIdent + " something failed! on " + message;
    error() << errorMessage;

    if( m_lastErrors.count() < 20 )
        m_lastErrors.append( errorMessage );
}

void
SQLiteStorage::createSQLiteTables()
{
    DEBUG_BLOCK


    //auto storage = this;


    // see docs/database/amarokTables.svg for documentation about database layout
    {
        QString create = "CREATE TABLE admin (component " + this->textColumnType() + ", version INTEGER)";
        query( create );
        QString insrt = "INSERT INTO admin(component, version) VALUES ('DB_VERSION', 15 )";
        QString table = "admin";
        insert(insrt,table);

    }

    {
        QString create = "CREATE TABLE urls "
                         "(id " + this->idType() +
                         ",deviceid INTEGER"
                         ",rpath " + this->exactIndexableTextColumnType() + " NOT NULL" +
                         ",directory INTEGER"
                         ",uniqueid " + this->exactTextColumnType(128) + " UNIQUE)";
        query( create );
        query( "CREATE UNIQUE INDEX urls_id_rpath ON urls(deviceid, rpath)" );
        query( "CREATE INDEX urls_uniqueid ON urls(uniqueid)" );
        query( "CREATE INDEX urls_directory ON urls(directory)" );
    }
    {
        QString create = "CREATE TABLE devices "
                         "(id " + this->idType() +
                         ",type " + this->textColumnType() +
                         ",label " + this->textColumnType() +
                         ",lastmountpoint " + this->textColumnType() +
                         ",uuid " + this->textColumnType() +
                         ",servername " + this->textColumnType(80) +
                         ",sharename " + this->textColumnType(240) +
                         ")";
        query( create );
        query( "CREATE INDEX devices_type ON devices( type )" );
        query( "CREATE UNIQUE INDEX devices_uuid ON devices( uuid )" );
        query( "CREATE INDEX devices_rshare ON devices( servername, sharename )" );
    }

    {
        QString create = "CREATE TABLE directories "
                         "(id " + this->idType() +
                         ",deviceid INTEGER"
                         ",dir " + this->exactTextColumnType() + " NOT NULL" +
                         ",changedate INTEGER)";
        query( create );
        query( "CREATE INDEX directories_deviceid ON directories(deviceid)" );
    }
    {
        QString create = "CREATE TABLE artists "
                         "(id " + this->idType() +
                         ",name " + this->textColumnType() + " NOT NULL)";
        query( create );
        query( "CREATE UNIQUE INDEX artists_name ON artists(name)" );
    }
    {
        QString create = "CREATE TABLE images "
                         "(id " + this->idType() +
                         ",path " + this->textColumnType() + " NOT NULL)";
        query( create );
        query( "CREATE UNIQUE INDEX images_name ON images(path)" );
    }
    {
        QString create = "CREATE TABLE albums "
                    "(id " + this->idType() +
                    ",name " + this->textColumnType() + " NOT NULL"
                    ",artist INTEGER" +
                    ",image INTEGER)";
        query( create );
        query( "CREATE INDEX albums_name ON albums(name)" );
        query( "CREATE INDEX albums_artist ON albums(artist)" );
        query( "CREATE INDEX albums_image ON albums(image)" );
        query( "CREATE UNIQUE INDEX albums_name_artist ON albums(name,artist)" );

    }
    {
        QString create = "CREATE TABLE genres "
                         "(id " + this->idType() +
                         ",name " + this->textColumnType() + " NOT NULL)";
        query( create );
        query( "CREATE UNIQUE INDEX genres_name ON genres(name)" );
    }
    {
        QString create = "CREATE TABLE composers "
                         "(id " + this->idType() +
                         ",name " + this->textColumnType() + " NOT NULL)";
        query( create );
        query( "CREATE UNIQUE INDEX composers_name ON composers(name)" );
    }
    {
        QString create = "CREATE TABLE years "
                         "(id " + this->idType() +
                         ",name " + this->textColumnType() + " NOT NULL)";
        query( create );
        query( "CREATE UNIQUE INDEX years_name ON years(name)" );
    }
    {
        QString create = "CREATE TABLE tracks "
                    "(id " + this->idType() +
                    ",url INTEGER"
                    ",artist INTEGER"
                    ",album INTEGER"
                    ",genre INTEGER"
                    ",composer INTEGER"
                    ",year INTEGER"
                    ",title " + this->textColumnType() +
                    ",comment " + this->longTextColumnType() +
                    ",tracknumber INTEGER"
                    ",discnumber INTEGER"
                    ",bitrate INTEGER"
                    ",length INTEGER"
                    ",samplerate INTEGER"
                    ",filesize INTEGER"
                    ",filetype INTEGER"     //does this still make sense?
                    ",bpm FLOAT"
                    ",createdate INTEGER"   // this is the track creation time
                    ",modifydate INTEGER"   // UNUSED currently
                    ",albumgain FLOAT"
                    ",albumpeakgain FLOAT" // decibels, relative to albumgain
                    ",trackgain FLOAT"
                    ",trackpeakgain FLOAT" // decibels, relative to trackgain
                    ") ";

        query( create );
        query( "CREATE UNIQUE INDEX tracks_url ON tracks(url)" );

        QStringList indices;
        indices << "id" << "artist" << "album" << "genre" << "composer" << "year" << "title";
        indices << "discnumber" << "createdate" << "length" << "bitrate" << "filesize";
        foreach( const QString &index, indices )
        {
            QString create = QString( "CREATE INDEX tracks_%1 ON tracks(%2);" ).arg( index, index );
            query( create );
        }
    }
    {
        QString create = "CREATE TABLE statistics "
                    "(id " + this->idType() +
                    ",url INTEGER NOT NULL"
                    ",createdate INTEGER" // this is the first played time
                    ",accessdate INTEGER" // this is the last played time
                    ",score FLOAT"
                    ",rating INTEGER NOT NULL DEFAULT 0" // the "default" undefined rating is 0. We cannot display anything else.
                    ",playcount INTEGER NOT NULL DEFAULT 0" // a track is either played or not.
                    ",deleted BOOL NOT NULL DEFAULT " + this->boolFalse() +
                    ")";
        query( create );
        query( "CREATE UNIQUE INDEX statistics_url ON statistics(url)" );
        QStringList indices;
        indices << "createdate" << "accessdate" << "score" << "rating" << "playcount";
        foreach( const QString &index, indices )
        {
            QString create = QString( "CREATE INDEX statistics_%1 ON statistics(%2);" ).arg( index, index );
            query( create );
        }
    }
    {
        QString create = "CREATE TABLE labels "
                    "(id " + this->idType() +
                    ",label " + this->textColumnType() +
                    ")";
        query( create );
        query( "CREATE UNIQUE INDEX labels_label ON labels(label)" );
     }
    {
        QString create = "CREATE TABLE urls_labels(url INTEGER, label INTEGER)";
        query( create );
        query( "CREATE INDEX urlslabels_url ON urls_labels(url)" );
        query( "CREATE INDEX urlslabels_label ON urls_labels(label)" );
    }
    {
        QString create = "CREATE TABLE amazon ("
                    "asin " + this->textColumnType( 20 ) +
                    ",locale " + this->textColumnType( 2 ) +
                    ",filename " + this->textColumnType( 33 ) +
                    ",refetchdate INTEGER )";
        query( create );
        query( "CREATE INDEX amazon_date ON amazon(refetchdate)" );
    }
    {
        QString create = "CREATE TABLE lyrics ("
                    "url INTEGER PRIMARY KEY"
                    ",lyrics " + this->longTextColumnType() +
                    ")";
        query( create );
    }
    query( "INSERT INTO admin(component,version) "
                          "VALUES('AMAROK_TRACK'," + QString::number( DB_VERSION ) + ")" );
    {
         query( "CREATE TABLE statistics_permanent "
                            "(url " + this->exactIndexableTextColumnType() + " NOT NULL" +
                            ",firstplayed DATETIME"
                            ",lastplayed DATETIME"
                            ",score FLOAT"
                            ",rating INTEGER DEFAULT 0"
                            ",playcount INTEGER)" );

        query( "CREATE UNIQUE INDEX stats_perm_url ON statistics_permanent(url)" );

    }
    {
        query( "CREATE TABLE statistics_tag "
                             "(name " + this->textColumnType(108) +
                             ",artist " + this->textColumnType(108) +
                             ",album " + this->textColumnType(108) +
                             ",firstplayed DATETIME"
                             ",lastplayed DATETIME"
                             ",score FLOAT"
                             ",rating INTEGER DEFAULT 0"
                             ",playcount INTEGER)" );

        query( "CREATE UNIQUE INDEX stats_tag_name_artist_album ON statistics_tag(name,artist,album)" );
    }
}
