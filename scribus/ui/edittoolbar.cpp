/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          texttoolb.cpp  -  description
                             -------------------
    begin                : Sun Mar 10 2002
    copyright            : (C) 2002 by Franz Schmid
    email                : Franz.Schmid@altmuehlnet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "edittoolbar.h"

#include <QEvent>
#include <QDebug>
#include <QFont>
#include <QGuiApplication>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QStyleHints>
#include <QTimer>
#include <QToolButton>
#include <QWidget>

#include <utility>

#include "iconmanager.h"
#include "pageitem.h"
#include "prefsmanager.h"
#include "scraction.h"
#include "scribus.h"
#include "scribusapp.h"
#include "scribusdoc.h"
#include "selection.h"
#include "ui/factories/scribusproxystyle.h"


EditToolBar::EditToolBar(ScribusMainWindow* parent) : ScToolBar(tr("Context"), "Context", parent),
	m_mainWindow(parent)
{
	setProperty("contextToolbar", true);
	setMinimumHeight(44);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	setAllowedAreas(Qt::TopToolBarArea);
	setFloatable(false);
	setMovable(false);

	m_contextLabel = new QLabel(this);
	m_contextLabel->setObjectName(QStringLiteral("contextToolbarTitle"));
	QFont labelFont(m_contextLabel->font());
	labelFont.setBold(true);
	m_contextLabel->setFont(labelFont);
	m_contextLabel->setMinimumWidth(90);
	m_contextLabel->setContentsMargins(6, 0, 8, 0);
	addWidget(m_contextLabel);
	addSeparator();

	addContextAction("pageManageProperties", NoSelection);
	addContextAction("fileDocSetup150", NoSelection);

	addContextAction("toolsEditWithStoryEditor", TextSelection);
	addContextAction("itemAdjustFrameHeightToText", TextSelection);
	addContextAction("toolsLinkTextFrame", TextSelection);
	addContextAction("toolsUnlinkTextFrame", TextSelection);

	addContextAction("itemAdjustFrameToImage", ImageSelection);
	addContextAction("itemAdjustImageToFrame", ImageSelection);
	addContextAction("itemUpdateImage", ImageSelection);
	addContextAction("styleImageEffects", ImageSelection);

	addContextAction("toolsProperties", ShapeSelection);
	addContextAction("itemConvertToBezierCurve", ShapeSelection);
	addContextAction("itemLockAspectRatio", ShapeSelection);

	addContextAction("toolsAlignDistribute", MultipleSelection);
	addContextAction("itemGroup", MultipleSelection);

	// The approved workspace keeps file and contextual controls in a single
	// compact row. Remove the legacy toolbar break after QMainWindow has added us.
	QTimer::singleShot(0, this, [this]() {
		if (auto* mainWindow = qobject_cast<QMainWindow*>(parentWidget()))
			mainWindow->removeToolBarBreak(this);
	});

	auto* appearanceSpacer = new QWidget(this);
	appearanceSpacer->setObjectName(QStringLiteral("contextToolbarSpacer"));
	appearanceSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	addWidget(appearanceSpacer);

	m_lightAppearanceButton = new QToolButton(this);
	m_lightAppearanceButton->setObjectName(QStringLiteral("appearanceButton"));
	m_lightAppearanceButton->setCheckable(true);
	m_lightAppearanceButton->setAutoExclusive(true);
	m_lightAppearanceButton->setFixedSize(34, 32);
	m_lightAppearanceButton->setIconSize(QSize(20, 20));
	m_lightAppearanceButton->setToolTip(tr("Use Light Appearance"));
	m_lightAppearanceButton->setAccessibleName(tr("Light Appearance"));
	addWidget(m_lightAppearanceButton);

	m_darkAppearanceButton = new QToolButton(this);
	m_darkAppearanceButton->setObjectName(QStringLiteral("appearanceButton"));
	m_darkAppearanceButton->setCheckable(true);
	m_darkAppearanceButton->setAutoExclusive(true);
	m_darkAppearanceButton->setFixedSize(34, 32);
	m_darkAppearanceButton->setIconSize(QSize(20, 20));
	m_darkAppearanceButton->setToolTip(tr("Use Dark Appearance"));
	m_darkAppearanceButton->setAccessibleName(tr("Dark Appearance"));
	addWidget(m_darkAppearanceButton);

	connect(m_lightAppearanceButton, &QToolButton::clicked, this, &EditToolBar::setLightAppearance);
	connect(m_darkAppearanceButton, &QToolButton::clicked, this, &EditToolBar::setDarkAppearance);
	connect(ScQApp, &ScribusQApp::iconSetChanged, this, &EditToolBar::updateAppearanceButtons);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
	connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &EditToolBar::updateAppearanceButtons);
#endif
	updateAppearanceButtons();

	if (qEnvironmentVariableIsSet("SCRIBUS_APPEARANCE_SELFTEST"))
	{
		QTimer::singleShot(150, this, [this]() {
			const QString originalAppearance = PrefsManager::instance().appPrefs.uiPrefs.stylePalette;
			if (auto* mainWindow = qobject_cast<QMainWindow*>(parentWidget()))
				qInfo().noquote() << "[workspace-test] contextual-toolbar"
					<< "| singleRow=" << !mainWindow->toolBarBreak(this)
					<< "| appearanceControls=" << (m_lightAppearanceButton && m_darkAppearanceButton);
			m_lightAppearanceButton->click();
			qInfo().noquote() << "[appearance-test] light"
				<< "| preference=" << PrefsManager::instance().appPrefs.uiPrefs.stylePalette
				<< "| checked=" << m_lightAppearanceButton->isChecked();
			m_darkAppearanceButton->click();
			qInfo().noquote() << "[appearance-test] dark"
				<< "| preference=" << PrefsManager::instance().appPrefs.uiPrefs.stylePalette
				<< "| checked=" << m_darkAppearanceButton->isChecked();
			setAppearance(originalAppearance);
			qInfo().noquote() << "[appearance-test] restored"
				<< "| preference=" << PrefsManager::instance().appPrefs.uiPrefs.stylePalette;
		});
	}

	updateForSelection();
}

void EditToolBar::setLightAppearance()
{
	setAppearance(QStringLiteral("light"));
}

void EditToolBar::setDarkAppearance()
{
	setAppearance(QStringLiteral("dark"));
}

void EditToolBar::setAppearance(const QString& appearance)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
	PrefsManager::instance().appPrefs.uiPrefs.stylePalette = appearance;
	ScribusProxyStyle::ApplicationTheme theme = ScribusProxyStyle::ApplicationTheme::System;
	if (appearance == QStringLiteral("dark"))
		theme = ScribusProxyStyle::ApplicationTheme::Dark;
	else if (appearance == QStringLiteral("light"))
		theme = ScribusProxyStyle::ApplicationTheme::Light;
	ScribusProxyStyle::instance()->setApplicationTheme(theme);
	updateAppearanceButtons();
#else
	Q_UNUSED(appearance);
#endif
}

void EditToolBar::updateAppearanceButtons()
{
	if (!m_lightAppearanceButton || !m_darkAppearanceButton)
		return;

	auto statefulIcon = [](const QString& iconName) {
		const QSize iconSize(20, 20);
		QPixmap normal = IconManager::instance().loadPixmap(iconName, iconSize.width());
		QPixmap selected = normal;
		QPainter painter(&selected);
		painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
		painter.fillRect(selected.rect(), QApplication::palette().color(QPalette::Highlight));
		painter.end();
		QIcon icon;
		icon.addPixmap(normal, QIcon::Normal, QIcon::Off);
		icon.addPixmap(selected, QIcon::Normal, QIcon::On);
		icon.addPixmap(selected, QIcon::Active, QIcon::On);
		return icon;
	};

	m_lightAppearanceButton->setIcon(statefulIcon(QStringLiteral("appearance-light")));
	m_darkAppearanceButton->setIcon(statefulIcon(QStringLiteral("appearance-dark")));
	const QString appearance = PrefsManager::instance().appPrefs.uiPrefs.stylePalette;
	m_lightAppearanceButton->setChecked(appearance == QStringLiteral("light"));
	m_darkAppearanceButton->setChecked(appearance == QStringLiteral("dark"));
}

void EditToolBar::addContextAction(const QString& actionName, int contexts)
{
	ScrAction* action = m_mainWindow->scrActions.value(actionName);
	if (!action)
		return;

	auto* button = new QToolButton(this);
	button->setObjectName(QStringLiteral("contextToolButton"));
	button->setAutoRaise(true);
	button->setIconSize(QSize(20, 20));
	button->setFixedSize(36, 32);
	button->setToolButtonStyle(Qt::ToolButtonIconOnly);
	button->setDefaultAction(action);
	addWidget(button);
	m_contextButtons.append(qMakePair(button, contexts));
}

void EditToolBar::setDoc(ScribusDoc* doc)
{
	if (m_doc == doc)
	{
		updateForSelection();
		return;
	}
	if (m_doc)
		disconnect(m_doc->m_Selection, &Selection::selectionChanged, this, &EditToolBar::updateForSelection);

	m_doc = doc;
	if (m_doc)
		connect(m_doc->m_Selection, &Selection::selectionChanged, this, &EditToolBar::updateForSelection);
	updateForSelection();
}

void EditToolBar::updateForSelection()
{
	if (!m_doc)
	{
		setContext(NoSelection, tr("No document"));
		return;
	}

	const int selectionCount = m_doc->m_Selection->count();
	if (selectionCount == 0)
	{
		setContext(NoSelection, tr("Page"));
		return;
	}
	if (selectionCount > 1)
	{
		setContext(MultipleSelection, tr("%n objects", nullptr, selectionCount));
		return;
	}

	const PageItem* item = m_doc->m_Selection->itemAt(0);
	if (item->isTextFrame() || item->isPathText() || item->isTable())
		setContext(TextSelection, item->isTable() ? tr("Table") : tr("Text"));
	else if (item->isImageFrame())
		setContext(ImageSelection, tr("Image"));
	else
		setContext(ShapeSelection, tr("Shape"));
}

void EditToolBar::setContext(int context, const QString& labelText)
{
	m_contextLabel->setText(labelText);
	for (const auto& entry : std::as_const(m_contextButtons))
		entry.first->setVisible(entry.second & context);
}

void EditToolBar::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
	{
		m_lightAppearanceButton->setToolTip(tr("Use Light Appearance"));
		m_lightAppearanceButton->setAccessibleName(tr("Light Appearance"));
		m_darkAppearanceButton->setToolTip(tr("Use Dark Appearance"));
		m_darkAppearanceButton->setAccessibleName(tr("Dark Appearance"));
		updateForSelection();
	}
	ScToolBar::changeEvent(event);
}
