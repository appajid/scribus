/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ACTIONSEARCH_H
#define ACTIONSEARCH_H

class QAction;
class QMenu;
class QMenuBar;

#include <QHash>
#include <QIcon>
#include <QList>
#include <QPointer>
#include <QString>
#include <QStringList>

#include "scribusapi.h"

class SCRIBUS_API ActionSearch
{
public:
	struct ActionInfo
	{
		QString id;
		QString name;
		QString menuPath;
		QString shortcut;
		QIcon icon;
		bool enabled {false};
	};

	ActionSearch(QMenuBar *menuBar);
	~ActionSearch() = default;

	const QList<ActionInfo>& actions() const { return m_actionInfo; }
	QList<QString> getActionNames() const { return m_legacyActionNames; }

	void update();
	void execute(const QString& actionKey);
	bool tryExecute(const QString& actionKey);

private:
	QMenuBar* m_menuBar {nullptr};

	void readMenuActions(QMenu* menu, const QStringList& parentMenus = {});

	QList<ActionInfo> m_actionInfo;
	QList<QString> m_legacyActionNames;
	QHash<QString, QPointer<QAction>> m_actions;
};

#endif
