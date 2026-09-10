/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STYLESEARCHDIALOG_H
#define STYLESEARCHDIALOG_H

#include <QDialog>
#include <QList>
#include <QSet>
#include <QStringList>

#include "stylesearch.h"

class QEvent;
class QKeyEvent;
class QMainWindow;
class QString;
class PrefsContext;

namespace Ui { class StyleSearchDialog; }

class StyleSearchDialog : public QDialog
{
	Q_OBJECT

public:
	StyleSearchDialog(QMainWindow *parent, const QList<StyleSearchItem> &styleNames);
	~StyleSearchDialog();

	StyleSearchItem getStyle() const;

protected:
	bool eventFilter(QObject *obj, QEvent *ev) override;

private:
	Ui::StyleSearchDialog *ui { nullptr };
	QList<StyleSearchItem> styles;
	PrefsContext* m_prefs { nullptr };
	QSet<QString> m_favoriteKeys;
	QStringList m_recentKeys;

	bool filterLineEditKeyPress(QKeyEvent * event);
	void acceptCurrentStyle();
	void selectNextEnabled(int step);
	void restoreUsage();
	void recordRecent(const StyleSearchItem& style);
	void saveFavorites();
	void updatePreview();
	void toggleFavorite();

private slots:
	void moveSelectionUp();
	void moveSelectionDown();
	void updateList();

signals:
	void keyArrowUpPressed();
	void keyArrowDownPressed();
};

#endif
