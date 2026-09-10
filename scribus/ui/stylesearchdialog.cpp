/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QEvent>
#include <QFont>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QString>
#include <QToolButton>
#include <QUrl>

#include "iconmanager.h"
#include "modernui.h"
#include "prefscontext.h"
#include "prefsfile.h"
#include "prefsmanager.h"
#include "stylesearchdialog.h"
#include "ui_stylesearchdialog.h"

namespace
{
QString storageKey(const StyleSearchItem& style)
{
	const QString prefix = style.type == StyleSearchType::paragraph
		? QStringLiteral("P/") : QStringLiteral("C/");
	return prefix + QString::fromLatin1(QUrl::toPercentEncoding(style.name));
}

QString displayText(const StyleSearchItem& style, const QString& typeName, const QString& recentText)
{
	QString text = style.favorite ? QStringLiteral("★ ") + style.name : style.name;
	text += QStringLiteral("  —  ") + typeName;
	if (style.recentRank >= 0)
		text += QStringLiteral("  •  ") + recentText;
	return text;
}
}

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
	ui->favoriteButton->setAccessibleName(tr("Favourite style"));
	ModernUI::markSections(this);
	m_prefs = PrefsManager::instance().prefsFile->getContext("QuickApplyStyles");
	restoreUsage();

	ui->filterLineEdit->installEventFilter(this);
	installEventFilter(this);

	connect(ui->filterLineEdit, &QLineEdit::textChanged,      this, &StyleSearchDialog::updateList);
	connect(this, &StyleSearchDialog::keyArrowUpPressed,     this, &StyleSearchDialog::moveSelectionUp);
	connect(this, &StyleSearchDialog::keyArrowDownPressed,   this, &StyleSearchDialog::moveSelectionDown);
	connect(ui->stylesListWidget, &QListWidget::itemDoubleClicked, this, [this]() { acceptCurrentStyle(); });
	connect(ui->stylesListWidget, &QListWidget::currentItemChanged, this, [this]() { updatePreview(); });
	connect(ui->favoriteButton, &QToolButton::clicked, this, &StyleSearchDialog::toggleFavorite);
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
	const StyleSearchItem style = getStyle();
	if (style.name.isEmpty())
		return;
	recordRecent(style);
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

void StyleSearchDialog::restoreUsage()
{
	if (!m_prefs)
		return;
	const QStringList favorites = m_prefs->get("favorites").split(QLatin1Char('\n'), Qt::SkipEmptyParts);
	m_favoriteKeys = QSet<QString>(favorites.cbegin(), favorites.cend());
	m_recentKeys = m_prefs->get("recent").split(QLatin1Char('\n'), Qt::SkipEmptyParts);
	for (StyleSearchItem& style : styles)
	{
		const QString key = storageKey(style);
		style.favorite = m_favoriteKeys.contains(key);
		style.recentRank = m_recentKeys.indexOf(key);
	}
}

void StyleSearchDialog::recordRecent(const StyleSearchItem& style)
{
	const QString key = storageKey(style);
	m_recentKeys.removeAll(key);
	m_recentKeys.prepend(key);
	while (m_recentKeys.size() > 8)
		m_recentKeys.removeLast();
	if (m_prefs)
		m_prefs->set("recent", m_recentKeys.join(QLatin1Char('\n')));
}

void StyleSearchDialog::saveFavorites()
{
	if (!m_prefs)
		return;
	QStringList favorites(m_favoriteKeys.cbegin(), m_favoriteKeys.cend());
	favorites.sort(Qt::CaseInsensitive);
	m_prefs->set("favorites", favorites.join(QLatin1Char('\n')));
}

void StyleSearchDialog::updatePreview()
{
	const StyleSearchItem selected = getStyle();
	const QString key = selected.name.isEmpty() ? QString() : storageKey(selected);
	const StyleSearchItem* style = nullptr;
	for (const StyleSearchItem& candidate : styles)
	{
		if (storageKey(candidate) == key)
		{
			style = &candidate;
			break;
		}
	}
	if (!style)
	{
		ui->previewGroup->setEnabled(false);
		ui->previewNameLabel->setText(tr("No style selected"));
		ui->previewSampleLabel->clear();
		ui->previewDetailsLabel->clear();
		ui->favoriteButton->setChecked(false);
		return;
	}

	ui->previewGroup->setEnabled(true);
	ui->previewNameLabel->setText(style->name);
	ui->previewSampleLabel->setText(tr("Aa Bb Cc 123 — The quick brown fox"));
	QFont previewFont = font();
	if (!style->fontFamily.isEmpty())
		previewFont.setFamily(style->fontFamily);
	if (!style->fontStyle.isEmpty())
		previewFont.setStyleName(style->fontStyle);
	if (style->fontSize > 0.0)
		previewFont.setPointSizeF(qBound(10.0, style->fontSize, 28.0));
	ui->previewSampleLabel->setFont(previewFont);
	ui->previewSampleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

	QStringList details;
	details.append(style->type == StyleSearchType::paragraph ? tr("Paragraph Style") : tr("Character Style"));
	if (!style->fontFamily.isEmpty())
		details.append(style->fontStyle.isEmpty()
			? style->fontFamily : tr("%1 %2").arg(style->fontFamily, style->fontStyle));
	if (style->fontSize > 0.0)
		details.append(tr("%1 pt").arg(style->fontSize, 0, 'f', 1));
	if (style->type == StyleSearchType::paragraph)
	{
		const QStringList alignments = {
			tr("Left"), tr("Centre"), tr("Right"), tr("Justified"), tr("Forced Justified")
		};
		if (style->paragraphAlignment >= 0 && style->paragraphAlignment < alignments.size())
		{
			details.append(alignments.at(style->paragraphAlignment));
			if (style->paragraphAlignment == 0)
				ui->previewSampleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
			else if (style->paragraphAlignment == 2)
				ui->previewSampleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
		}
	}
	if (!style->parentStyle.isEmpty())
		details.append(tr("Based on %1").arg(style->parentStyle));
	ui->previewDetailsLabel->setText(details.join(QStringLiteral("  •  ")));
	ui->favoriteButton->setChecked(style->favorite);
	ui->favoriteButton->setText(style->favorite ? tr("★ Favourite") : tr("☆ Add Favourite"));
}

void StyleSearchDialog::toggleFavorite()
{
	const StyleSearchItem selected = getStyle();
	if (selected.name.isEmpty())
		return;
	const QString key = storageKey(selected);
	const bool favorite = !m_favoriteKeys.contains(key);
	if (favorite)
		m_favoriteKeys.insert(key);
	else
		m_favoriteKeys.remove(key);
	for (StyleSearchItem& style : styles)
	{
		if (storageKey(style) == key)
		{
			style.favorite = favorite;
			break;
		}
	}
	saveFavorites();
	QListWidgetItem* item = ui->stylesListWidget->currentItem();
	if (item)
	{
		const QString typeName = selected.type == StyleSearchType::paragraph
			? tr("Paragraph Style") : tr("Character Style");
		StyleSearchItem updated = selected;
		updated.favorite = favorite;
		item->setText(displayText(updated, typeName, tr("Recent")));
	}
	updatePreview();
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
			displayText(style, typeName, tr("Recent")),
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
