/***************************************************************************
                          solarsystemcomposite.h  -  K Desktop Planetarium
                             -------------------
    begin                : 2005/01/09
    copyright            : (C) 2005 by Thomas Kabelmann
    email                : thomas.kabelmann@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "solarsystemcomposite.h"

#include <KLocalizedString>

#include "solarsystemsinglecomponent.h"
#include "asteroidscomponent.h"
#include "cometscomponent.h"
#include "skycomponent.h"

#include "Options.h"
#ifndef KSTARS_LITE
#include "skymap.h"
#endif
#include "kstarsdata.h"
#include "ksnumbers.h"
#include "skyobjects/ksplanet.h"
#include "skyobjects/kssun.h"
#include "skyobjects/ksmoon.h"
//#include "skyobjects/kspluto.h"
#include "planetmoonscomponent.h"

SolarSystemComposite::SolarSystemComposite(SkyComposite *parent ) :
    SkyComposite(parent)
{
    emitProgressText( i18n("Loading solar system" ) );
    m_Earth = new KSPlanet( I18N_NOOP( "Earth" ), QString(), QColor( "white" ), 12756.28 /*diameter in km*/ );

    m_Sun = new KSSun();
    SolarSystemSingleComponent *sun = new SolarSystemSingleComponent( this, m_Sun, Options::showSun );
    addComponent( sun, 2 );
    m_Moon = new KSMoon();
    SolarSystemSingleComponent *moon = new SolarSystemSingleComponent( this, m_Moon, Options::showMoon );
    addComponent( moon, 3 );
    SolarSystemSingleComponent *mercury = new SolarSystemSingleComponent( this, new KSPlanet( KSPlanetBase::MERCURY ), Options::showMercury );
    addComponent( mercury, 4 );
    SolarSystemSingleComponent *venus = new SolarSystemSingleComponent( this, new KSPlanet( KSPlanetBase::VENUS ), Options::showVenus );
    addComponent( venus, 4 );
    SolarSystemSingleComponent *mars = new SolarSystemSingleComponent( this, new KSPlanet( KSPlanetBase::MARS ), Options::showMars );
    addComponent( mars, 4 );
    SolarSystemSingleComponent *jup = new SolarSystemSingleComponent( this, new KSPlanet( KSPlanetBase::JUPITER ), Options::showJupiter );
    addComponent( jup, 4 );
    m_JupiterMoons = new PlanetMoonsComponent( this, jup, KSPlanetBase::JUPITER);
    addComponent( m_JupiterMoons, 5 );
    SolarSystemSingleComponent *sat = new SolarSystemSingleComponent( this, new KSPlanet( KSPlanetBase::SATURN ), Options::showSaturn );
    addComponent( sat, 4 );
    SolarSystemSingleComponent *uranus = new SolarSystemSingleComponent( this, new KSPlanet( KSPlanetBase::URANUS ), Options::showUranus );
    addComponent( uranus, 4 );
    SolarSystemSingleComponent *nep = new SolarSystemSingleComponent( this, new KSPlanet( KSPlanetBase::NEPTUNE ), Options::showNeptune );
    addComponent( nep, 4 );
    //addComponent( new SolarSystemSingleComponent( this, new KSPluto(), Options::showPluto ) );

    m_planets.append(sun);
    m_planets.append(moon);
    m_planets.append(mercury);
    m_planets.append(venus);
    m_planets.append(mars);
    m_planets.append(sat);
    m_planets.append(jup);
    m_planets.append(uranus);
    m_planets.append(nep);

    /*m_planetObjects.append(sun->planet());
    m_planetObjects.append(moon->planet());
    m_planetObjects.append(mercury->planet());
    m_planetObjects.append(venus->planet());
    m_planetObjects.append(mars->planet());
    m_planetObjects.append(sat->planet());
    m_planetObjects.append(jup->planet());
    m_planetObjects.append(uranus->planet());
    m_planetObjects.append(nep->planet());

    foreach(PlanetMoonsComponent *pMoons, planetMoonsComponent()) {
        PlanetMoons *moons = pMoons->getMoons();
        for(int i = 0; i < moons->nMoons(); ++i) {
            SkyObject *moon = moons->moon(i);
            objectLists(SkyObject::MOON).append(QPair<QString, const SkyObject*>(moon->name(), moon));
        }
    }*/

    addComponent( m_AsteroidsComponent = new AsteroidsComponent( this ), 7);
    addComponent( m_CometsComponent    = new CometsComponent( this ), 7);
}

SolarSystemComposite::~SolarSystemComposite()
{
    delete m_Earth;
}

bool SolarSystemComposite::selected()
{
#ifndef KSTARS_LITE
    return Options::showSolarSystem() &&
           !( Options::hideOnSlew() && Options::hidePlanets() && SkyMap::IsSlewing() );
#else
    return Options::showSolarSystem() &&
           !( Options::hideOnSlew() && Options::hidePlanets() );
#endif
}

void SolarSystemComposite::update( KSNumbers *num )
{
	//    if ( ! selected() ) return;

    KStarsData *data = KStarsData::Instance(); 
    m_Sun->EquatorialToHorizontal( data->lst(), data->geo()->lat() );
    m_Moon->EquatorialToHorizontal( data->lst(), data->geo()->lat() );
    m_JupiterMoons->update( num );

    foreach ( SkyComponent *comp, components() ) {
        comp->update( num );
    }
}

void SolarSystemComposite::updateSolarSystemBodies( KSNumbers *num )
{
    m_Earth->findPosition( num );
    foreach ( SkyComponent *comp, components() ) {
        comp->updateSolarSystemBodies( num );
    }
}

void SolarSystemComposite::updateMoons( KSNumbers *num )
{
	//    if ( ! selected() ) return;
    KStarsData *data = KStarsData::Instance(); 
    m_Sun->findPosition( num );
    m_Moon->findPosition( num, data->geo()->lat(), data->lst() );
    m_Moon->findPhase(0);
    m_JupiterMoons->updateMoons( num );
}

void SolarSystemComposite::drawTrails( SkyPainter* skyp )
{
    if( selected() )
        foreach( SkyComponent *comp, components() )
            comp->drawTrails( skyp );
}

const QList<SkyObject*>& SolarSystemComposite::asteroids() const {
    return m_AsteroidsComponent->objectList();
}

const QList<SkyObject*>& SolarSystemComposite::comets() const {
    return m_CometsComponent->objectList();
}

const QList<SkyObject*>& SolarSystemComposite::planetObjects() const {
    return m_planetObjects;
}

const QList<SkyObject*>& SolarSystemComposite::moons() const {
    return m_moons;
}


CometsComponent* SolarSystemComposite::cometsComponent()
{
    return m_CometsComponent;
}

AsteroidsComponent* SolarSystemComposite::asteroidsComponent()
{
    return m_AsteroidsComponent;
}

const QList<SolarSystemSingleComponent *>& SolarSystemComposite::planets() const {
    return m_planets;
}

QList<PlanetMoonsComponent *> SolarSystemComposite::planetMoonsComponent() const
{
    return QList <PlanetMoonsComponent *>({m_JupiterMoons});
}
