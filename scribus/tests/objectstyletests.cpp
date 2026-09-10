/*
 For general Scribus (>=1.3.2) copyright and licensing information please refer
 to the COPYING file provided with the program. Following this notice may exist
 a copyright and/or license notice that predates the release of Scribus 1.3.2
 for which a new license (GPL+exception) is in place.
 */

#include <QtTest>

#include "styles/objectstyle.h"
#include "styles/styleset.h"

class ObjectStyleTests : public QObject
{
	Q_OBJECT

private slots:
	void defaultsAreInheritedAndGeometryNeutral();
	void resolvesInheritanceAndLocalOverrides();
	void comparesAndErasesOverrides();
	void collectsAndReplacesNamedResources();
	void preservesUnicodeNamesAndShortcuts();
};

void ObjectStyleTests::defaultsAreInheritedAndGeometryNeutral()
{
	ObjectStyle style;
	QVERIFY(style.isInhFillColor());
	QVERIFY(style.isInhLineColor());
	QVERIFY(style.isInhCornerRadius());
	QCOMPARE(style.fillColor(), QStringLiteral("None"));
	QCOMPARE(style.lineColor(), QStringLiteral("None"));
	QCOMPARE(style.lineWidth(), 0.0);
	QCOMPARE(style.cornerRadius(), 0.0);
}

void ObjectStyleTests::resolvesInheritanceAndLocalOverrides()
{
	StyleSet<ObjectStyle> styles;
	ObjectStyle base;
	base.setName(QStringLiteral("Base"));
	base.setFillColor(QStringLiteral("Blue"));
	base.setLineColor(QStringLiteral("Black"));
	styles.create(base);

	ObjectStyle child;
	child.setName(QStringLiteral("Child"));
	child.setParent(QStringLiteral("Base"));
	child.setFillColor(QStringLiteral("Red"));
	ObjectStyle* storedChild = styles.create(child);

	QCOMPARE(storedChild->fillColor(), QStringLiteral("Red"));
	QCOMPARE(storedChild->lineColor(), QStringLiteral("Black"));
	QVERIFY(storedChild->isDefLineColor());
	QVERIFY(!storedChild->isInhFillColor());
}

void ObjectStyleTests::comparesAndErasesOverrides()
{
	ObjectStyle first;
	first.setFillShade(75.0);
	first.setLineWidth(2.5);
	ObjectStyle second(first);
	QVERIFY(first.equiv(second));

	second.setCornerRadius(4.0);
	QVERIFY(!first.equiv(second));
	second.erase();
	QVERIFY(second.isInhCornerRadius());
	QVERIFY(second.isInhLineWidth());
	QCOMPARE(second.cornerRadius(), 0.0);
}

void ObjectStyleTests::collectsAndReplacesNamedResources()
{
	ObjectStyle style;
	style.setParent(QStringLiteral("Base Object"));
	style.setFillColor(QStringLiteral("Brand Blue"));
	style.setLineColor(QStringLiteral("Registration"));
	style.setCustomLineStyle(QStringLiteral("Rule"));

	ResourceCollection resources;
	style.getNamedResources(resources);
	QVERIFY(resources.objectStyles().contains(QStringLiteral("Base Object")));
	QVERIFY(resources.colors().contains(QStringLiteral("Brand Blue")));
	QVERIFY(resources.colors().contains(QStringLiteral("Registration")));
	QVERIFY(resources.lineStyles().contains(QStringLiteral("Rule")));

	ResourceCollection replacements;
	replacements.mapObjectStyle(QStringLiteral("Base Object"), QStringLiteral("Foundation"));
	replacements.mapColor(QStringLiteral("Brand Blue"), QStringLiteral("Corporate Blue"));
	replacements.mapColor(QStringLiteral("Registration"), QStringLiteral("Black"));
	replacements.mapLineStyle(QStringLiteral("Rule"), QStringLiteral("Thin Rule"));
	style.replaceNamedResources(replacements);
	QCOMPARE(style.parent(), QStringLiteral("Foundation"));
	QCOMPARE(style.fillColor(), QStringLiteral("Corporate Blue"));
	QCOMPARE(style.lineColor(), QStringLiteral("Black"));
	QCOMPARE(style.customLineStyle(), QStringLiteral("Thin Rule"));
}

void ObjectStyleTests::preservesUnicodeNamesAndShortcuts()
{
	ObjectStyle style;
	style.setName(QStringLiteral("చిత్ర చట్రం"));
	style.setShortcut(QStringLiteral("Ctrl+Alt+7"));
	QCOMPARE(style.name(), QStringLiteral("చిత్ర చట్రం"));
	QCOMPARE(style.shortcut(), QStringLiteral("Ctrl+Alt+7"));
}

QTEST_APPLESS_MAIN(ObjectStyleTests)

#include "objectstyletests.moc"
