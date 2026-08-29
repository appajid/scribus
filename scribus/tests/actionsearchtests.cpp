/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QtTest/QtTest>

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QSignalSpy>

#include "actionsearch.h"

class ActionSearchTests : public QObject
{
	Q_OBJECT

private slots:
	void discoversMenuMetadata();
	void executesOnlyAvailableActions();
};

void ActionSearchTests::discoversMenuMetadata()
{
	QMenuBar menuBar;
	QMenu* fileMenu = menuBar.addMenu(QStringLiteral("&File"));
	QAction* openAction = fileMenu->addAction(QStringLiteral("&Open..."));
	openAction->setShortcut(QKeySequence::Open);
	QMenu* exportMenu = fileMenu->addMenu(QStringLiteral("E&xport"));
	exportMenu->addAction(QStringLiteral("Export as &PDF"));
	QAction* disabledAction = fileMenu->addAction(QStringLiteral("Collect for Output"));
	disabledAction->setEnabled(false);

	ActionSearch search(&menuBar);
	search.update();
	QCOMPARE(search.actions().size(), 3);

	auto findAction = [&search](const QString& name) -> const ActionSearch::ActionInfo* {
		for (const auto& action : search.actions())
		{
			if (action.name == name)
				return &action;
		}
		return nullptr;
	};

	const auto* openInfo = findAction(QStringLiteral("Open..."));
	QVERIFY(openInfo);
	QCOMPARE(openInfo->menuPath, QStringLiteral("File"));
	QVERIFY(!openInfo->shortcut.isEmpty());
	QVERIFY(openInfo->enabled);

	const auto* pdfInfo = findAction(QStringLiteral("Export as PDF"));
	QVERIFY(pdfInfo);
	QCOMPARE(pdfInfo->menuPath, QStringLiteral("File > Export"));

	const auto* disabledInfo = findAction(QStringLiteral("Collect for Output"));
	QVERIFY(disabledInfo);
	QVERIFY(!disabledInfo->enabled);
}

void ActionSearchTests::executesOnlyAvailableActions()
{
	QMenuBar menuBar;
	QMenu* editMenu = menuBar.addMenu(QStringLiteral("&Edit"));
	QAction* enabledAction = editMenu->addAction(QStringLiteral("Enabled Action"));
	QAction* disabledAction = editMenu->addAction(QStringLiteral("Disabled Action"));
	disabledAction->setEnabled(false);

	ActionSearch search(&menuBar);
	search.update();

	QString enabledId;
	QString disabledId;
	for (const auto& action : search.actions())
	{
		if (action.name == QLatin1String("Enabled Action"))
			enabledId = action.id;
		else if (action.name == QLatin1String("Disabled Action"))
			disabledId = action.id;
	}

	QSignalSpy enabledSpy(enabledAction, &QAction::triggered);
	QSignalSpy disabledSpy(disabledAction, &QAction::triggered);
	QVERIFY(search.tryExecute(enabledId));
	QCOMPARE(enabledSpy.count(), 1);
	search.execute(QStringLiteral("Enabled Action (Edit)"));
	QCOMPARE(enabledSpy.count(), 2);
	QVERIFY(!search.tryExecute(disabledId));
	QCOMPARE(disabledSpy.count(), 0);
	QVERIFY(!search.tryExecute(QStringLiteral("missing")));
}

QTEST_MAIN(ActionSearchTests)
#include "actionsearchtests.moc"
