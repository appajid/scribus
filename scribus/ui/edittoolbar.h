/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          texttoolb.h  -  description
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

#ifndef EDITTOOLBAR_H
#define EDITTOOLBAR_H

#include "scribusapi.h"
#include "ui/sctoolbar.h"

#include <QList>
#include <QPair>

class QAction;
class QEvent;
class QLabel;
class QToolButton;
class PageItem;
class ScribusDoc;
class ScribusMainWindow;

class SCRIBUS_API EditToolBar : public ScToolBar
{
	Q_OBJECT

public:
	EditToolBar(ScribusMainWindow* parent);
	~EditToolBar() {};

	void setDoc(ScribusDoc* doc);

public slots:
	void updateForSelection();

protected:
	void changeEvent(QEvent* event) override;

private:
	enum Context
	{
		NoSelection = 1,
		TextSelection = 2,
		ImageSelection = 4,
		ShapeSelection = 8,
		MultipleSelection = 16
	};

	void addContextAction(const QString& actionName, int contexts);
	void setContext(int context, const QString& labelText);

	ScribusMainWindow* m_mainWindow { nullptr };
	ScribusDoc* m_doc { nullptr };
	QLabel* m_contextLabel { nullptr };
	QList<QPair<QToolButton*, int>> m_contextButtons;
};

#endif
