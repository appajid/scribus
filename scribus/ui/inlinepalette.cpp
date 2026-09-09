/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          inlinepalette.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2012
    copyright            : (C) 2012 by Franz Schmid
    email                : Franz.Schmid@altmuehlnet.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "inlinepalette.h"
#include <QPainter>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDrag>
#include <QFormLayout>
#include <QGroupBox>
#include <QMimeData>
#include <QVBoxLayout>

#include "appmodes.h"
#include "iconmanager.h"
#include "pageitem.h"
#include "pageitem_table.h"
#include "pageitem_textframe.h"
#include "scribus.h"
#include "scribusdoc.h"
#include "scrspinbox.h"
#include "selection.h"
#include "units.h"

namespace
{
class AnchorOptionsDialog : public QDialog
{
public:
	explicit AnchorOptionsDialog(const AnchorPosition& position, int unitIndex, QWidget* parent = nullptr)
		: QDialog(parent)
		, m_unitIndex(unitIndex)
	{
		setWindowTitle(tr("Anchored Object Options"));
		setMinimumWidth(420);
		auto* mainLayout = new QVBoxLayout(this);

		auto* modeLayout = new QFormLayout;
		m_mode = new QComboBox(this);
		m_mode->addItem(tr("Inline"), static_cast<int>(AnchorPosition::Mode::Inline));
		m_mode->addItem(tr("Above Line"), static_cast<int>(AnchorPosition::Mode::AboveLine));
		m_mode->addItem(tr("Custom"), static_cast<int>(AnchorPosition::Mode::Custom));
		modeLayout->addRow(tr("Position:"), m_mode);
		mainLayout->addLayout(modeLayout);

		m_positionGroup = new QGroupBox(tr("Position"), this);
		auto* positionLayout = new QFormLayout(m_positionGroup);
		m_horizontalReference = new QComboBox(m_positionGroup);
		m_horizontalReference->addItem(tr("Anchor Character"), static_cast<int>(AnchorPosition::HorizontalReference::AnchorCharacter));
		m_horizontalReference->addItem(tr("Text Column"), static_cast<int>(AnchorPosition::HorizontalReference::TextColumn));
		m_horizontalReference->addItem(tr("Text Frame"), static_cast<int>(AnchorPosition::HorizontalReference::TextFrame));
		m_horizontalReference->addItem(tr("Page"), static_cast<int>(AnchorPosition::HorizontalReference::Page));
		m_horizontalReference->addItem(tr("Spread"), static_cast<int>(AnchorPosition::HorizontalReference::Spread));
		m_horizontalAlignment = new QComboBox(m_positionGroup);
		m_horizontalAlignment->addItem(tr("Left"), static_cast<int>(AnchorPosition::HorizontalAlignment::Left));
		m_horizontalAlignment->addItem(tr("Center"), static_cast<int>(AnchorPosition::HorizontalAlignment::Center));
		m_horizontalAlignment->addItem(tr("Right"), static_cast<int>(AnchorPosition::HorizontalAlignment::Right));
		m_horizontalAlignment->addItem(tr("Spine"), static_cast<int>(AnchorPosition::HorizontalAlignment::Spine));
		m_horizontalAlignment->addItem(tr("Away from Spine"), static_cast<int>(AnchorPosition::HorizontalAlignment::AwayFromSpine));
		m_horizontalAlignment->addItem(tr("Custom"), static_cast<int>(AnchorPosition::HorizontalAlignment::Custom));
		m_verticalReference = new QComboBox(m_positionGroup);
		m_verticalReference->addItem(tr("Anchor Line"), static_cast<int>(AnchorPosition::VerticalReference::AnchorLine));
		m_verticalReference->addItem(tr("Paragraph"), static_cast<int>(AnchorPosition::VerticalReference::Paragraph));
		m_verticalReference->addItem(tr("Text Frame"), static_cast<int>(AnchorPosition::VerticalReference::TextFrame));
		m_verticalReference->addItem(tr("Page"), static_cast<int>(AnchorPosition::VerticalReference::Page));
		m_verticalAlignment = new QComboBox(m_positionGroup);
		m_verticalAlignment->addItem(tr("Top"), static_cast<int>(AnchorPosition::VerticalAlignment::Top));
		m_verticalAlignment->addItem(tr("Center"), static_cast<int>(AnchorPosition::VerticalAlignment::Center));
		m_verticalAlignment->addItem(tr("Bottom"), static_cast<int>(AnchorPosition::VerticalAlignment::Bottom));
		m_verticalAlignment->addItem(tr("Baseline"), static_cast<int>(AnchorPosition::VerticalAlignment::Baseline));
		m_verticalAlignment->addItem(tr("Custom"), static_cast<int>(AnchorPosition::VerticalAlignment::Custom));
		m_xOffset = createDistanceSpinBox();
		m_yOffset = createDistanceSpinBox();
		positionLayout->addRow(tr("Horizontal reference:"), m_horizontalReference);
		positionLayout->addRow(tr("Horizontal alignment:"), m_horizontalAlignment);
		positionLayout->addRow(tr("Vertical reference:"), m_verticalReference);
		positionLayout->addRow(tr("Vertical alignment:"), m_verticalAlignment);
		positionLayout->addRow(tr("X offset:"), m_xOffset);
		positionLayout->addRow(tr("Y offset:"), m_yOffset);
		mainLayout->addWidget(m_positionGroup);

		m_wrapGroup = new QGroupBox(tr("Text Wrap"), this);
		auto* wrapLayout = new QFormLayout(m_wrapGroup);
		m_wrapMode = new QComboBox(m_wrapGroup);
		m_wrapMode->addItem(tr("None"), static_cast<int>(AnchorPosition::WrapMode::None));
		m_wrapMode->addItem(tr("Bounding Box"), static_cast<int>(AnchorPosition::WrapMode::BoundingBox));
		m_wrapMode->addItem(tr("Frame Shape"), static_cast<int>(AnchorPosition::WrapMode::FrameShape));
		m_wrapMode->addItem(tr("Contour"), static_cast<int>(AnchorPosition::WrapMode::Contour));
		m_wrapMode->addItem(tr("Image Clip Path"), static_cast<int>(AnchorPosition::WrapMode::ImageClipPath));
		m_wrapLeft = createDistanceSpinBox(false);
		m_wrapTop = createDistanceSpinBox(false);
		m_wrapRight = createDistanceSpinBox(false);
		m_wrapBottom = createDistanceSpinBox(false);
		wrapLayout->addRow(tr("Wrap shape:"), m_wrapMode);
		wrapLayout->addRow(tr("Left offset:"), m_wrapLeft);
		wrapLayout->addRow(tr("Top offset:"), m_wrapTop);
		wrapLayout->addRow(tr("Right offset:"), m_wrapRight);
		wrapLayout->addRow(tr("Bottom offset:"), m_wrapBottom);
		mainLayout->addWidget(m_wrapGroup);

		m_keepWithinBounds = new QCheckBox(tr("Keep within reference bounds"), this);
		m_lockPosition = new QCheckBox(tr("Prevent manual positioning"), this);
		mainLayout->addWidget(m_keepWithinBounds);
		mainLayout->addWidget(m_lockPosition);
		auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
		connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
		mainLayout->addWidget(buttons);

		setComboValue(m_mode, static_cast<int>(position.mode));
		setComboValue(m_horizontalReference, static_cast<int>(position.horizontalReference));
		setComboValue(m_verticalReference, static_cast<int>(position.verticalReference));
		setComboValue(m_horizontalAlignment, static_cast<int>(position.horizontalAlignment));
		setComboValue(m_verticalAlignment, static_cast<int>(position.verticalAlignment));
		setComboValue(m_wrapMode, static_cast<int>(position.wrapMode));
		m_xOffset->setValue(position.xOffset, SC_PT);
		m_yOffset->setValue(position.yOffset, SC_PT);
		m_wrapLeft->setValue(position.wrapOffsets.left(), SC_PT);
		m_wrapTop->setValue(position.wrapOffsets.top(), SC_PT);
		m_wrapRight->setValue(position.wrapOffsets.right(), SC_PT);
		m_wrapBottom->setValue(position.wrapOffsets.bottom(), SC_PT);
		m_keepWithinBounds->setChecked(position.keepWithinBounds);
		m_lockPosition->setChecked(position.preventManualPositioning);
		connect(m_mode, &QComboBox::currentIndexChanged, this, [this] { updateEnabledState(); });
		updateEnabledState();
	}

	AnchorPosition position() const
	{
		AnchorPosition result;
		result.mode = static_cast<AnchorPosition::Mode>(m_mode->currentData().toInt());
		result.horizontalReference = static_cast<AnchorPosition::HorizontalReference>(m_horizontalReference->currentData().toInt());
		result.verticalReference = static_cast<AnchorPosition::VerticalReference>(m_verticalReference->currentData().toInt());
		result.horizontalAlignment = static_cast<AnchorPosition::HorizontalAlignment>(m_horizontalAlignment->currentData().toInt());
		result.verticalAlignment = static_cast<AnchorPosition::VerticalAlignment>(m_verticalAlignment->currentData().toInt());
		result.wrapMode = static_cast<AnchorPosition::WrapMode>(m_wrapMode->currentData().toInt());
		result.xOffset = m_xOffset->getValue(SC_PT);
		result.yOffset = m_yOffset->getValue(SC_PT);
		result.wrapOffsets = QMarginsF(m_wrapLeft->getValue(SC_PT), m_wrapTop->getValue(SC_PT),
			m_wrapRight->getValue(SC_PT), m_wrapBottom->getValue(SC_PT));
		result.keepWithinBounds = m_keepWithinBounds->isChecked();
		result.preventManualPositioning = m_lockPosition->isChecked();
		return result;
	}

private:
	ScrSpinBox* createDistanceSpinBox(bool allowNegative = true)
	{
		auto* spin = new ScrSpinBox(allowNegative ? -10000.0 : 0.0, 10000.0, this, m_unitIndex);
		return spin;
	}

	static void setComboValue(QComboBox* combo, int value)
	{
		const int index = combo->findData(value);
		if (index >= 0)
			combo->setCurrentIndex(index);
	}

	void updateEnabledState()
	{
		const auto mode = static_cast<AnchorPosition::Mode>(m_mode->currentData().toInt());
		m_positionGroup->setEnabled(mode != AnchorPosition::Mode::Inline);
		m_wrapGroup->setEnabled(mode == AnchorPosition::Mode::Custom);
		m_keepWithinBounds->setEnabled(mode == AnchorPosition::Mode::Custom);
		m_lockPosition->setEnabled(mode != AnchorPosition::Mode::Inline);
	}

	QComboBox* m_mode { nullptr };
	QGroupBox* m_positionGroup { nullptr };
	QComboBox* m_horizontalReference { nullptr };
	QComboBox* m_horizontalAlignment { nullptr };
	QComboBox* m_verticalReference { nullptr };
	QComboBox* m_verticalAlignment { nullptr };
	ScrSpinBox* m_xOffset { nullptr };
	ScrSpinBox* m_yOffset { nullptr };
	QGroupBox* m_wrapGroup { nullptr };
	QComboBox* m_wrapMode { nullptr };
	ScrSpinBox* m_wrapLeft { nullptr };
	ScrSpinBox* m_wrapTop { nullptr };
	ScrSpinBox* m_wrapRight { nullptr };
	ScrSpinBox* m_wrapBottom { nullptr };
	QCheckBox* m_keepWithinBounds { nullptr };
	QCheckBox* m_lockPosition { nullptr };
	int m_unitIndex { SC_PT };
};
}

InlineView::InlineView(QWidget* parent) : QListWidget(parent)
{
	setDragEnabled(true);
	setViewMode(QListView::IconMode);
	setFlow(QListView::LeftToRight);
	setSortingEnabled(true);
	setWrapping(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setDragDropMode(QAbstractItemView::DragDrop);
	setResizeMode(QListView::Adjust);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setContextMenuPolicy(Qt::CustomContextMenu);
	delegate = new ScListWidgetDelegate(this, this);
	delegate->setIconOnly(true);
	setItemDelegate(delegate);
	setIconSize(QSize(50, 50));
}

void InlineView::dragEnterEvent(QDragEnterEvent *e)
{
	if (e->source() == this)
		e->ignore();
	else
		e->acceptProposedAction();
}

void InlineView::dragMoveEvent(QDragMoveEvent *e)
{
	if (e->source() == this)
		e->ignore();
	else
		e->acceptProposedAction();
}

void InlineView::dropEvent(QDropEvent *e)
{
	if (e->mimeData()->hasText())
	{
		e->acceptProposedAction();
		if (e->source() == this)
			return;
		QString text = e->mimeData()->text();
		if ((text.startsWith("<SCRIBUSELEM")) || (text.startsWith("<SCRIBUSELEMUTF8")) || (text.startsWith("<ScribusElementUTF8")))
		{
			emit objectDropped(text);
		}
	}
	else
		e->ignore();
}

 void InlineView::startDrag(Qt::DropActions supportedActions)
 {
	QMimeData *mimeData = new QMimeData;
	int id = currentItem()->data(Qt::UserRole).toInt();
	QByteArray data;
	data.setNum(id);
	mimeData->setData("text/inline", data);
	QDrag *drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->setPixmap(currentItem()->icon().pixmap(48, 48));
	drag->exec(Qt::CopyAction);
	clearSelection();
}

InlinePalette::InlinePalette( QWidget* parent) : DockPanelBase("Inline", "panel-inline-items", parent)
{
	setContentsMargins(3, 3, 3, 3);
	setMinimumSize( QSize( 220, 240 ) );
	setObjectName(QString::fromLocal8Bit("Inline"));
	setSizePolicy( QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	InlineViewWidget = new InlineView(this);
	InlineViewWidget->clear();
	setWidget( InlineViewWidget );

	unsetDoc();
	m_scMW  = nullptr;
	currentEditedItem = -1;
	languageChange();
	connect(InlineViewWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(handleDoubleClick(QListWidgetItem*)));
	connect(InlineViewWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(handleContextMenue(QPoint)));
	connect(InlineViewWidget, SIGNAL(objectDropped(QString)), this, SIGNAL(objectDropped(QString)));
}

void InlinePalette::handleContextMenue(QPoint p)
{
	if (currentEditedItem > 0)
		return;
	QListWidgetItem *item = InlineViewWidget->itemAt(p);
	if (item)
	{
		actItem = item->data(Qt::UserRole).toInt();
		bool txFrame = false;
		if (m_doc->m_Selection->isNotEmpty())
		{
			PageItem* selItem = m_doc->m_Selection->itemAt(0);
			if ((selItem->isTextFrame() || selItem->isTable()))
				txFrame = true;
		}
		QMenu *pmenu = new QMenu();
		if (txFrame)
		{
			QAction* pasteAct = pmenu->addAction( tr("Paste to Item"));
			connect(pasteAct, SIGNAL(triggered()), this, SLOT(handlePasteToItem()));
		}
		if ((m_doc->appMode != modeEdit) && (m_doc->appMode != modeEditTable))
		{
			QAction* editAct = pmenu->addAction( tr("Edit Item"));
			connect(editAct, SIGNAL(triggered()), this, SLOT(handleEditItem()));
		}
		QAction* anchorAct = pmenu->addAction(tr("Anchored Object Options..."));
		connect(anchorAct, &QAction::triggered, this, &InlinePalette::handleAnchorOptions);
		QAction* delAct = pmenu->addAction( tr("Remove Item"));
		connect(delAct, SIGNAL(triggered()), this, SLOT(handleDeleteItem()));
		pmenu->exec(QCursor::pos());
		delete pmenu;
		actItem = -1;
	}
}

void InlinePalette::handlePasteToItem()
{
	PageItem* selItem = m_doc->m_Selection->itemAt(0);
	PageItem_TextFrame *currItem;
	if (selItem->isTable())
		currItem = selItem->asTable()->activeCell().textFrame();
	else
		currItem = selItem->asTextFrame();
	if (currItem->HasSel)
		currItem->deleteSelectedTextFromFrame();
	currItem->itemText.insertObject(actItem);
	if (selItem->isTable())
		selItem->asTable()->update();
	else
		currItem->update();
}

void InlinePalette::handleEditItem()
{
	emit startEdit(actItem);
}

void InlinePalette::handleAnchorOptions()
{
	PageItem* item = m_doc ? m_doc->FrameItems.value(actItem, nullptr) : nullptr;
	if (!item)
		return;

	AnchorOptionsDialog dialog(item->anchorPosition(), m_doc->unitIndex(), this);
	if (dialog.exec() != QDialog::Accepted)
		return;
	const AnchorPosition newPosition = dialog.position();
	if (newPosition == item->anchorPosition())
		return;

	item->setAnchorPosition(newPosition);
	updateItemList();
}

void InlinePalette::handleDoubleClick(QListWidgetItem *item)
{
	if (item)
		emit startEdit(item->data(Qt::UserRole).toInt());
}

void InlinePalette::handleDeleteItem()
{
	m_doc->removeInlineFrame(actItem);
	QListWidgetItem* item = InlineViewWidget->takeItem(InlineViewWidget->currentRow());
	delete item;
	InlineViewWidget->update();
}

void InlinePalette::editingStart(int itemID)
{
	currentEditedItem = itemID;
	for (int a = 0; a < InlineViewWidget->count(); a++)
	{
		QListWidgetItem* item = InlineViewWidget->item(a);
		if (item)
			item->setFlags(Qt::NoItemFlags);
	}
}

void InlinePalette::editingFinished()
{
	updateItemList();
	currentEditedItem = -1;
}

void InlinePalette::setMainWindow(ScribusMainWindow *mw)
{
	m_scMW = mw;
	if (m_scMW == nullptr)
	{
		InlineViewWidget->clear();
		disconnect(m_scMW, SIGNAL(UpdateRequest(int)), this, SLOT(handleUpdateRequest(int)));
		return;
	}
	connect(m_scMW, SIGNAL(UpdateRequest(int)), this, SLOT(handleUpdateRequest(int)), Qt::UniqueConnection);
}

void InlinePalette::setDoc(ScribusDoc *newDoc)
{
	if (m_scMW == nullptr)
		m_doc = nullptr;
	else
		m_doc = newDoc;
	if (m_doc == nullptr)
	{
		InlineViewWidget->clear();
		setEnabled(true);
	}
	else
	{
		setEnabled(!m_doc->drawAsPreview);
		updateItemList();
	}
}

void InlinePalette::unsetDoc()
{
	m_doc = nullptr;
	InlineViewWidget->clear();
	setEnabled(true);
}

void InlinePalette::handleUpdateRequest(int updateFlags)
{
	if (updateFlags & reqInlinePalUpdate)
		updateItemList();
}

void InlinePalette::updateItemList()
{
	InlineViewWidget->clear();
	InlineViewWidget->setWordWrap(true);
	if (!m_doc)
		return;
	for (auto it = m_doc->FrameItems.cbegin(); it != m_doc->FrameItems.cend(); ++it)
	{
		PageItem *currItem = it.value();
		QPixmap pm = QPixmap::fromImage(currItem->DrawObj_toImage(48));
		QPixmap pm2(50, 50);
		pm2.fill(palette().color(QPalette::Base));
		QPainter p;
		p.begin(&pm2);
		QBrush b(QColor(205,205,205), IconManager::instance().loadPixmap("testfill"));
		p.setBrush(b);
		p.drawRect(0, 0, 50, 50);
		p.drawPixmap(25 - pm.width() / 2, 25 - pm.height() / 2, pm);
		p.end();
		QString displayName = currItem->itemName();
		if (currItem->anchorPosition().mode == AnchorPosition::Mode::AboveLine)
			displayName += tr(" — Above Line");
		else if (currItem->anchorPosition().mode == AnchorPosition::Mode::Custom)
			displayName += tr(" — Anchored");
		QListWidgetItem *item = new QListWidgetItem(pm2, displayName, InlineViewWidget);
		item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
		item->setData(Qt::UserRole, currItem->inlineCharID);
	}
}

void InlinePalette::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
	}
	else
		DockPanelBase::changeEvent(e);
}

void InlinePalette::languageChange()
{
	setWindowTitle( tr( "Inline Items" ) );
}
