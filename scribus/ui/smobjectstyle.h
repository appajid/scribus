/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#ifndef SMOBJECTSTYLE_H
#define SMOBJECTSTYLE_H

#include "styleitem.h"
#include "styles/objectstyle.h"
#include "styles/styleset.h"

class QTabWidget;
class ScribusDoc;
class SMObjectStyleWidget;

class SMObjectStyle : public StyleItem
{
	Q_OBJECT

public:
	SMObjectStyle();
	~SMObjectStyle() override;

	QTabWidget* widget() override;
	QString typeNamePlural() override;
	QString typeNameSingular() override;
	void setCurrentDoc(ScribusDoc* doc) override;
	QList<StyleName> styles(bool reloadFromDoc = true) override;
	void reload() override;
	void selected(const QStringList& styleNames) override;
	QString fromSelection() const override;
	void toSelection(const QString& styleName) const override;
	QString newStyle() override;
	QString newStyle(const QString& fromStyle) override;
	void apply() override;
	void editMode(bool isOn) override;
	bool isDefaultStyle(const QString& styleName) const override;
	void setDefaultStyle(bool isDefaultStyle) override;
	QString shortcut(const QString& styleName) const override;
	void setShortcut(const QString& shortcut) override;
	void deleteStyles(const QList<RemoveItem>& removeList) override;
	void nameChanged(const QString& newName) override;
	QString getUniqueName(const QString& name) override;
	void languageChange() override;
	void unitChange() override;

	StyleSet<ObjectStyle>* tmpStyles() { return &m_tmpStyles; }

signals:
	void selectionDirty();

private slots:
	void slotWidgetChanged();

private:
	QString internalName(const QString& displayName) const;
	void updateStylesCache();
	void markDirty();

	QTabWidget* m_widget { nullptr };
	SMObjectStyleWidget* m_page { nullptr };
	ScribusDoc* m_doc { nullptr };
	StyleSet<ObjectStyle> m_tmpStyles;
	QList<ObjectStyle*> m_selection;
	QList<RemoveItem> m_deleted;
	bool m_selectionIsDirty { false };
};

#endif
