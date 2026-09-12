/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "smobjectstyle.h"

#include <algorithm>

#include <QTabWidget>

#include "commonstrings.h"
#include "pageitem.h"
#include "scribus.h"
#include "scribusdoc.h"
#include "selection.h"
#include "smobjectstylewidget.h"
#include "util.h"

SMObjectStyle::SMObjectStyle()
{
	m_widget = new QTabWidget();
	m_widget->setContentsMargins(5, 5, 5, 5);
	m_page = new SMObjectStyleWidget(m_widget);
	m_widget->addTab(m_page, tr("Properties"));
	connect(m_page, &SMObjectStyleWidget::changed, this, &SMObjectStyle::slotWidgetChanged);
}

SMObjectStyle::~SMObjectStyle()
{
	delete m_widget;
}

QTabWidget* SMObjectStyle::widget()
{
	return m_widget;
}

QString SMObjectStyle::typeNamePlural()
{
	return tr("Object Styles");
}

QString SMObjectStyle::typeNameSingular()
{
	return tr("Object Style");
}

QString SMObjectStyle::internalName(const QString& displayName) const
{
	return displayName == CommonStrings::trDefaultObjectStyle ? CommonStrings::DefaultObjectStyle : displayName;
}

void SMObjectStyle::setCurrentDoc(ScribusDoc* doc)
{
	m_doc = doc;
	m_page->setDoc(doc);
	if (!m_doc)
	{
		m_selection.clear();
		m_tmpStyles.clear();
	}
}

QList<StyleName> SMObjectStyle::styles(bool reloadFromDoc)
{
	QList<StyleName> result;
	if (!m_doc)
		return result;
	if (reloadFromDoc)
		updateStylesCache();
	m_tmpStyles.invalidate();
	for (int i = 0; i < m_tmpStyles.count(); ++i)
	{
		const ObjectStyle& style = m_tmpStyles[i];
		if (!style.hasName())
			continue;
		QString parentName = style.parent();
		if (parentName == CommonStrings::DefaultObjectStyle)
			parentName = CommonStrings::trDefaultObjectStyle;
		result.append(StyleName(style.displayName(), parentName));
	}
	std::sort(result.begin(), result.end(), sortingQPairOfStrings);
	return result;
}

void SMObjectStyle::reload()
{
	updateStylesCache();
}

void SMObjectStyle::selected(const QStringList& styleNames)
{
	m_selection.clear();
	m_selectionIsDirty = false;
	m_tmpStyles.invalidate();
	for (const QString& displayName : styleNames)
	{
		const int index = m_tmpStyles.find(internalName(displayName));
		if (index >= 0)
			m_selection.append(&m_tmpStyles[index]);
	}
	if (m_selection.count() == 1)
	{
		QList<ObjectStyle> allStyles;
		for (int i = 0; i < m_tmpStyles.count(); ++i)
			allStyles.append(m_tmpStyles[i]);
		m_page->showStyle(m_selection.first(), allStyles);
	}
	else
		m_page->showMultipleStyles();
}

QString SMObjectStyle::fromSelection() const
{
	if (!m_doc || !m_doc->m_Selection || m_doc->m_Selection->isEmpty())
		return QString();
	QString styleName = m_doc->m_Selection->itemAt(0)->objectStyleName();
	for (int i = 1; i < m_doc->m_Selection->count(); ++i)
	{
		if (m_doc->m_Selection->itemAt(i)->objectStyleName() != styleName)
			return QString();
	}
	return styleName == CommonStrings::DefaultObjectStyle ? CommonStrings::trDefaultObjectStyle : styleName;
}

void SMObjectStyle::toSelection(const QString& styleName) const
{
	if (m_doc)
		m_doc->itemSelection_SetNamedObjectStyle(internalName(styleName));
}

QString SMObjectStyle::newStyle()
{
	Q_ASSERT(m_doc);
	ObjectStyle style;
	style.setName(getUniqueName(tr("New Style")));
	style.setDefaultStyle(false);
	m_tmpStyles.create(style);
	return style.name();
}

QString SMObjectStyle::newStyle(const QString& fromStyle)
{
	const QString sourceName = internalName(fromStyle);
	if (!m_tmpStyles.resolve(sourceName))
		return QString();
	ObjectStyle style(m_tmpStyles.get(sourceName));
	style.setName(getUniqueName(fromStyle));
	style.setDefaultStyle(false);
	style.setShortcut(QString());
	m_tmpStyles.create(style);
	return style.name();
}

void SMObjectStyle::apply()
{
	if (!m_doc)
		return;
	QMap<QString, QString> replacement;
	for (const RemoveItem& removed : m_deleted)
	{
		if (removed.first != removed.second)
			replacement.insert(removed.first, internalName(removed.second));
	}
	m_doc->applyObjectStyleChanges(m_tmpStyles, replacement);
	m_deleted.clear();
	m_selectionIsDirty = false;
}

void SMObjectStyle::editMode(bool isOn)
{
	if (isOn)
		updateStylesCache();
}

bool SMObjectStyle::isDefaultStyle(const QString& styleName) const
{
	const int index = m_tmpStyles.find(internalName(styleName));
	return index >= 0 && m_tmpStyles[index].isDefaultStyle();
}

void SMObjectStyle::setDefaultStyle(bool isDefaultStyle)
{
	if (m_selection.count() != 1)
		return;
	m_selection.first()->setDefaultStyle(isDefaultStyle);
	markDirty();
}

QString SMObjectStyle::shortcut(const QString& styleName) const
{
	const int index = m_tmpStyles.find(internalName(styleName));
	return index >= 0 ? m_tmpStyles[index].shortcut() : QString();
}

void SMObjectStyle::setShortcut(const QString& shortcut)
{
	if (m_selection.count() != 1)
		return;
	m_selection.first()->setShortcut(shortcut);
	markDirty();
}

void SMObjectStyle::deleteStyles(const QList<RemoveItem>& removeList)
{
	for (const RemoveItem& removed : removeList)
	{
		const QString removedName = internalName(removed.first);
		const QString replacementName = internalName(removed.second);
		for (int i = m_selection.count() - 1; i >= 0; --i)
		{
			if (m_selection.at(i)->name() == removedName)
				m_selection.removeAt(i);
		}
		const int index = m_tmpStyles.find(removedName);
		if (index >= 0)
			m_tmpStyles.remove(index);
		m_deleted.append(RemoveItem(removedName, replacementName));
	}
	for (int i = 0; i < m_tmpStyles.count(); ++i)
	{
		ObjectStyle& style = m_tmpStyles[i];
		for (const RemoveItem& removed : removeList)
		{
			if (style.parent() != internalName(removed.first))
				continue;
			QString replacement = internalName(removed.second);
			if (replacement.isEmpty() || m_tmpStyles.find(replacement) < 0 || !style.canInherit(replacement))
				replacement.clear();
			style.setParent(replacement);
			break;
		}
	}
}

void SMObjectStyle::nameChanged(const QString& newName)
{
	if (m_selection.count() != 1)
		return;
	QString oldName = m_selection.first()->name();
	ObjectStyle renamed(*m_selection.first());
	renamed.setName(newName);
	m_tmpStyles.create(renamed);
	m_selection.clear();
	m_tmpStyles.remove(m_tmpStyles.find(oldName));
	for (int i = 0; i < m_tmpStyles.count(); ++i)
	{
		if (m_tmpStyles[i].parent() == oldName)
			m_tmpStyles[i].setParent(newName);
	}
	m_selection.append(&m_tmpStyles[m_tmpStyles.find(newName)]);
	for (auto it = m_deleted.begin(); it != m_deleted.end(); ++it)
	{
		if (it->second == oldName)
		{
			oldName = it->first;
			m_deleted.erase(it);
			break;
		}
	}
	m_deleted.append(RemoveItem(oldName, newName));
	markDirty();
}

QString SMObjectStyle::getUniqueName(const QString& name)
{
	return m_tmpStyles.getUniqueCopyName(name);
}

void SMObjectStyle::languageChange()
{
	m_widget->setTabText(m_widget->indexOf(m_page), tr("Properties"));
	m_page->languageChange();
}

void SMObjectStyle::unitChange()
{
	if (m_doc)
		m_page->setUnit(m_doc->unitIndex());
}

void SMObjectStyle::slotWidgetChanged()
{
	if (m_selection.count() != 1)
		return;
	m_page->updateStyle(*m_selection.first());
	m_tmpStyles.invalidate();
	markDirty();
}

void SMObjectStyle::updateStylesCache()
{
	if (!m_doc)
		return;
	m_selection.clear();
	m_tmpStyles.clear();
	m_deleted.clear();
	m_tmpStyles.redefine(m_doc->objectStyles(), true);
}

void SMObjectStyle::markDirty()
{
	if (m_selectionIsDirty)
		return;
	m_selectionIsDirty = true;
	emit selectionDirty();
}
