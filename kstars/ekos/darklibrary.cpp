/*  Ekos Dark Library Handler
    Copyright (C) 2016 Jasem Mutlaq <mutlaqja@ikarustech.com>

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
 */

#include <QVariantMap>

#include "darklibrary.h"
#include "Options.h"

#include "kstars.h"
#include "kspaths.h"
#include "kstarsdata.h"
#include "fitsviewer/fitsview.h"
#include "fitsviewer/fitsdata.h"
#include "auxiliary/ksuserdb.h"

namespace Ekos
{

DarkLibrary * DarkLibrary::_DarkLibrary = NULL;

DarkLibrary * DarkLibrary::Instance()
{
    if (_DarkLibrary == NULL)
        _DarkLibrary = new DarkLibrary(KStars::Instance());

    return _DarkLibrary;
}

DarkLibrary::DarkLibrary(QObject *parent) : QObject(parent)
{
    KStarsData::Instance()->userdb()->GetAllDarkFrames(darkFrames);


    subtractParams.duration=0;
    subtractParams.offsetX=0;
    subtractParams.offsetY=0;
    subtractParams.targetChip=0;
    subtractParams.targetImage=0;

    QDir writableDir;
    writableDir.mkdir(KSPaths::writableLocation(QStandardPaths::GenericDataLocation) + "darks");
}

DarkLibrary::~DarkLibrary()
{
    qDeleteAll(darkFiles);
}

FITSData * DarkLibrary::getDarkFrame(ISD::CCDChip *targetChip, double duration)
{
    foreach(QVariantMap map, darkFrames)
    {
        // First check CCD name matches
        if (map["ccd"].toString() == targetChip->getCCD()->getDeviceName())
        {
            // Then check we are on the correct chip
            if (map["chip"].toInt() == static_cast<int>(targetChip->getType()))
            {
                int binX, binY;
                targetChip->getBinning(&binX, &binY);

                // Then check if binning is the same
                if (map["binX"].toInt() == binX && map["binY"].toInt() == binY)
                {
                    // Then check for temperature
                    if (targetChip->getCCD()->hasCooler())
                    {
                        double temperature=0;
                        targetChip->getCCD()->getTemperature(&temperature);
                        // TODO make this configurable value, the threshold
                        if (fabs(map["temperature"].toDouble()-temperature) > 0.1)
                            continue;
                    }

                    // Then check for duration
                    // TODO make this value configurable
                    if (fabs(map["duration"].toDouble() - duration) > 0.05)
                        continue;

                    // Finaly check if the duration is acceptable
                    QDateTime frameTime = QDateTime::fromString(map["timestamp"].toString(), Qt::ISODate);
                    if (frameTime.daysTo(QDateTime::currentDateTime()) > Options::darkLibraryDuration())
                        continue;

                    QString filename = map["filename"].toString();

                    if (darkFiles.contains(filename))
                        return darkFiles[filename];

                    // Finally we made it, let's put it in the hash
                    bool rc = loadDarkFile(filename);
                    if (rc)
                        return darkFiles[filename];
                    else
                        return NULL;
                }
            }
        }
    }

    return NULL;
}

bool DarkLibrary::loadDarkFile(const QString &filename)
{
    FITSData *darkData = new FITSData();

    bool rc = darkData->loadFITS(filename);

    if (rc)
        darkFiles[filename] = darkData;
    else
    {
        emit newLog(i18n("Failed to load dark frame file %1", filename));
        delete (darkData);
    }

    return rc;
}

bool DarkLibrary::saveDarkFile(FITSData *darkData)
{
    QDateTime ts = QDateTime::currentDateTime();

    QString path     = KSPaths::writableLocation(QStandardPaths::GenericDataLocation) + "darks/darkframe_" + ts.toString(Qt::ISODate) + ".fits";

    if (darkData->saveFITS(path) != 0)
        return false;

    darkFiles[path] = darkData;

    QVariantMap map;
    int binX, binY;
    double temperature=0;

    subtractParams.targetChip->getBinning(&binX, &binY);
    subtractParams.targetChip->getCCD()->getTemperature(&temperature);

    map["ccd"]  = subtractParams.targetChip->getCCD()->getDeviceName();
    map["chip"] = static_cast<int>(subtractParams.targetChip->getType());
    map["binX"] = binX;
    map["binY"] = binY;
    map["temperature"] = temperature;
    map["duration"] = subtractParams.duration;
    map["filename"] = path;

    darkFrames.append(map);

    emit newLog(i18n("Dark frame saved to %1", path));

    KStarsData::Instance()->userdb()->AddDarkFrame(map);

    return true;
}

bool DarkLibrary::subtract(FITSData *darkData, FITSView *lightImage, FITSScale filter, uint16_t offsetX, uint16_t offsetY)
{
   Q_ASSERT(darkData);
   Q_ASSERT(lightImage);

   FITSData *lightData = lightImage->getImageData();

   float *darkBuffer     = darkData->getImageBuffer();
   float *lightBuffer    = lightData->getImageBuffer();

   int darkoffset   = offsetX + offsetY * darkData->getWidth();
   int darkW        = darkData->getWidth();

   int lightOffset  = 0;
   int lightW       = lightData->getWidth();
   int lightH       = lightData->getHeight();

   for (int i=0; i < lightH; i++)
   {
       for (int j=0; j < lightW; j++)
       {
           lightBuffer[j+lightOffset] -= darkBuffer[j+darkoffset];
           if (lightBuffer[j+lightOffset] < 0)
               lightBuffer[j+lightOffset] = 0;
       }

       lightOffset += lightW;
       darkoffset  += darkW;
   }

   lightData->applyFilter(filter);
   lightImage->rescale(ZOOM_KEEP_LEVEL);
   lightImage->updateFrame();

   emit darkFrameCompleted(true);

   return true;
}

void DarkLibrary::captureAndSubtract(ISD::CCDChip *targetChip, FITSView*targetImage, double duration, uint16_t offsetX, uint16_t offsetY)
{
    targetChip->resetFrame();
    targetChip->setCaptureMode(FITS_CALIBRATE);
    targetChip->setFrameType(FRAME_DARK);

    subtractParams.targetChip = targetChip;
    subtractParams.targetImage= targetImage;
    subtractParams.duration   = duration;
    subtractParams.offsetX    = offsetX;
    subtractParams.offsetY    = offsetY;

    connect(targetChip->getCCD(), SIGNAL(BLOBUpdated(IBLOB*)), this, SLOT(newFITS(IBLOB*)));

    emit newLog(i18n("Capturing dark frame..."));

    targetChip->capture(duration);
}

void DarkLibrary::newFITS(IBLOB *bp)
{
    INDI_UNUSED(bp);

    Q_ASSERT(subtractParams.targetChip);

    disconnect(subtractParams.targetChip->getCCD(), SIGNAL(BLOBUpdated(IBLOB*)), this, SLOT(newFITS(IBLOB*)));

    FITSView *calibrationView = subtractParams.targetChip->getImage(FITS_CALIBRATE);

    emit newLog(i18n("Dark frame received."));

    FITSData *calibrationData = new FITSData();

    // Deep copy of the data
    if (calibrationData->loadFITS(calibrationView->getImageData()->getFilename()))
    {
        saveDarkFile(calibrationData);
        subtract(calibrationData, subtractParams.targetImage, subtractParams.targetChip->getCaptureFilter(), subtractParams.offsetX, subtractParams.offsetY);
    }
    else
    {
        emit darkFrameCompleted(false);
        emit newLog(i18n("Warning: Cannot load calibration file %1", calibrationView->getImageData()->getFilename()));
    }
}

}



