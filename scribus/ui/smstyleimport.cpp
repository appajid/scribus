/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QCheckBox>

#include "smstyleimport.h"


SMStyleImport::SMStyleImport(QWidget* parent,
							 StyleSet<ParagraphStyle> *pstyleList,
							 StyleSet<CharStyle> *cstyleList,
							 QHash<QString, MultiLine> *lstyleList,
							 StyleSet<TableStyle> *tstyleList,
							 StyleSet<CellStyle> *cellstyleList,
							 StyleSet<ObjectStyle> *objectstyleList)
	: QDialog(parent, Qt::WindowFlags())
{
	setupUi(this);
	setModal(true);
	cstyleItem = new QTreeWidgetItem(styleWidget);
	cstyleItem->setText(0, tr("Character Styles"));
	for (int x = 0; x < cstyleList->count(); ++x)
	{
		CharStyle& vg ((*cstyleList)[x]);
		QCheckBox *box = new QCheckBox(vg.name());
		box->setChecked(true);
		QTreeWidgetItem *item = new QTreeWidgetItem(cstyleItem, cType);
		styleWidget->setItemWidget(item, 0, box);
	}
	styleWidget->expandItem(cstyleItem);

	pstyleItem = new QTreeWidgetItem(styleWidget);
	pstyleItem->setText(0, tr("Paragraph Styles"));
	for (int x = 0; x < pstyleList->count(); ++x)
	{
		ParagraphStyle& vg ((*pstyleList)[x]);
		QCheckBox *box = new QCheckBox(vg.name());
		box->setChecked(true);
		QTreeWidgetItem *item = new QTreeWidgetItem(pstyleItem, pType);
		styleWidget->setItemWidget(item, 0, box);
	}
	styleWidget->expandItem(pstyleItem);

	lstyleItem = new QTreeWidgetItem(styleWidget);
	lstyleItem->setText(0, tr("Line Styles"));
	QList<QString> lkeys = lstyleList->keys();
	for (int x = 0; x < lkeys.count(); ++x)
	{
		QCheckBox *box = new QCheckBox(lkeys[x]);
		box->setChecked(true);
		QTreeWidgetItem *item = new QTreeWidgetItem(lstyleItem, lType);
		styleWidget->setItemWidget(item, 0, box);
	}
	styleWidget->expandItem(lstyleItem);

	objectstyleItem = new QTreeWidgetItem(styleWidget);
	objectstyleItem->setText(0, tr("Object Styles"));
	for (int x = 0; x < objectstyleList->count(); ++x)
	{
		ObjectStyle& style((*objectstyleList)[x]);
		if (!style.hasName())
			continue;
		QCheckBox *box = new QCheckBox(style.name());
		box->setChecked(true);
		QTreeWidgetItem *item = new QTreeWidgetItem(objectstyleItem, objectType);
		styleWidget->setItemWidget(item, 0, box);
	}
	styleWidget->expandItem(objectstyleItem);

	tstyleItem = new QTreeWidgetItem(styleWidget);
	tstyleItem->setText(0, tr("Table Styles"));
	for (int x = 0; x < tstyleList->count(); ++x)
	{
		TableStyle& vg ((*tstyleList)[x]);
		if (!vg.hasName())
			continue;
		QCheckBox *box = new QCheckBox(vg.name());
		box->setChecked(true);
		QTreeWidgetItem *item = new QTreeWidgetItem(tstyleItem, tType);
		styleWidget->setItemWidget(item, 0, box);
	}
	styleWidget->expandItem(tstyleItem);

	cellstyleItem = new QTreeWidgetItem(styleWidget);
	cellstyleItem->setText(0, tr("Cell Styles"));
	for (int x = 0; x < cellstyleList->count(); ++x)
	{
		CellStyle& vg ((*cellstyleList)[x]);
		if (!vg.hasName())
			continue;
		QCheckBox *box = new QCheckBox(vg.name());
		box->setChecked(true);
		QTreeWidgetItem *item = new QTreeWidgetItem(cellstyleItem, cellType);
		styleWidget->setItemWidget(item, 0, box);
	}
	styleWidget->expandItem(cellstyleItem);

	connect(importAllCheckBox, SIGNAL(clicked(bool)), this, SLOT(checkAll(bool)));
}

bool SMStyleImport::clashRename()
{
	return renameButton->isChecked();
}

QStringList SMStyleImport::paragraphStyles()
{
	return commonStyles(pstyleItem, pType);
}

QStringList SMStyleImport::characterStyles()
{
	return commonStyles(cstyleItem, cType);
}

QStringList SMStyleImport::lineStyles()
{
	return commonStyles(lstyleItem, lType);
}

QStringList SMStyleImport::tableStyles()
{
	return commonStyles(tstyleItem, tType);
}

QStringList SMStyleImport::cellStyles()
{
	return commonStyles(cellstyleItem, cellType);
}

QStringList SMStyleImport::objectStyles()
{
	return commonStyles(objectstyleItem, objectType);
}

QStringList SMStyleImport::commonStyles(QTreeWidgetItem * rootItem, int type)
{
	QStringList ret;
	QTreeWidgetItemIterator it(styleWidget);

	while (*it)
	{
		if ((*it)->type() == type)
		{
			QCheckBox *w = qobject_cast<QCheckBox*>(styleWidget->itemWidget((*it), 0));
			if (w && w->isChecked())
				ret.append(w->text());
		}
		++it;
	}
	return ret;
}

void SMStyleImport::checkAll(bool allChecked)
{
	QTreeWidgetItemIterator it(styleWidget);

	while (*it)
	{
		QCheckBox *w = qobject_cast<QCheckBox*>(styleWidget->itemWidget((*it), 0));
		if (w)
			w->setCheckState(allChecked ? Qt::Checked : Qt::Unchecked);
		++it;
	}
}
