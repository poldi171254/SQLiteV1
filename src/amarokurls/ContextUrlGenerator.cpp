/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ContextUrlGenerator.h"

#include "AmarokUrl.h"
#include "AmarokUrlHandler.h"
#include "context/ContextView.h"

#include <KLocalizedString>


ContextUrlGenerator * ContextUrlGenerator::s_instance = nullptr;

ContextUrlGenerator * ContextUrlGenerator::instance()
{
    if( s_instance == nullptr )
        s_instance = new ContextUrlGenerator();

    return s_instance;
}

ContextUrlGenerator::ContextUrlGenerator()
{
}

ContextUrlGenerator::~ContextUrlGenerator()
{
    The::amarokUrlHandler()->unRegisterGenerator( this );
}

AmarokUrl
ContextUrlGenerator::createContextBookmark()
{
    QStringList pluginNames = Context::ContextView::self()->currentApplets();
    QStringList appletNames = Context::ContextView::self()->currentAppletNames();
    AmarokUrl url;

    url.setCommand( QStringLiteral("context") );
    url.setArg( QStringLiteral("applets"), pluginNames.join( QStringLiteral(",") ) );

    url.setName( i18n( "Context: %1", appletNames.join( QStringLiteral(",") ) ) );
    return url;
}


QString
ContextUrlGenerator::description()
{
    return i18n( "Bookmark Context View Applets" );
}

QIcon ContextUrlGenerator::icon()
{
    return QIcon::fromTheme( QStringLiteral("x-media-podcast-amarok") );
}

AmarokUrl
ContextUrlGenerator::createUrl()
{
    return createContextBookmark();
}
