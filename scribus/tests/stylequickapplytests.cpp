/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QtTest/QtTest>

#include "stylequickapplymodel.h"

class StyleQuickApplyTests : public QObject
{
	Q_OBJECT

private slots:
	void showsAllStylesForEmptyQuery();
	void ranksStrongMatchesFirst();
	void supportsMultiTermAndFuzzyMatching();
	void filtersByStyleTypePrefix();
	void filtersTableAndCellStyles();
	void prioritizesFavoritesAndRecentStyles();
	void supportsUnicodeStyleNames();
};

void StyleQuickApplyTests::showsAllStylesForEmptyQuery()
{
	const QList<StyleSearchItem> styles = {
		{ QStringLiteral("Body"), StyleSearchType::character },
		{ QStringLiteral("Title"), StyleSearchType::paragraph },
		{ QStringLiteral("Body"), StyleSearchType::paragraph },
	};
	const auto matches = StyleQuickApplyModel::matches(styles, QString());
	QCOMPARE(matches.size(), 3);
	QCOMPARE(matches[0].name, QStringLiteral("Body"));
	QCOMPARE(matches[0].type, StyleSearchType::paragraph);
	QCOMPARE(matches[1].type, StyleSearchType::character);
}

void StyleQuickApplyTests::ranksStrongMatchesFirst()
{
	const QList<StyleSearchItem> styles = {
		{ QStringLiteral("Chapter Heading"), StyleSearchType::paragraph },
		{ QStringLiteral("Heading"), StyleSearchType::paragraph },
		{ QStringLiteral("Subheading"), StyleSearchType::paragraph },
	};
	const auto matches = StyleQuickApplyModel::matches(styles, QStringLiteral("heading"));
	QCOMPARE(matches.size(), 3);
	QCOMPARE(matches[0].name, QStringLiteral("Heading"));
	QCOMPARE(matches[1].name, QStringLiteral("Chapter Heading"));
	QCOMPARE(matches[2].name, QStringLiteral("Subheading"));
}

void StyleQuickApplyTests::supportsMultiTermAndFuzzyMatching()
{
	const QList<StyleSearchItem> styles = {
		{ QStringLiteral("Chapter Number"), StyleSearchType::character },
		{ QStringLiteral("Chapter Heading"), StyleSearchType::paragraph },
		{ QStringLiteral("Caption"), StyleSearchType::paragraph },
	};
	const auto multiTerm = StyleQuickApplyModel::matches(styles, QStringLiteral("chap head"));
	QCOMPARE(multiTerm.size(), 1);
	QCOMPARE(multiTerm[0].name, QStringLiteral("Chapter Heading"));

	const auto fuzzy = StyleQuickApplyModel::matches(styles, QStringLiteral("cptn"));
	QVERIFY(!fuzzy.isEmpty());
	QCOMPARE(fuzzy[0].name, QStringLiteral("Caption"));
}

void StyleQuickApplyTests::filtersByStyleTypePrefix()
{
	const QList<StyleSearchItem> styles = {
		{ QStringLiteral("Emphasis"), StyleSearchType::paragraph },
		{ QStringLiteral("Emphasis"), StyleSearchType::character },
	};
	const auto paragraphs = StyleQuickApplyModel::matches(styles, QStringLiteral("p: emphasis"));
	QCOMPARE(paragraphs.size(), 1);
	QCOMPARE(paragraphs[0].type, StyleSearchType::paragraph);

	const auto characters = StyleQuickApplyModel::matches(styles, QStringLiteral("character: emphasis"));
	QCOMPARE(characters.size(), 1);
	QCOMPARE(characters[0].type, StyleSearchType::character);
}

void StyleQuickApplyTests::filtersTableAndCellStyles()
{
	const QList<StyleSearchItem> styles = {
		{ QStringLiteral("Body"), StyleSearchType::paragraph },
		{ QStringLiteral("Body"), StyleSearchType::character },
		{ QStringLiteral("Body"), StyleSearchType::table },
		{ QStringLiteral("Body"), StyleSearchType::cell },
	};

	const auto tables = StyleQuickApplyModel::matches(styles, QStringLiteral("table: body"));
	QCOMPARE(tables.size(), 1);
	QCOMPARE(tables[0].type, StyleSearchType::table);

	const auto cells = StyleQuickApplyModel::matches(styles, QStringLiteral("cl: body"));
	QCOMPARE(cells.size(), 1);
	QCOMPARE(cells[0].type, StyleSearchType::cell);

	const auto all = StyleQuickApplyModel::matches(styles, QStringLiteral("body"));
	QCOMPARE(all.size(), 4);
	QCOMPARE(all[0].type, StyleSearchType::paragraph);
	QCOMPARE(all[1].type, StyleSearchType::character);
	QCOMPARE(all[2].type, StyleSearchType::table);
	QCOMPARE(all[3].type, StyleSearchType::cell);
}

void StyleQuickApplyTests::prioritizesFavoritesAndRecentStyles()
{
	QList<StyleSearchItem> styles = {
		{ QStringLiteral("Alpha"), StyleSearchType::paragraph },
		{ QStringLiteral("Beta"), StyleSearchType::paragraph },
		{ QStringLiteral("Gamma"), StyleSearchType::character },
		{ QStringLiteral("Delta"), StyleSearchType::character },
	};
	styles[1].favorite = true;
	styles[2].recentRank = 0;
	styles[3].recentRank = 1;

	const auto matches = StyleQuickApplyModel::matches(styles, QString());
	QCOMPARE(matches.size(), 4);
	QCOMPARE(matches[0].name, QStringLiteral("Beta"));
	QCOMPARE(matches[1].name, QStringLiteral("Gamma"));
	QCOMPARE(matches[2].name, QStringLiteral("Delta"));
	QCOMPARE(matches[3].name, QStringLiteral("Alpha"));
}

void StyleQuickApplyTests::supportsUnicodeStyleNames()
{
	const QList<StyleSearchItem> styles = {
		{ QStringLiteral("ప్రధాన శీర్షిక"), StyleSearchType::paragraph },
		{ QStringLiteral("मुख्य शीर्षक"), StyleSearchType::character },
		{ QStringLiteral("Body"), StyleSearchType::paragraph },
	};

	const auto telugu = StyleQuickApplyModel::matches(styles, QStringLiteral("శీర్షిక"));
	QCOMPARE(telugu.size(), 1);
	QCOMPARE(telugu[0].name, QStringLiteral("ప్రధాన శీర్షిక"));

	const auto hindi = StyleQuickApplyModel::matches(styles, QStringLiteral("शीर्षक"));
	QCOMPARE(hindi.size(), 1);
	QCOMPARE(hindi[0].name, QStringLiteral("मुख्य शीर्षक"));
}

QTEST_APPLESS_MAIN(StyleQuickApplyTests)
#include "stylequickapplytests.moc"
