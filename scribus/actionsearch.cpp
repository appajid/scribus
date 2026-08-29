/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "actionsearch.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QStringList>

ActionSearch::ActionSearch(QMenuBar *menuBar)
	: m_menuBar(menuBar)
{
}

void ActionSearch::update()
{
	m_actions.clear();
	m_actionInfo.clear();
	m_legacyActionNames.clear();

	if (!m_menuBar)
		return;

	for (QAction* menuAction : m_menuBar->actions())
		readMenuActions(menuAction->menu());
}

void ActionSearch::execute(const QString& actionKey)
{
	tryExecute(actionKey);
}

bool ActionSearch::tryExecute(const QString& actionKey)
{
	QAction* action = m_actions.value(actionKey);
	if (!action || !action->isEnabled() || !action->isVisible())
		return false;

	action->trigger();
	return true;
}

void ActionSearch::readMenuActions(QMenu* menu, const QStringList& parentMenus)
{
	if (!menu)
		return;

	QStringList menus(parentMenus);
	const QString menuTitle = QString(menu->title()).remove(QLatin1Char('&')).trimmed();
	if (!menuTitle.isEmpty())
		menus.append(menuTitle);

	for (QAction* action : menu->actions())
	{
		if (action->menu())
		{
			readMenuActions(action->menu(), menus);
			continue;
		}

		if (action->isSeparator() || !action->isVisible())
			continue;

		const QString actionName = QString(action->text()).remove(QLatin1Char('&')).trimmed();
		if (actionName.isEmpty())
			continue;

		ActionInfo info;
		info.id = QString::number(m_actionInfo.size());
		info.name = actionName;
		info.menuPath = menus.join(QStringLiteral(" > "));
		info.shortcut = action->shortcut().toString(QKeySequence::NativeText);
		info.icon = action->icon();
		info.enabled = action->isEnabled();
		m_actionInfo.append(info);
		m_actions.insert(info.id, action);

		// Preserve the original public, name-based API for scripts or plugins
		// that use ActionSearch directly. The Quick Actions dialog uses the
		// collision-safe ID above.
		if (info.enabled)
		{
			QString legacyName = info.name;
			if (!info.menuPath.isEmpty())
				legacyName += QStringLiteral(" (") + info.menuPath + QLatin1Char(')');
			m_legacyActionNames.append(legacyName);
			m_actions.insert(legacyName, action);
		}
	}
}
