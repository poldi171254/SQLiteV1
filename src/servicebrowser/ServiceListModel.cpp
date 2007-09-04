/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "ServiceListModel.h"

ServiceListModel::ServiceListModel()
 : QAbstractListModel()
{
}


ServiceListModel::~ServiceListModel()
{
}

int ServiceListModel::rowCount(const QModelIndex & parent) const
{

    if ( !parent.isValid() )
        return 0;

    return m_services.count();
}

QVariant ServiceListModel::data(const QModelIndex & index, int role) const
{
     if ( (!index.isValid()) || ( m_services.count() <= index.row() ) )
         return QVariant();


    if ( role == Qt::DisplayRole )
        return m_services[index.row()]->getName();
    else if ( role ==  Qt::DecorationRole )
        return m_services[index.row()]->getIcon();
    else 
        return QVariant();
}

void ServiceListModel::addService(ServiceBase * service)
{

    beginInsertRows ( QModelIndex(), m_services.count(), m_services.count() + 1 );
    m_services.push_back( service );
    endInsertRows();


}


