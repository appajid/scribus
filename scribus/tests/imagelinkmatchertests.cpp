/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "imagelinkmatcher.h"

class ImageLinkMatcherTests : public QObject
{
	Q_OBJECT

private slots:
	void findsUniqueAndMissingFiles();
	void reportsAmbiguousMatches();
	void prefersExactCaseAndSupportsFallback();
	void respectsRecursiveOption();
	void incrementalSearchCompletes();
	void incrementalSearchCanBeCancelled();
};

static void createFile(const QString& path)
{
	QFile file(path);
	QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
	file.write("image-placeholder");
}

void ImageLinkMatcherTests::findsUniqueAndMissingFiles()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	createFile(directory.filePath(QStringLiteral("cover.png")));

	const auto matches = findImageLinkMatches(
		{ QStringLiteral("/old/assets/cover.png"), QStringLiteral("/old/assets/missing.png") },
		directory.path());
	QCOMPARE(matches.size(), 2);
	QVERIFY(matches.at(0).isUnique());
	QCOMPARE(matches.at(0).candidatePaths.first(), directory.filePath(QStringLiteral("cover.png")));
	QVERIFY(matches.at(1).candidatePaths.isEmpty());
}

void ImageLinkMatcherTests::reportsAmbiguousMatches()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("print"))));
	QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("web"))));
	createFile(directory.filePath(QStringLiteral("print/logo.png")));
	createFile(directory.filePath(QStringLiteral("web/logo.png")));

	const auto matches = findImageLinkMatches({ QStringLiteral("/old/logo.png") }, directory.path());
	QCOMPARE(matches.size(), 1);
	QVERIFY(matches.first().isAmbiguous());
	QCOMPARE(matches.first().candidatePaths.size(), 2);
}

void ImageLinkMatcherTests::prefersExactCaseAndSupportsFallback()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("exact"))));
	QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("alternate"))));
	createFile(directory.filePath(QStringLiteral("exact/Logo.PNG")));
	createFile(directory.filePath(QStringLiteral("alternate/logo.png")));
	createFile(directory.filePath(QStringLiteral("photo.jpg")));

	const auto matches = findImageLinkMatches(
		{ QStringLiteral("/old/Logo.PNG"), QStringLiteral("/old/PHOTO.JPG") }, directory.path());
	QVERIFY(matches.at(0).isUnique());
	QCOMPARE(matches.at(0).candidatePaths.first(), directory.filePath(QStringLiteral("exact/Logo.PNG")));
	QVERIFY(matches.at(1).isUnique());
	QCOMPARE(matches.at(1).candidatePaths.first(), directory.filePath(QStringLiteral("photo.jpg")));
}

void ImageLinkMatcherTests::respectsRecursiveOption()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("nested"))));
	createFile(directory.filePath(QStringLiteral("nested/inside.png")));

	const auto shallow = findImageLinkMatches(
		{ QStringLiteral("/old/inside.png") }, directory.path(), false);
	QVERIFY(shallow.first().candidatePaths.isEmpty());
	const auto recursive = findImageLinkMatches(
		{ QStringLiteral("/old/inside.png") }, directory.path(), true);
	QVERIFY(recursive.first().isUnique());
}

void ImageLinkMatcherTests::incrementalSearchCompletes()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	createFile(directory.filePath(QStringLiteral("cover.png")));

	ImageLinkSearchTask search(nullptr, { QStringLiteral("/old/cover.png") }, directory.path());
	QSignalSpy finishedSpy(&search, &DeferredTask::finished);
	search.start();
	QTRY_COMPARE(finishedSpy.count(), 1);
	QVERIFY(search.isFinished());
	QCOMPARE(search.scannedFileCount(), 1);
	QVERIFY(search.matches().first().isUnique());
}

void ImageLinkMatcherTests::incrementalSearchCanBeCancelled()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());

	ImageLinkSearchTask search(nullptr, { QStringLiteral("/old/cover.png") }, directory.path());
	QSignalSpy abortedSpy(&search, &DeferredTask::aborted);
	search.start();
	search.cancel();
	QCOMPARE(abortedSpy.count(), 1);
	QCOMPARE(abortedSpy.first().first().toBool(), true);
	QVERIFY(!search.isFinished());
}

QTEST_GUILESS_MAIN(ImageLinkMatcherTests)

#include "imagelinkmatchertests.moc"
