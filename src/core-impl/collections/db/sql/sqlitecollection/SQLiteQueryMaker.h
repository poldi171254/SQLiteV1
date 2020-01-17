/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_COLLECTION_SQLITEQUERYMAKER_H
#define AMAROK_COLLECTION_SQLITEQUERYMAKER_H

#include "../SqlCollection.h"
#include "../SqlQueryMaker.h"

namespace Collections
{

class SQLiteQueryMaker : public SqlQueryMaker
{
    public:
        explicit SQLiteQueryMaker( SqlCollection* collection );
        ~SQLiteQueryMaker() override;

    protected:
        QString escape( const QString &text ) const override;
};

}

#endif
