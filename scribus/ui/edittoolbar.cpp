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
#include <QFont>
#include <QLabel>
#include <QToolButton>

#include <utility>

#include "pageitem.h"
#include "scraction.h"
#include "scribus.h"
#include "scribusdoc.h"
#include "selection.h"


EditToolBar::EditToolBar(ScribusMainWindow* parent) : ScToolBar(tr("Context"), "Context", parent),
	m_mainWindow(parent)
{
	m_contextLabel = new QLabel(this);
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

	updateForSelection();
}

void EditToolBar::addContextAction(const QString& actionName, int contexts)
{
	ScrAction* action = m_mainWindow->scrActions.value(actionName);
	if (!action)
		return;

	auto* button = new QToolButton(this);
	button->setAutoRaise(true);
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
		setEnabled(false);
		return;
	}

	setEnabled(true);
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
		updateForSelection();
	ScToolBar::changeEvent(event);
}
