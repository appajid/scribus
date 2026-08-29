/***************************************************************************
 *   Copyright (C) 2023 by Martin Reininger                                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

/*
For general Scribus copyright and licensing information please refer
to the COPYING file provided with the program.
*/


#include "dock_manager.h"

#include <QMenu>
#include "third_party/Qt-Advanced-Docking-System/src/DockAreaWidget.h"
#include "ui/docks/dock_centralwidget.h"

#include "iconmanager.h"
#include "prefsmanager.h"
#include "scribusapp.h"
#include "ui/aligndistribute.h"
#include "ui/bookmarkpalette.h"
#include "ui/contentpalette.h"
#include "ui/inlinepalette.h"
#include "ui/layers.h"
#include "ui/outlinepalette.h"
#include "ui/pagepalette.h"
#include "ui/propertiespalette.h"
#include "ui/scrapbookpalette.h"
#include "ui/symbolpalette.h"
#include "ui/toolpalette.h"
#include "undogui.h"


/* ********************************************************************************* *
 *
 * Constructor + Setup
 *
 * ********************************************************************************* */

DockManager::DockManager(QWidget *parent)
	: CDockManager(parent)
{
	dockCenter = new DockCentralWidget();
	auto *areaCenter = CDockManager::setCentralWidget(dockCenter);
	areaCenter->setAllowedAreas(LeftDockWidgetArea | RightDockWidgetArea);

	setStyleSheet(""); // reset style sheet to use custom icons

	connect( ScQApp, SIGNAL(iconSetChanged()), this, SLOT(iconSetChange()) );
}

void DockManager::setupDocks()
{
	alignDistributePalette = new AlignDistributePalette();
	bookPalette = new BookPalette(this);
	contentPalette = new ContentPalette(this);
	inlinePalette = new InlinePalette(this);
	layerPalette = new LayerPalette(this);
	outlinePalette = new OutlinePalette(this);
	pagePalette = new PagePalette((QWidget *) this->parent());
	propertiesPalette = new PropertiesPalette(this);
	scrapbookPalette = new Biblio(this);
	symbolPalette = new SymbolPalette(this);
	toolPalette = new ToolPalette((QWidget *) this->parent());
	undoPalette = new UndoPalette(this);

	// Apply common configuration for each palette
	configureDock(alignDistributePalette);
	configureDock(bookPalette);
	configureDock(contentPalette);
	configureDock(inlinePalette);
	configureDock(layerPalette);
	configureDock(outlinePalette);
	configureDock(pagePalette);
	configureDock(propertiesPalette);
	configureDock(scrapbookPalette);
	configureDock(symbolPalette);
	configureDock(toolPalette);
	configureDock(undoPalette);

	connect(contentPalette, &ContentPalette::inspectorTargetChanged, this, [this](int target) {
		if (m_dockTemporaryHidden)
			return;
		if (propertiesPalette->isClosed() && contentPalette->isClosed() && alignDistributePalette->isClosed())
			return;
		if (auto* inspectorArea = propertiesPalette->dockAreaWidget())
		{
			auto* currentDock = inspectorArea->currentDockWidget();
			if (currentDock != propertiesPalette && currentDock != contentPalette && currentDock != alignDistributePalette)
				return;
		}

		CDockWidget* targetDock = contentPalette;
		if (target == ContentPalette::InspectorAppearance)
			targetDock = propertiesPalette;
		else if (target == ContentPalette::InspectorAlignment)
			targetDock = alignDistributePalette;

		if (targetDock->isClosed())
			targetDock->toggleView(true);
		if (auto* area = targetDock->dockAreaWidget())
			area->setCurrentDockWidget(targetDock);
	});

	// Panel ToolProperties
	//    PanelToolProperties * panelTest = new PanelToolProperties();
	//    dockToolProperties->setWidget(panelTest);
	//    dockToolProperties->setFeature(ads::CDockWidget::NoTab, true);

}

void DockManager::setCentralWidget(QWidget *widget)
{
	dockCenter->setWidget(widget);
}

/* ********************************************************************************* *
 *
 * Public Methods
 *
 * ********************************************************************************* */

void DockManager::initWorkspaces()
{
	createDefaultWorkspace();
//	restoreWorkspaceFromPrefs();
}

void DockManager::removeAllDockWidgets()
{
	QMap<QString, CDockWidget *> map = dockWidgetsMap();
	foreach (QString key, map.keys())
		removeDockWidget(map.value(key));
}

void DockManager::hideAllDocks()
{
	foreach (CDockWidget *dock, this->dockWidgets())
	{
		if(dock->isCentralWidget())
			continue;
		dock->closeDockWidget();
	}

	foreach (CFloatingDockContainer *dockContainer, this->floatingWidgets())
	{
		dockContainer->close();
	}
}

void DockManager::toggleDocksVisibility()
{
	QString perspective = "_allpaletteshidden";

	if(perspectiveNames().contains(perspective))
	{
		openPerspective(perspective);
		removePerspective(perspective);
		m_dockTemporaryHidden = false;
	}
	else
	{
		addPerspective(perspective);
		hideAllDocks();
		m_dockTemporaryHidden = true;

//		qDebug() << Q_FUNC_INFO << this->dockWidgets();
//		qDebug() << Q_FUNC_INFO << this->floatingWidgets();
	}

}

bool DockManager::hasTemporaryHiddenDocks()
{
	return m_dockTemporaryHidden;
}

bool DockManager::resetWorkspaceToDefault()
{
	const QString defaultWorkspace = QStringLiteral("Default");
	if (!perspectiveNames().contains(defaultWorkspace))
		return false;

	restoreHiddenWorkspace();
	openPerspective(defaultWorkspace);
	m_dockTemporaryHidden = false;
	saveWorkspaceToPrefs();
	return true;
}

CDockAreaWidget *DockManager::addDockFromPlugin(CDockWidget *dock, bool closed)
{
	CDockAreaWidget *a = addDockWidget(RightDockWidgetArea, dock, dockCenter->dockAreaWidget());
	dock->toggleView(!closed);	
	updateIcon(dock);
	configureDock(dock);
	return a;
}

void DockManager::restoreWorkspaceFromPrefs()
{
	QByteArray ba = PrefsManager::instance().appPrefs.uiPrefs.adsDockState;

	if(ba.isEmpty())
		return;

	this->restoreState(ba);
}

void DockManager::saveWorkspaceToPrefs()
{
	PrefsManager::instance().appPrefs.uiPrefs.adsDockState = this->saveState();
}

void DockManager::iconSetChange()
{
	// Update all icons of each docked CDockContainer
	updateIcons(this->dockWidgets());

	// Update all icons of each floating CFloatingDockContainer
	foreach (CFloatingDockContainer *dockContainer, this->floatingWidgets())
		updateIcons(dockContainer->dockWidgets());
}

void DockManager::restoreHiddenWorkspace()
{
	if (m_dockTemporaryHidden)
		toggleDocksVisibility();
}

void DockManager::updateIcons(QList<CDockWidget *> dockWidgets)
{
	foreach (CDockWidget *dock, dockWidgets)
		updateIcon(dock);
}

void DockManager::updateIcon(CDockWidget *dockWidget)
{
	IconManager &iconManager = IconManager::instance();

	dockWidget->dockAreaWidget()->titleBarButton(ads::TitleBarButtonClose)->setIcon(iconManager.loadIcon("close", 12));
	dockWidget->dockAreaWidget()->titleBarButton(ads::TitleBarButtonUndock)->setIcon(iconManager.loadIcon("dock-float", 16));
	dockWidget->dockAreaWidget()->titleBarButton(ads::TitleBarButtonTabsMenu)->setIcon(iconManager.loadIcon("menu-down", 16));
	dockWidget->dockAreaWidget()->titleBarButton(ads::TitleBarButtonAutoHide)->setIcon(iconManager.loadIcon("dock-auto-hide", 16));
	dockWidget->dockAreaWidget()->titleBarButton(ads::TitleBarButtonMinimize)->setIcon(iconManager.loadIcon("dock-minimize", 16));

	for (auto tabCloseButton : dockWidget->dockAreaWidget()->findChildren<QAbstractButton*>("tabCloseButton"))
		tabCloseButton->setIcon(iconManager.loadIcon("close", 12));
}

void DockManager::configureDock(CDockWidget *dock)
{
	/*
	* MinimumSizeHintFromContent
	* MinimumSizeHintFromDockWidgetMinimumSize
	* MinimumSizeHintFromContentMinimumSize
	*/

	dock->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidgetMinimumSize);
	dock->setMinimumWidth(dock->widget()->minimumSizeHint().width());
	connect( dock, &CDockWidget::viewToggled, this, &DockManager::restoreHiddenWorkspace);
}

void DockManager::createDefaultWorkspace()
{
	/************************************************************
	 *
	 *
	 *      LAYOUT SCHEME
	 *
	 *       96px                *                  340px
	 *     |------|-------------------------------|---------|
	 *     |Tools |        Document canvas        |Context  |
	 *     |      |                               |inspector|
	 *     |      |                               |         |
	 *     |------|-------------------------------|---------|
	 *
	 *
	 *************************************************************/

	auto *areaCenter = dockCenter->dockAreaWidget();

	// Compact, function-grouped tools palette on the left.
	auto *areaToolbox = addDockWidget(LeftDockWidgetArea, toolPalette, areaCenter);

	// Contextual inspector on the right. Existing editors share one dock
	// area and are selected automatically as the document selection changes.
	auto *areaRight = addDockWidget(RightDockWidgetArea, propertiesPalette, areaCenter);
	addDockWidgetTabToArea(contentPalette, areaRight);
	addDockWidgetTabToArea(alignDistributePalette, areaRight);

	// Utility panels stay available from the Windows menu. Dock them as
	// secondary right-side tabs so opening one never narrows the canvas again.
	addDockWidgetTabToArea(pagePalette, areaRight);
	addDockWidgetTabToArea(outlinePalette, areaRight);
	addDockWidgetTabToArea(layerPalette, areaRight);
	addDockWidgetTabToArea(undoPalette, areaRight);
	addDockWidgetTabToArea(inlinePalette, areaRight);
	addDockWidgetTabToArea(scrapbookPalette, areaRight);
	addDockWidgetTabToArea(bookPalette, areaRight);
	addDockWidgetTabToArea(symbolPalette, areaRight);

	const int workspaceWidth = qMax(areaCenter->width(), 900);
	const int toolboxWidth = 96;
	const int inspectorWidth = 340;
	setSplitterSizes(areaCenter, {toolboxWidth, workspaceWidth - toolboxWidth - inspectorWidth, inspectorWidth});


	// hide panels that are not visible in default workspace
	inlinePalette->closeDockWidget();
	scrapbookPalette->closeDockWidget();
	bookPalette->closeDockWidget();
	symbolPalette->closeDockWidget();
	layerPalette->closeDockWidget();
	undoPalette->closeDockWidget();
	outlinePalette->closeDockWidget();
	pagePalette->closeDockWidget();

	// active palettes
	areaToolbox->setCurrentDockWidget(toolPalette);
	areaRight->setCurrentDockWidget(contentPalette);

	// addDockWidget() does not open the docks, open the active ones explicitly
	toolPalette->toggleView(true);
	propertiesPalette->toggleView(true);
	contentPalette->toggleView(true);
	alignDistributePalette->toggleView(true);
	areaRight->setCurrentDockWidget(contentPalette);

	// add perspective for a later usage, like reset workspace to default.
	this->addPerspective("Default");
}
