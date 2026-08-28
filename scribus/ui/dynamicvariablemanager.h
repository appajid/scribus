/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#ifndef DYNAMICVARIABLEMANAGER_H
#define DYNAMICVARIABLEMANAGER_H

#include <QDialog>

class QPushButton;
class QTableWidget;
class ScribusDoc;

class DynamicVariableManager : public QDialog
{
public:
	explicit DynamicVariableManager(ScribusDoc* doc, QWidget* parent = nullptr);

private:
	void addVariable();
	void editVariable();
	void deleteVariable();
	void refresh();
	void updateButtons();
	QString selectedId() const;

	ScribusDoc* m_doc {nullptr};
	QTableWidget* m_table {nullptr};
	QPushButton* m_editButton {nullptr};
	QPushButton* m_deleteButton {nullptr};
};

#endif
