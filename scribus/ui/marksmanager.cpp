#include "marksmanager.h"
#include "notesstyles.h"
#include "prefsmanager.h"
#include "prefsfile.h"
#include "scribus.h"
#include "scribusdoc.h"
#include "undomanager.h"
#include "util.h"
#include "iconmanager.h"
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QStandardItemModel>

MarksManager::MarksManager(QWidget *parent, const char *name)
	: ScrPaletteBase(parent, name)
{
	setupUi(this);
	listView->setSelectionMode(QAbstractItemView::SingleSelection);
	listView->setSortingEnabled(true);
	listView->setHeaderHidden(false);
	listView->setColumnCount(3);
	listView->setAlternatingRowColors(true);
	listView->setUniformRowHeights(true);
	listView->setContextMenuPolicy(Qt::CustomContextMenu);
	listView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	listView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	listView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	QString pname(name);
	if (pname.isEmpty())
		pname = "marksManager";
	m_prefs = PrefsManager::instance().prefsFile->getContext(pname);
	connect(listView, &QTreeWidget::customContextMenuRequested, this, &MarksManager::showContextMenu);
	setDoc(nullptr);
	languageChange();
	GoToButton->setEnabled(false);
	EditButton->setEnabled(false);
	DeleteButton->setEnabled(false);
	UpdateButton->setEnabled(false);
}

MarksManager::~MarksManager()
{
	storeVisibility(this->isVisible());
	storePosition();
	storeSize();
}

void MarksManager::addListItem(MarkType typeMrk, const QString& typeStr, const QList<Mark*> &marks, int &index)
{
	bool noSuchMarks = true;
	QTreeWidgetItem *listItem = new QTreeWidgetItem(listView);
	listItem->setText(0,typeStr);
	listItem->setFlags(listItem->flags() & (~Qt::ItemIsSelectable));
	listItem->setBackground(0, this->palette().color(QPalette::AlternateBase));
	listItem->setFirstColumnSpanned(true);
	for (int i = 0; i < marks.size(); ++i)
	{
		if (marks[i]->isType(typeMrk))
		{
			// Dynamic variables are managed by the Variables dialog. Keeping them
			// out of the legacy mark editor prevents their stable references from
			// being changed into ordinary variable-text marks.
			if (typeMrk == MARKVariableTextType && !marks[i]->getVariableId().isEmpty())
				continue;
			QTreeWidgetItem *listItem2 = new QTreeWidgetItem(listItem);
			if (marks[i]->isType(MARKIndexType))
				listItem2->setText(0, marks[i]->getString());
			else
				listItem2->setText(0, marks[i]->label);
			listItem2->setData(0, Qt::UserRole, QVariant::fromValue<void*>(marks[i]));

			PageItem* markItem = m_Doc->findFirstMarkItem(marks[i]);
			if (markItem && markItem->OwnPage >= 0 && markItem->OwnPage < m_Doc->DocPages.count())
				listItem2->setText(1, m_Doc->getSectionPageNumberForPageIndex(static_cast<uint>(markItem->OwnPage)));
			else
				listItem2->setText(1, QStringLiteral("\u2014"));

			if (typeMrk == MARKAnchorType)
			{
				if (markItem)
					listItem2->setText(2, tr("Target"));
				else
				{
					listItem2->setText(2, tr("Not placed"));
					listItem2->setIcon(2, IconManager::instance().loadIcon("alert-warning"));
					listItem2->setToolTip(2, tr("This target is not present in document text."));
				}
			}
			else if (typeMrk == MARK2MarkType)
			{
				const QString targetName = marks[i]->getDestMarkName();
				Mark* target = m_Doc->getMark(targetName, marks[i]->getDestMarkType());
				PageItem* targetItem = target ? m_Doc->findFirstMarkItem(target) : nullptr;
				if (target && targetItem)
				{
					const QString formatName = marks[i]->getCrossReferenceFormat() == CrossReferenceParagraphText
						? tr("Paragraph text") : tr("Page number");
					listItem2->setText(2, tr("To %1 · %2").arg(targetName, formatName));
					if (!marks[i]->getCrossReferencePrefix().isEmpty() || !marks[i]->getCrossReferenceSuffix().isEmpty())
						listItem2->setToolTip(2, tr("Prefix: “%1”\nSuffix: “%2”")
							.arg(marks[i]->getCrossReferencePrefix(), marks[i]->getCrossReferenceSuffix()));
				}
				else
				{
					listItem2->setText(2, tr("Missing: %1").arg(targetName));
					listItem2->setIcon(2, IconManager::instance().loadIcon("alert-warning"));
					listItem2->setToolTip(2, tr("Edit this page reference and choose an existing target."));
				}
			}
			index++;
			noSuchMarks = false;
		}
	}
	if (noSuchMarks)
	{
		listView->removeItemWidget(listItem,0);
		delete listItem;
	}
	else
		listItem->sortChildren(0, Qt::AscendingOrder);
}

void MarksManager::storeColaption()
{
	m_expandedItems.clear();
	for (int i=0; i < listView->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = listView->topLevelItem(i);
		if (item->isExpanded())
			m_expandedItems.append(item->text(0));
	}
}

void MarksManager::restoreColaption()
{
	listView->collapseAll();
	if (m_expandedItems.isEmpty())
		return;

	for (int i=0; i < listView->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *item = listView->topLevelItem(i);
		if (m_expandedItems.contains(item->text(0)))
			item->setExpanded(true);
	}
}

void MarksManager::updateListView()
{
	storeColaption();
	listView->clear();
	if (m_Doc == nullptr)
		return;
	if (m_Doc->marksList().isEmpty())
		UpdateButton->setEnabled(false);
	else
	{
		UpdateButton->setEnabled(true);
		int index = 0;
		addListItem(MARKAnchorType, tr("Cross-reference Targets"), m_Doc->marksList(), index);
		addListItem(MARKVariableTextType, tr("Variable Text"), m_Doc->marksList(), index);
		addListItem(MARK2ItemType, tr("Marks to Items"), m_Doc->marksList(), index);
		addListItem(MARK2MarkType, tr("Cross-references"), m_Doc->marksList(), index);
		addListItem(MARKNoteMasterType, tr("Notes marks"), m_Doc->marksList(), index);
		addListItem(MARKIndexType, tr("Index Entries"), m_Doc->marksList(), index);
		listView->sortByColumn(0, Qt::AscendingOrder);
	}
	restoreColaption();
	m_Doc->flag_updateMarksLabels = false;
	m_Doc->flag_updateEndNotes = false;
}

void MarksManager::setDoc(ScribusDoc *doc)
{
	if (m_Doc != nullptr)
		disconnect(m_Doc->scMW(), SIGNAL(UpdateRequest(int)), this , SLOT(handleUpdateRequest(int)));

	UpdateButton->setEnabled(false);
	GoToButton->setEnabled(false);
	EditButton->setEnabled(false);
	DeleteButton->setEnabled(false);
	listView->setEnabled(false);

	m_Doc = doc;
	if (!m_Doc)
	{
		listView->clear();
		return;
	}

	UpdateButton->setEnabled(true);
	listView->setEnabled(true);
	updateListView();
	connect(m_Doc->scMW(), SIGNAL(UpdateRequest(int)), this , SLOT(handleUpdateRequest(int)));
}

void MarksManager::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
		return;
	}
	if (e->type() == QEvent::PaletteChange)
		paletteChange();
	ScrPaletteBase::changeEvent(e);
}

void MarksManager::languageChange()
{
	retranslateUi(this);
	setWindowTitle(tr("References and Marks"));
	listView->setHeaderLabels({tr("Name"), tr("Page"), tr("Details")});
	UpdateButton->setText(tr("Update References and Marks"));

	listView->setToolTip(tr("Double-click an entry to locate its target in the document"));
	UpdateButton->setToolTip(tr("Update all page references, variables, and marks"));
	GoToButton->setToolTip(tr("Locate the selected target or mark in the document"));
	EditButton->setToolTip(tr("Edit the selected reference or mark"));
	if (m_Doc != nullptr)
		updateListView();
	on_listView_itemSelectionChanged();
}

void MarksManager::paletteChange()
{
	QColor listItemColor = this->palette().color(QPalette::AlternateBase);
	for (int i = 0; i < listView->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *listItem = listView->topLevelItem(i);
		listItem->setBackground(0, listItemColor);
	}
}

void MarksManager::handleUpdateRequest(int updateFlags)
{
	if (updateFlags & reqMarksUpdate)
		updateListView();

	m_Doc->flag_updateMarksLabels = false;
}

Mark* MarksManager::getMarkFromListView()
{
	QTreeWidgetItem* selectedItem = listView->currentItem();
	if (selectedItem == nullptr)
		return nullptr;
	Mark* mrk = reinterpret_cast<Mark*>(selectedItem->data(0, Qt::UserRole).value<void*>());
	return mrk;
}

Mark* MarksManager::navigationMark(Mark* mark) const
{
	if (!m_Doc || !mark)
		return nullptr;
	return mark->isType(MARK2MarkType) ? m_Doc->crossReferenceDestination(mark) : mark;
}

bool MarksManager::canNavigateToMark(Mark* mark) const
{
	Mark* destination = navigationMark(mark);
	return destination && m_Doc->findFirstMarkItem(destination);
}

QString MarksManager::navigationLabel(const Mark* mark) const
{
	return mark && (mark->isType(MARK2MarkType) || mark->isType(MARKAnchorType))
		? tr("Go to Target") : tr("Go to Mark");
}

bool MarksManager::isBrokenCrossReference(const Mark* mark) const
{
	if (!m_Doc || !mark || !mark->isType(MARK2MarkType))
		return false;
	Mark* target = m_Doc->getMark(mark->getDestMarkName(), mark->getDestMarkType());
	return !target || !m_Doc->findFirstMarkItem(target);
}

void MarksManager::on_GoToButton_clicked()
{
	Mark* destination = navigationMark(getMarkFromListView());
	if (destination)
		m_Doc->navigateToMark(destination);
}

void MarksManager::on_UpdateButton_clicked()
{
	m_Doc->flag_updateMarksLabels = true;
	m_Doc->flag_updateEndNotes = true;
	m_Doc->setNotesChanged(true);
	if (m_Doc->updateMarks(true))
	{
		m_Doc->changed();
		m_Doc->regionsChanged()->update(QRectF());
		updateListView();
	}

	//update labels for "lost" marks (marks not in any text)
	QList<Mark*> notUsed;
	for (int i=0; i < m_Doc->marksList().count(); ++i)
	{
		Mark* mrk = m_Doc->marksList().at(i);
		if (mrk->isUnique() && !mrk->label.startsWith("INVISIBLE*") && !m_Doc->isMarkUsed(mrk, true))
			notUsed.append(mrk);
	}

	if (notUsed.isEmpty())
		return;

	for (int i=0; i < notUsed.count(); ++i)
	{
		Mark* mrk = notUsed.at(i);
		QString l = "INVISIBLE*" + mrk->label;
		getUniqueName(l, m_Doc->marksLabelsList(mrk->getType()), "_");
		mrk->label = l;
	}
	updateListView();
}

void MarksManager::on_EditButton_clicked()
{
	Mark* mrk = getMarkFromListView();
	if (mrk == nullptr)
		return;

	if (m_Doc->scMW()->editMarkDlg(mrk))
	{
		if (mrk->isType(MARKVariableTextType))
			m_Doc->flag_updateMarksLabels = true;
//		else
//			currItem->invalid = true;
		m_Doc->changed();
		m_Doc->regionsChanged()->update(QRectF());
		updateListView();
	}
}

void MarksManager::on_DeleteButton_clicked()
{
	Mark* mrk = getMarkFromListView();
	if (mrk == nullptr)
		return;

	if (mrk->isType(MARKAnchorType))
	{
		QString message = tr("Delete the cross-reference target “%1”?").arg(mrk->label);
		const int usageCount = m_Doc->crossReferenceTargetUsage(mrk->label);
		if (usageCount > 0)
		{
			message += QStringLiteral("\n\n") + tr("%n page reference(s) use this target. They will remain in the document, display no value, and be flagged by Preflight until repaired or deleted.", nullptr, usageCount);
		}
		if (QMessageBox::warning(this, tr("Delete Cross-reference Target"), message,
			QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
			return;
		if (!m_Doc->deleteCrossReferenceTarget(mrk->label))
			return;
	}
	else if (mrk->isType(MARKNoteMasterType))
	{
		m_Doc->setUndoDelNote(mrk->getNotePtr());
		m_Doc->eraseMark(mrk, true, mrk->getItemPtr(), true);
	}
	else
	{
		m_Doc->setUndoDelMark(mrk);
		m_Doc->eraseMark(mrk, true, mrk->getItemPtr(), true);
	}
	m_Doc->changed();
	m_Doc->regionsChanged()->update(QRectF());
	updateListView();
}

void MarksManager::on_listView_doubleClicked(const QModelIndex &index)
{
	Q_UNUSED(index);
	on_GoToButton_clicked();
}

void MarksManager::on_listView_itemSelectionChanged()
{
	Mark* mark = getMarkFromListView();
	bool isMark = (mark != nullptr);
	GoToButton->setText(navigationLabel(mark));
	GoToButton->setEnabled(canNavigateToMark(mark));
	EditButton->setEnabled(isMark);
	DeleteButton->setEnabled(isMark);
	const bool needsRepair = isBrokenCrossReference(mark);
	EditButton->setText(needsRepair ? tr("Repair") : tr("Edit"));
	EditButton->setToolTip(needsRepair
		? tr("Choose an existing target for this broken page reference")
		: tr("Edit the selected reference or mark"));
}

void MarksManager::showContextMenu(const QPoint& point)
{
	QTreeWidgetItem* item = listView->itemAt(point);
	if (!item || !item->data(0, Qt::UserRole).isValid())
		return;
	listView->setCurrentItem(item);
	Mark* mark = getMarkFromListView();
	if (!mark)
		return;

	QMenu menu(this);
	QAction* goToAction = menu.addAction(navigationLabel(mark));
	goToAction->setEnabled(canNavigateToMark(mark));
	menu.addSeparator();
	QAction* editAction = menu.addAction(isBrokenCrossReference(mark) ? tr("Repair") : tr("Edit"));
	QAction* deleteAction = menu.addAction(tr("Delete"));
	QAction* selectedAction = menu.exec(listView->viewport()->mapToGlobal(point));
	if (selectedAction == goToAction)
		on_GoToButton_clicked();
	else if (selectedAction == editAction)
		on_EditButton_clicked();
	else if (selectedAction == deleteAction)
		on_DeleteButton_clicked();
}
