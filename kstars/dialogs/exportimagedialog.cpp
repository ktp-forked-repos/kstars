/***************************************************************************
                          exportimagedialog.cpp  -  K Desktop Planetarium
                             -------------------
    begin                : Mon Jun 13 2011
    copyright            : (C) 2011 by Rafał Kułaga
    email                : rl.kulaga@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "exportimagedialog.h"
#include "kstars/kstars.h"
#include "kstars/skymap.h"
#include "../printing/legend.h"
#include "kstars/skyqpainter.h"
#include "../imageexporter.h"

#include <KMessageBox>

#include <QtSvg/QSvgGenerator>
#include <QDir>
#include <QDesktopWidget>


ExportImageDialogUI::ExportImageDialogUI(QWidget *parent)
    : QFrame(parent)
{
    setupUi(this);
}

ExportImageDialog::ExportImageDialog(const QString &url, const QSize &size, ImageExporter *imgExporter)
    : QDialog((QWidget*) KStars::Instance()), m_KStars(KStars::Instance()), m_Url(url), m_Size(size)
{
    m_DialogUI = new ExportImageDialogUI(this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_DialogUI);
    setLayout(mainLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(exportImage()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    QPushButton *previewB = new QPushButton(xi18n("Preview image"));
    buttonBox->addButton(previewB, QDialogButtonBox::ActionRole);
    connect(previewB, SIGNAL(clicked()), this, SLOT(previewImage()));

    connect(m_DialogUI->addLegendCheckBox, SIGNAL(toggled(bool)), this, SLOT(switchLegendEnabled(bool)));
    connect(m_DialogUI->addLegendCheckBox, SIGNAL(toggled(bool)), previewB, SLOT(setEnabled(bool)));

    m_ImageExporter = ( ( imgExporter ) ? imgExporter : new ImageExporter( this ) );

    setWindowTitle(xi18n("Export sky image"));

    setupWidgets();
}

void ExportImageDialog::switchLegendEnabled(bool enabled)
{
    m_DialogUI->legendOrientationLabel->setEnabled(enabled);
    m_DialogUI->legendOrientationComboBox->setEnabled(enabled);
    m_DialogUI->legendTypeLabel->setEnabled(enabled);
    m_DialogUI->legendTypeComboBox->setEnabled(enabled);
    m_DialogUI->legendPositionLabel->setEnabled(enabled);
    m_DialogUI->legendPositionComboBox->setEnabled(enabled);
}

void ExportImageDialog::previewImage()
{
    updateLegendSettings();
    const Legend *legend = m_ImageExporter->getLegend();

    // Preview current legend settings on sky map
    m_KStars->map()->setLegend( Legend( *legend ) );
    m_KStars->map()->setPreviewLegend(true);

    // Update sky map
    m_KStars->map()->forceUpdate(true);

    // Hide export dialog
    hide();
}

void ExportImageDialog::setupWidgets()
{    
    m_DialogUI->addLegendCheckBox->setChecked(true);

    m_DialogUI->legendOrientationComboBox->addItem(xi18n("Horizontal"));
    m_DialogUI->legendOrientationComboBox->addItem(xi18n("Vertical"));

    QStringList types;
    types << xi18n("Full legend") << xi18n("Scale with magnitudes chart") << xi18n("Only scale")
            << xi18n("Only magnitudes") << xi18n("Only symbols");
    m_DialogUI->legendTypeComboBox->addItems(types);

    QStringList positions;
    positions << xi18n("Upper left corner") << xi18n("Upper right corner") << xi18n("Lower left corner")
            << xi18n("Lower right corner");
    m_DialogUI->legendPositionComboBox->addItems(positions);
}

void ExportImageDialog::updateLegendSettings()
{
    Legend::LEGEND_ORIENTATION orientation = ( ( m_DialogUI->legendOrientationComboBox->currentIndex() == 1) ? Legend::LO_VERTICAL : Legend::LO_HORIZONTAL );

    Legend::LEGEND_TYPE type = static_cast<Legend::LEGEND_TYPE>(m_DialogUI->legendTypeComboBox->currentIndex());

    Legend::LEGEND_POSITION pos = static_cast<Legend::LEGEND_POSITION>(m_DialogUI->legendPositionComboBox->currentIndex());

    m_ImageExporter->setLegendProperties( type, orientation, pos );
}

void ExportImageDialog::exportImage()
{
    qDebug() << "Exporting sky image";
    updateLegendSettings();
    m_ImageExporter->includeLegend( m_DialogUI->addLegendCheckBox->isChecked() );
    if( !m_ImageExporter->exportImage( m_Url ) ) {
        KMessageBox::sorry( 0, m_ImageExporter->getLastErrorMessage(), xi18n( "Could not export image" ) );
    }
}
