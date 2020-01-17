/****************************************************************************************
 * Copyright (c) 2008 Edward Toroshchin <edward.hades@gmail.com>                        *
 * Copyright (c) 2009 Jeff Mitchell <mitchell@kde.org>                                  *
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

#define DEBUG_PREFIX "SQLiteStorage"

#include "SQLiteStorage.h"

#include <amarokconfig.h>
#include <core/support/Amarok.h>
#include <core/support/Debug.h>

#include <QDir>
#include <QVarLengthArray>
#include <QVector>
#include <QAtomicInt>

/** number of times the library is used.
 */
//static QAtomicInt libraryInitRef;

/*
SQLiteStorage::SQLiteStorage()
    : SqlStorage()
{
    // Needs to

}
*/

SQLiteStorage::SQLiteStorage()
{

}
bool
SQLiteStorage::init( const QString &storageLocation )
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage init";


    // -- figuring out and setting the database path.
    QString storagePath = storageLocation;
    QString databaseDir;
    // TODO: the following logic is not explained in the comments.
    //  tests use a different directory then the real run
    if( storagePath.isEmpty() )
    {
        storagePath = Amarok::saveLocation();
        databaseDir = Amarok::config( "MySQLe" ).readEntry( "data", QString(storagePath + "mysqle") );
    }
    else
    {
        QDir dir( storagePath );
        dir.mkpath( "." );  //ensure directory exists
        databaseDir = dir.absolutePath() + QDir::separator() + "mysqle";
    }

    QVector<const char*> sqlite_args;
    QByteArray dataDir = QStringLiteral( "--datadir=%1" ).arg( databaseDir ).toLocal8Bit();
    return true;
}

SQLiteStorage::~SQLiteStorage()
{
    /*
    if( m_db )
    {
        mysql_close( m_db );
        libraryInitRef.deref();
    }
    */
}

QString
SQLiteStorage::escape(const QString &text) const
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage esacpe";

    return QString::fromUtf8( "Test");
}

QStringList
SQLiteStorage::query(const QString &query)
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage query";
    QStringList fonts = { "Arial", "Helvetica", "Times" };
    return fonts;
}

int
SQLiteStorage::insert(const QString &statement, const QString &table = QString())
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage insert";
    return 1;
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
    return "INTEGER PRIMARY KEY AUTO_INCREMENT";
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

    QStringList fonts = { "Arial", "Helvetica", "Times" };
    return fonts;
}

void
SQLiteStorage::clearLastErrors()
{
    DEBUG_BLOCK
    debug() << "SQLiteStorage clearlastErrors";
}
