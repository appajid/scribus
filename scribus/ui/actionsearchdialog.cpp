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

#include <algorithm>
#include <utility>

#include "actionsearchdialog.h"
#include "modernui.h"
#include "ui_actionsearchdialog.h"

ActionSearchDialog::ActionSearchDialog(QMainWindow *parent, const QList<ActionSearch::ActionInfo>& actions) :
	QDialog{parent},
	ui{new Ui::ActionSearchDialog},
	m_actions{actions}
{
	ui->setupUi(this);
	ModernUI::applySurfaceStyle(this, "commandPalette");
	ui->filterLineEdit->setAccessibleName(tr("Search commands"));
	ui->filterLineEdit->setAccessibleDescription(tr("Search by command name, menu, or keyboard shortcut"));
	ui->actionsListWidget->setAccessibleName(tr("Matching commands"));
	ui->actionsListWidget->setIconSize(QSize(20, 20));

	ui->filterLineEdit->installEventFilter(this);
	installEventFilter(this);

	connect(ui->filterLineEdit, &QLineEdit::textChanged,      this, &ActionSearchDialog::updateList);
	connect(this, &ActionSearchDialog::keyArrowUpPressed,     this, &ActionSearchDialog::moveSelectionUp);
	connect(this, &ActionSearchDialog::keyArrowDownPressed,   this, &ActionSearchDialog::moveSelectionDown);
	connect(ui->actionsListWidget, &QListWidget::itemDoubleClicked, this, [this]() { acceptCurrentAction(); });
	updateList();
	ui->filterLineEdit->setFocus();
}

ActionSearchDialog::~ActionSearchDialog()
{
	delete ui;
}

QString ActionSearchDialog::actionId() const
{
	QListWidgetItem* item = ui->actionsListWidget->currentItem();
	if (!item || !item->data(Qt::UserRole + 1).toBool())
		return QString();
	return item->data(Qt::UserRole).toString();
}

/**
 * @brief capture return, arrow keys, and tab
 */
bool ActionSearchDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == ui->filterLineEdit)
	{
		if (event->type() == QEvent::KeyPress)
			return filterLineEditKeyPress(static_cast<QKeyEvent*>(event));
	}
	return false;
}

bool ActionSearchDialog::filterLineEditKeyPress(QKeyEvent *event)
{
	switch (event->key())
	{
		case Qt::Key_Enter:
		case Qt::Key_Return:
			acceptCurrentAction();
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

void ActionSearchDialog::moveSelectionUp()
{
	selectNextEnabled(-1);
}

void ActionSearchDialog::moveSelectionDown()
{
	selectNextEnabled(1);
}

void ActionSearchDialog::acceptCurrentAction()
{
	if (!actionId().isEmpty())
		accept();
}

void ActionSearchDialog::selectNextEnabled(int step)
{
	const int count = ui->actionsListWidget->count();
	if (count == 0)
		return;

	int row = ui->actionsListWidget->currentRow();
	if (row < 0)
		row = (step > 0) ? -1 : count;
	for (int attempts = 0; attempts < count; ++attempts)
	{
		row = (row + step + count) % count;
		QListWidgetItem* item = ui->actionsListWidget->item(row);
		if (item->data(Qt::UserRole + 1).toBool())
		{
			ui->actionsListWidget->setCurrentRow(row);
			return;
		}
	}
}


/**
 * Fill the list with all actions that match the filter.
 * If the filter contains multiple words, accepts all actions that
 * contain all the words
 */
void ActionSearchDialog::updateList()
{
	ui->actionsListWidget->clear();

	QString filter = ui->filterLineEdit->text().trimmed();
	if (filter.startsWith(QLatin1Char('?')))
		filter = filter.sliced(1).trimmed();
	const QStringList words = filter.split(QLatin1Char(' '), Qt::SkipEmptyParts);
	QList<ActionSearch::ActionInfo> matches;
	for (const ActionSearch::ActionInfo& action : std::as_const(m_actions))
	{
		const QString searchable = action.name + QLatin1Char(' ') + action.menuPath + QLatin1Char(' ') + action.shortcut;
		bool matchesAll = true;
		for (const QString& word : words)
		{
			if (!searchable.contains(word, Qt::CaseInsensitive))
			{
				matchesAll = false;
				break;
			}
		}
		if (matchesAll)
			matches.append(action);
	}

	std::sort(matches.begin(), matches.end(), [&filter](const auto& left, const auto& right) {
		const bool leftStarts = !filter.isEmpty() && left.name.startsWith(filter, Qt::CaseInsensitive);
		const bool rightStarts = !filter.isEmpty() && right.name.startsWith(filter, Qt::CaseInsensitive);
		if (leftStarts != rightStarts)
			return leftStarts;
		const int nameOrder = QString::compare(left.name, right.name, Qt::CaseInsensitive);
		return nameOrder == 0 ? QString::compare(left.menuPath, right.menuPath, Qt::CaseInsensitive) < 0 : nameOrder < 0;
	});

	for (const ActionSearch::ActionInfo& action : std::as_const(matches))
	{
		QString displayText = action.name;
		if (!action.menuPath.isEmpty())
			displayText += tr("  —  %1").arg(action.menuPath);
		if (!action.shortcut.isEmpty())
			displayText += tr("  [%1]").arg(action.shortcut);

		auto* item = new QListWidgetItem(action.icon, displayText, ui->actionsListWidget);
		item->setData(Qt::UserRole, action.id);
		item->setData(Qt::UserRole + 1, action.enabled);
		QString toolTip = action.menuPath;
		if (!action.enabled)
			toolTip += (toolTip.isEmpty() ? QString() : QStringLiteral("\n")) + tr("Unavailable in the current context");
		item->setToolTip(toolTip);
		if (!action.enabled)
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
	}

	ui->resultCountLabel->setText(matches.count() == 1
		? tr("1 command")
		: tr("%1 commands").arg(matches.count()));
	if (matches.isEmpty())
	{
		auto* item = new QListWidgetItem(tr("No matching commands"), ui->actionsListWidget);
		item->setData(Qt::UserRole + 1, false);
		item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
	}

	selectNextEnabled(1);
}
