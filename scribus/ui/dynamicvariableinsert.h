/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#ifndef DYNAMICVARIABLEINSERT_H
#define DYNAMICVARIABLEINSERT_H

#include <QDialog>

class QComboBox;
class QLabel;
class PageItem;
class ScribusDoc;

class DynamicVariableInsert : public QDialog
{
public:
	DynamicVariableInsert(ScribusDoc* doc, const PageItem* frame, QWidget* parent = nullptr);
	QString variableId() const;

private:
	void updatePreview();

	ScribusDoc* m_doc {nullptr};
	const PageItem* m_frame {nullptr};
	QComboBox* m_variables {nullptr};
	QLabel* m_preview {nullptr};
};

#endif
