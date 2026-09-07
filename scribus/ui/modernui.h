/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef MODERNUI_H
#define MODERNUI_H

#include <QAbstractButton>
#include <QByteArray>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QPushButton>
#include <QStyle>
#include <QWidget>

#include "iconmanager.h"
#include "scpaths.h"
#include "util.h"

namespace ModernUI
{
inline void repolish(QWidget* widget)
{
	if (!widget)
		return;
	widget->style()->unpolish(widget);
	widget->style()->polish(widget);
	widget->update();
}

inline void applySurfaceStyle(QWidget* surface, const char* role, bool dialog = true)
{
	if (!surface)
		return;

	surface->setProperty(dialog ? "modernDialog" : "modernPanel", true);
	surface->setProperty(dialog ? "modernDialogRole" : "modernPanelRole", QString::fromLatin1(role));
	surface->setAttribute(Qt::WA_StyledBackground, true);

	QByteArray styleSheet;
	if (!loadRawText(ScPaths::instance().libDir() + "scribus.css", styleSheet))
		return;

	styleSheet.replace("___downArrow___", IconManager::instance().pathForIcon("stylesheet/go-down.png").toUtf8());
	styleSheet.replace("___tb_menu_arrow___", IconManager::instance().pathForIcon("stylesheet/down_arrow.png").toUtf8());
	surface->setStyleSheet(QString::fromUtf8(styleSheet));
}

inline void markSections(QWidget* surface)
{
	if (!surface)
		return;

	const auto sections = surface->findChildren<QGroupBox*>();
	for (QGroupBox* section : sections)
	{
		section->setProperty("modernSection", true);
		section->setAttribute(Qt::WA_StyledBackground, true);
		repolish(section);
	}
}

inline void setPrimaryAction(QAbstractButton* button)
{
	if (button)
	{
		button->setProperty("primaryAction", true);
		repolish(button);
	}
}

inline void setPrimaryAction(QDialogButtonBox* buttonBox, QDialogButtonBox::StandardButton button)
{
	if (buttonBox)
		setPrimaryAction(buttonBox->button(button));
}
}

#endif
