/***************************************************************************
                          listcomponent.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : 2005/10/01
    copyright            : (C) 2005 by Jason Harris
    email                : kstars@30doradus.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "listcomponent.h"

#include <QList>

#include "skymap.h" 

PointListComponent::PointListComponent( SkyComponent *parent, bool (*visibleMethod)() )
: SkyComponent( parent, visibleMethod )
{
}

PointListComponent::~PointListComponent()
{
}

void PointListComponent::update( KStarsData *data, KSNumbers *num )
{
	if ( visible() ) {
		foreach ( SkyPoint *p, pointList() ) {
			if ( num ) p->updateCoords( num );
			p->EquatorialToHorizontal( data->lst(), data->geo()->lat() );
		}
	}
}
