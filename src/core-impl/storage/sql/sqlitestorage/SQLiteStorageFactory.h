/****************************************************************************************
 * Copyright (c) 2014 Ralf Engels <ralf-engels@gmx.de>                                   *
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

#ifndef AMAROK_STORAGE_SQLITESTORAGEFACTORY_H
#define AMAROK_STORAGE_SQLITESTORAGEFACTORY_H

#include "core/storage/StorageFactory.h"

class SQLiteStorageFactory : public StorageFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_storage-sqlitestorage.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        SQLiteStorageFactory();
        virtual ~SQLiteStorageFactory();

        void init() override;
};


#endif
