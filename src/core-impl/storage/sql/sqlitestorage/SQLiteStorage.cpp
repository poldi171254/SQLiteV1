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


//#include <SQLiteCpp/SQLiteCpp.h>

/** number of times the library is used.
 */
//static QAtomicInt libraryInitRef;


SQLiteStorage::SQLiteStorage() : SqlStorage ()
{
    DatabaseType = "SQLite";
}


bool
SQLiteStorage::init( const QString &storageLocation )
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage init";

    // debug() << "SQlite3 version " << SQLITE_VERSION;
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
            createSQLiteTables();
        } catch (std::exception& e) {
            debug() << "SQLite DB access problem" << e.what();
            return false;
        }
        }
    else {
        try {
            sqliteDB = new SQLite::Database(sqlitedb.toUtf8().data(), SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);

            // Since the DB was created, we need to add the tables for the collection
            createSQLiteTables();
        } catch (std::exception& e) {
            debug() << "SQLite DB could not be created" << e.what();
            return false;
        }
    }

    // This is only for initial debugging
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

SQLiteStorage::~SQLiteStorage()
{

    if(sqliteDB){
         sqliteDB->~Database();
    }

}

QString
SQLiteStorage::escape(const QString &text) const
{
    // DEBUG_BLOCK
    // debug() << "SQLiteStorage esacpe";
    /*
    const QByteArray utfText = text.toUtf8();
    const int length = utfText.length() * 2 + 1;
    QVarLengthArray<char, 1000> outputBuffer( length );

    {
        QMutexLocker locker( &m_mutex );
        mysql_real_escape_string( m_db, outputBuffer.data(), utfText.constData(), utfText.length() );
    }

    return QString::fromUtf8( outputBuffer.constData() );

     */

    // TODO
    // Need to find a way of doing this properly
    QByteArray utfText = text.toUtf8();
    QByteArray utftxt = text.toUtf8();

    int escTextIdx = 0;
    int araysz = utfText.size();

    utftxt.replace("'","''");

    //debug() << "SQLiteStorage string without escape-- " << utfText;
    //debug() << "SQLiteStorage string after escape-- " << utftxt ;

    return QString::fromUtf8(utftxt);
}

QStringList
SQLiteStorage::query(const QString &query)
{
    DEBUG_BLOCK

    debug() << "[ATTN!]SQLiteStorage::query( " << query << ")";

    QStringList values;

    bool finished = false;
    bool moreRows = true;

    if (query.contains("COUNT")){
        int a = 0;
    }
    try {
        SQLite::Statement statement(*sqliteDB, query.toUtf8().data() );
        while (statement.executeStep()) {
            // Check how many columns we have
            int cols = statement.getColumnCount();
            finished = statement.isDone();
            if (finished){
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
        debug() << "SQLite cannot get column " << e.what();
    }

    return values;
}


int
SQLiteStorage::insert(const QString &statement, const QString &table = QString())
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage insert " << statement;
    try {
        SQLite::Transaction transaction(*sqliteDB);
        int stat = sqliteDB->exec(statement.toUtf8().data());
        transaction.commit();
        return 1;
    } catch (std::exception& e) {
        debug() << "SQLite cannot INSERT " << e.what();
        return 0;
    }

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

    return db_lastErrors;
}

void
SQLiteStorage::clearLastErrors()
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage clearlastErrors";
    db_lastErrors.clear();
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
