/****************************************************************************************
 * Copyright (c) 2009 Leo List      <leo@zudiewiener.com>                               *
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

#ifndef SQLITESTORAGE_H
#define SQLITESTORAGE_H

#include "../amarok_sqlstorage_export.h"
#include "core/storage/SqlStorage.h"
#include <QMutex>
#include <QString>
#include <SQLiteCpp/SQLiteCpp.h>

/**
 * Implements SQLite storage
 */

struct st_sqlite;
typedef struct st_sqlite SQLITE;

class SQLiteStorage : public SqlStorage
{
    public:
        //bool DBOpen = false;
        SQLite::Database *sqliteDB;
        SQLite::Statement *res;
        SQLite::Transaction *transaction;
        static const int DB_VERSION = 15;


        SQLiteStorage();
        virtual ~SQLiteStorage();

        /** Initializes the storage.
         *  @param storageLocation The directory for storing the SQLite database, will use the default defined by Amarok/KDE if not set.
         */
        bool init( const QString &storageLocation = QString() );

        QString escape( const QString &text ) const override;

        QStringList query( const QString &query ) override ;
        int insert( const QString &statement, const QString &table ) override;

        QString boolTrue() const override;
        QString boolFalse() const override;

        /**
        use this type for auto incrementing integer primary keys.
        */
        QString idType() const override;

        QString textColumnType( int length = 255 ) const override;
        QString exactTextColumnType( int length = 1000 ) const override;
        //the below value may have to be decreased even more for different indexes; only time will tell
        QString exactIndexableTextColumnType( int length = 324 ) const override;
        QString longTextColumnType() const override;
        QString randomFunc() const override;

        /** Returns a list of the last sql errors.
            The list might not include every one error if the number
            is beyond a sensible limit.
        */
        QStringList getLastErrors() const override;

        /** Clears the list of the last errors. */
        void clearLastErrors() override;

        void ResultSet(SQLite::Column data, QStringList &values);
        //TODO MySQL creates these tables in collections, but just for now I'm doing this here
        void createSQLiteTables();

    protected:
        /** Adds an error message to the m_lastErrors.
     *
     *  Adds a message including the mysql error number and message
     *  to the last error messages.
     *  @param message Usually the query statement being executed.
     */
        void reportError( const QString &message );

        void initThreadInitializer();

        /** Sends the first sql commands to setup the connection.
     *
     *  Sets things like the used database and charset.
     *  @returns false if something fatal was wrong.
     */
        bool sharedInit( const QString &databaseName );

        SQLITE* m_db;

        /** Mutex protecting the m_lastErrors list */
        mutable QMutex m_mutex;

        QString m_debugIdent;
        QStringList m_lastErrors;


};

#endif // SQLITESTORAGE_H
