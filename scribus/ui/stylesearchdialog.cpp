/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QString>

#include "iconmanager.h"
#include "modernui.h"
#include "stylesearchdialog.h"
#include "ui_stylesearchdialog.h"

StyleSearchDialog::StyleSearchDialog(QMainWindow *parent, const QList<StyleSearchItem>& styles) :
	QDialog{parent},
	ui{new Ui::StyleSearchDialog},
	styles{styles}
{
	ui->setupUi(this);
	ModernUI::applySurfaceStyle(this, "commandPalette");
	ui->filterLineEdit->setAccessibleName(tr("Search styles"));
	ui->filterLineEdit->setAccessibleDescription(
		tr("Search paragraph and character styles by name. Use p: or c: to filter by type."));
	ui->stylesListWidget->setAccessibleName(tr("Matching styles"));
	ui->stylesListWidget->setIconSize(QSize(20, 20));

	ui->filterLineEdit->installEventFilter(this);
	installEventFilter(this);

	connect(ui->filterLineEdit, &QLineEdit::textChanged,      this, &StyleSearchDialog::updateList);
	connect(this, &StyleSearchDialog::keyArrowUpPressed,     this, &StyleSearchDialog::moveSelectionUp);
	connect(this, &StyleSearchDialog::keyArrowDownPressed,   this, &StyleSearchDialog::moveSelectionDown);
	connect(ui->stylesListWidget, &QListWidget::itemDoubleClicked, this, [this]() { acceptCurrentStyle(); });
	updateList();
	ui->filterLineEdit->setFocus();
}

StyleSearchDialog::~StyleSearchDialog()
{
	delete ui;
}

StyleSearchItem StyleSearchDialog::getStyle() const
{
	QListWidgetItem* item = ui->stylesListWidget->currentItem();
	if (!item || !item->data(Qt::UserRole + 2).toBool())
		return {"", StyleSearchType::paragraph};
	return {
		item->data(Qt::UserRole).toString(),
		static_cast<StyleSearchType>(item->data(Qt::UserRole + 1).toInt())
	};
}

/**
 * @brief capture return, arrow keys, and tab
 */
bool StyleSearchDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui->filterLineEdit)
	{
		if (event->type() == QEvent::KeyPress)
		{
			return filterLineEditKeyPress(static_cast<QKeyEvent*>(event));
		}
	}
	return false;
}

bool StyleSearchDialog::filterLineEditKeyPress(QKeyEvent * event)
{
	switch (event->key())
	{
		case Qt::Key_Enter:
		case Qt::Key_Return:
			acceptCurrentStyle();
			return true;
		case Qt::Key_Up:
			emit keyArrowUpPressed();
			return true;
		case Qt::Key_Down:
		case Qt::Key_Tab:
			emit keyArrowDownPressed();
			return true;
		default:
			return false;
	}
}

void StyleSearchDialog::moveSelectionUp()
{
	selectNextEnabled(-1);
}

void StyleSearchDialog::moveSelectionDown()
{
	selectNextEnabled(1);
}

void StyleSearchDialog::acceptCurrentStyle()
{
	if (!getStyle().name.isEmpty())
		accept();
}

void StyleSearchDialog::selectNextEnabled(int step)
{
	const int count = ui->stylesListWidget->count();
	if (count == 0)
		return;
	int row = ui->stylesListWidget->currentRow();
	if (row < 0)
		row = (step > 0) ? -1 : count;
	for (int attempts = 0; attempts < count; ++attempts)
	{
		row = (row + step + count) % count;
		QListWidgetItem* item = ui->stylesListWidget->item(row);
		if (item->data(Qt::UserRole + 2).toBool())
		{
			ui->stylesListWidget->setCurrentRow(row);
			return;
		}
	}
}


/**
 * Fill the list with all styles that match the filter.
 * Results are ranked by exact, prefix, word-prefix, substring, and fuzzy
 * subsequence matches. An empty filter intentionally shows all styles.
 */
void StyleSearchDialog::updateList()
{
	ui->stylesListWidget->clear();

	IconManager &im = IconManager::instance();
	const QIcon iconParagraph(im.loadPixmap("paragraph-style"));
	const QIcon iconCharacter(im.loadPixmap("character-style"));
	const QList<StyleSearchItem> matches = StyleQuickApplyModel::matches(styles, ui->filterLineEdit->text());
	for (const StyleSearchItem& style : matches)
	{
		const bool paragraph = style.type == StyleSearchType::paragraph;
		const QString typeName = paragraph ? tr("Paragraph Style") : tr("Character Style");
		auto* item = new QListWidgetItem(
			paragraph ? iconParagraph : iconCharacter,
			tr("%1  —  %2").arg(style.name, typeName),
			ui->stylesListWidget);
		item->setData(Qt::UserRole, style.name);
		item->setData(Qt::UserRole + 1, static_cast<int>(style.type));
		item->setData(Qt::UserRole + 2, true);
		item->setToolTip(tr("Apply %1").arg(typeName.toLower()));
		item->setData(Qt::AccessibleTextRole, tr("%1, %2").arg(style.name, typeName));
	}

	ui->resultCountLabel->setText(matches.count() == 1
		? tr("1 style")
		: tr("%1 styles").arg(matches.count()));
	if (matches.isEmpty())
	{
		auto* item = new QListWidgetItem(tr("No matching styles"), ui->stylesListWidget);
		item->setData(Qt::UserRole + 2, false);
		item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
	}
	selectNextEnabled(1);
}
