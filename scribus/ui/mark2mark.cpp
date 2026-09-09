#include "marks.h"
#include "mark2mark.h"
#include <QStandardItemModel>

Mark2Mark::Mark2Mark(const QList<Mark*>& marks, Mark* omitMark, QWidget *parent) : MarkInsert(marks, parent)
{
	setupUi(this);
	formatCombo->addItem(QString(), CrossReferencePageNumber);
	formatCombo->addItem(QString(), CrossReferenceParagraphText);
	LabelList->addItem("", QVariant::fromValue((void*) nullptr));
	
	//for each marks type
	QString typeStr;
	MarkType typeMrk;

	int index = 0;
	typeMrk = MARKAnchorType;
	typeStr = tr("Cross-reference Targets");
	//adding name of marks type, and make it unselectable
	LabelList->addItem("+++ " + typeStr);
	qobject_cast<QStandardItemModel *>(LabelList->model())->item(++index)->setEnabled(false);
	for (int i = 0; i < marks.size(); ++i)
	{
		if (marks[i]->isType(typeMrk))
		{
			LabelList->addItem(marks[i]->label, QVariant::fromValue((void*) marks[i]));
			index++;
		}
	}
	typeMrk = MARK2MarkType;
	typeStr = tr("Mark to Mark");
	LabelList->addItem("+++ " + typeStr);
	qobject_cast<QStandardItemModel *>(LabelList->model())->item(++index)->setEnabled(false);
	for (int i = 0; i < marks.size(); ++i)
	{
		if (marks[i]->isType(typeMrk) && marks[i] != omitMark)
		{
			LabelList->addItem(marks[i]->label, QVariant::fromValue((void*) marks[i]));
			index++;
		}
	}
	typeMrk = MARK2ItemType;
	typeStr = tr("Mark to Item");
	LabelList->addItem("+++ " + typeStr);
	qobject_cast<QStandardItemModel *>(LabelList->model())->item(++index)->setEnabled(false);
	for (int i = 0; i < marks.size(); ++i)
	{
		if (marks[i]->isType(typeMrk))
		{
			LabelList->addItem(marks[i]->label, QVariant::fromValue((void*) marks[i]));
			index++;
		}
	}
	typeMrk = MARKNoteMasterType;
	typeStr = tr("Note mark");
	LabelList->addItem("+++ " + typeStr);
	qobject_cast<QStandardItemModel *>(LabelList->model())->item(++index)->setEnabled(false);
	for (int i = 0; i < marks.size(); ++i)
	{
		if (marks[i]->isType(typeMrk))
		{
			LabelList->addItem(marks[i]->label, QVariant::fromValue((void*) marks[i]));
			index++;
		}
	}
//	typeMrk = MARKIndexType;	typeStr = tr("Index entry");
//	LabelList->addItem("+++ " + typeStr);
//	qobject_cast<QStandardItemModel *>(LabelList->model())->item(++index)->setEnabled(false);
//	for (int i = 0; i < marks.size(); ++i)
//	{
//		if (marks[i]->isType(typeMrk))
//		{
//			LabelList->addItem(marks[i]->label, QVariant::fromValue((void*) marks[i]));
//			index++;
//		}
//	}
	languageChange();
}

void Mark2Mark::values(QString& label, Mark* &mrk)
{
	label = this->labelEdit->text();
	int labelID = LabelList->currentIndex();
	if (labelID == 0)
		mrk = nullptr;
	else
		mrk = (Mark*) LabelList->itemData(labelID).value<void*>();
}

void Mark2Mark::setValues(const QString label, const Mark* mrk)
{
	int index = (mrk == nullptr)? -1:LabelList->findText(mrk->label);
	LabelList->setCurrentIndex(index);
	labelEdit->setText(label);
}

void Mark2Mark::crossReferenceValues(QString& label, Mark*& mrk, CrossReferenceFormat& format, QString& prefix, QString& suffix) const
{
	label = labelEdit->text();
	mrk = reinterpret_cast<Mark*>(LabelList->currentData().value<void*>());
	format = static_cast<CrossReferenceFormat>(formatCombo->currentData().toInt());
	prefix = prefixEdit->text();
	suffix = suffixEdit->text();
}

void Mark2Mark::setCrossReferenceValues(const QString& label, const Mark* mrk, CrossReferenceFormat format,
	const QString& prefix, const QString& suffix)
{
	setValues(label, mrk);
	const int formatIndex = formatCombo->findData(static_cast<int>(format));
	formatCombo->setCurrentIndex(formatIndex >= 0 ? formatIndex : 0);
	prefixEdit->setText(prefix);
	suffixEdit->setText(suffix);
}

void Mark2Mark::languageChange()
{
	setWindowTitle(tr("Cross-reference"));
	formatCombo->setItemText(0, tr("Page number"));
	formatCombo->setItemText(1, tr("Target paragraph text"));
	formatCombo->setToolTip(tr("Choose the content displayed by this cross-reference."));
	prefixEdit->setPlaceholderText(tr("Optional text before the value"));
	suffixEdit->setPlaceholderText(tr("Optional text after the value"));
}

void Mark2Mark::changeEvent(QEvent *e)
{
	QDialog::changeEvent(e);
	switch (e->type())
	{
		case QEvent::LanguageChange:
			retranslateUi(this);
			languageChange();
			break;
		default:
			break;
	}
}
