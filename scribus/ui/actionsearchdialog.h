/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ACTIONSEARCHDIALOG_H
#define ACTIONSEARCHDIALOG_H

class QKeyEvent;
class QMainWindow;
class QString;
class QEvent;

#include <QDialog>
#include <QList>

#include "actionsearch.h"

namespace Ui { class ActionSearchDialog; }

class ActionSearchDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ActionSearchDialog(QMainWindow *parent, const QList<ActionSearch::ActionInfo>& actions);
	~ActionSearchDialog();

	QString actionId() const;

protected:
	bool eventFilter(QObject *obj, QEvent *ev) override;

private:
	Ui::ActionSearchDialog *ui { nullptr };
	QList<ActionSearch::ActionInfo> m_actions;

	bool filterLineEditKeyPress(QKeyEvent *event);
	void acceptCurrentAction();
	void selectNextEnabled(int step);

private slots:
	void moveSelectionUp();
	void moveSelectionDown();
	void updateList();

signals:
	void keyArrowUpPressed();
	void keyArrowDownPressed();
};

#endif
