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
	void emptyDirectoriesYieldAndAllowCancellation();
	void sourceFoldersStopAtRoot();
	void sourceFoldersIncludeMissingAncestorsOnce();
	void relativeMappingPath_data();
	void relativeMappingPath();
	void mapsSubfoldersWithoutFilenameFallback();
	void mappedSearchSupportsCaseFallback();
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

void ImageLinkMatcherTests::emptyDirectoriesYieldAndAllowCancellation()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	for (int i = 0; i < 150; ++i)
		QVERIFY(QDir().mkpath(directory.filePath(QString::number(i))));

	class SteppedSearch : public ImageLinkSearchTask
	{
	public:
		using ImageLinkSearchTask::ImageLinkSearchTask;
		using ImageLinkSearchTask::next;
	};
	SteppedSearch search(nullptr, { "/old/cover.png" }, directory.path());
	QSignalSpy finishedSpy(&search, &DeferredTask::finished);
	QSignalSpy abortedSpy(&search, &DeferredTask::aborted);
	search.start();
	search.next();
	// Even with no files, one step must yield before traversing the whole tree.
	QVERIFY(!search.isFinished());
	QCOMPARE(search.scannedFileCount(), 0);
	search.cancel();
	QCOMPARE(abortedSpy.count(), 1);
	QCOMPARE(finishedSpy.count(), 0);
	QVERIFY(search.matches().isEmpty());
}

void ImageLinkMatcherTests::sourceFoldersStopAtRoot()
{
	const QString root = QDir::rootPath();
	QCOMPARE(imageLinkSourceFolders({ QDir(root).filePath("logo.png") }), QStringList { root });
	QVERIFY(imageLinkSourceFolders({ QString() }).isEmpty());
}

void ImageLinkMatcherTests::sourceFoldersIncludeMissingAncestorsOnce()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString missing = directory.filePath("missing/assets");
	QVERIFY(!QDir(missing).exists());
	const auto folders = imageLinkSourceFolders({ missing + "/print/logo.png", missing + "/web/logo.png" });
	QCOMPARE(folders.count(missing), 1);
	QCOMPARE(folders.count(directory.path()), 1);
	QVERIFY(folders.contains(missing + "/print"));
	QVERIFY(folders.contains(missing + "/web"));
	QCOMPARE(folders.count(QDir::rootPath()), 1);
	for (const QString& folder : folders)
		QVERIFY(!folder.split('/').contains(".."));
}

void ImageLinkMatcherTests::relativeMappingPath_data()
{
	QTest::addColumn<QString>("link");
	QTest::addColumn<QString>("source");
	QTest::addColumn<QString>("relative");
	QTest::newRow("unix") << "/old/assets/print/logo.png" << "/old/assets/" << "print/logo.png";
	QTest::newRow("sibling-prefix") << "/old/assets-other/logo.png" << "/old/assets" << "";
	QTest::newRow("outside") << "/elsewhere/logo.png" << "/old/assets" << "";
	QTest::newRow("escape") << "/old/assets/../logo.png" << "/old/assets" << "";
	QTest::newRow("normalize") << "/old/assets/print/../logo.png" << "/old/assets" << "logo.png";
	QTest::newRow("unix-case") << "/Old/assets/logo.png" << "/old/assets" << "";
	QTest::newRow("windows") << "C:\\Old\\Assets\\print\\logo.png" << "c:/old/assets" << "print/logo.png";
	QTest::newRow("other-drive") << "D:/Old/Assets/logo.png" << "C:/Old/Assets" << "";
	QTest::newRow("drive-root") << "C:/Assets/logo.png" << "C:/" << "Assets/logo.png";
	QTest::newRow("network") << "\\\\server\\share\\Assets\\print\\logo.png" << "//SERVER/share/Assets" << "print/logo.png";
	QTest::newRow("other-share") << "//server/share2/Assets/logo.png" << "//server/share" << "";
	QTest::newRow("empty") << "" << "/old/assets" << "";
	QTest::newRow("relative-source") << "/old/assets/logo.png" << "old/assets" << "";
	QTest::newRow("relative-link") << "old/assets/logo.png" << "/old/assets" << "";
}

void ImageLinkMatcherTests::relativeMappingPath()
{
	QFETCH(QString, link);
	QFETCH(QString, source);
	QFETCH(QString, relative);
	QCOMPARE(imageLinkRelativePath(link, source), relative);
}

void ImageLinkMatcherTests::mapsSubfoldersWithoutFilenameFallback()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	for (const QString& folder : { QStringLiteral("print"), QStringLiteral("web") })
	{
		QVERIFY(QDir().mkpath(directory.filePath(folder)));
		createFile(directory.filePath(folder + QStringLiteral("/logo.png")));
	}
	ImageLinkSearchTask search(nullptr, { "/old/assets/print/logo.png", "/old/assets/web/logo.png",
		"/old/assets/missing/logo.png", "/old/assets-other/print/logo.png" }, directory.path(), true, "/old/assets");
	search.start();
	search.runUntilFinished();
	const auto matches = search.matches();
	QCOMPARE(matches.size(), 4);
	QCOMPARE(matches.at(0).candidatePaths, QStringList { directory.filePath("print/logo.png") });
	QCOMPARE(matches.at(1).candidatePaths, QStringList { directory.filePath("web/logo.png") });
	QVERIFY(matches.at(2).candidatePaths.isEmpty());
	QVERIFY(matches.at(3).candidatePaths.isEmpty());
}

void ImageLinkMatcherTests::mappedSearchSupportsCaseFallback()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	QVERIFY(QDir().mkpath(directory.filePath("Print")));
	createFile(directory.filePath("Print/Logo.PNG"));
	ImageLinkSearchTask search(nullptr, { "C:\\old\\assets\\print\\logo.png" },
		directory.path(), true, "C:/old/assets");
	QSignalSpy finishedSpy(&search, &DeferredTask::finished);
	search.start();
	QTRY_COMPARE(finishedSpy.count(), 1);
	QCOMPARE(search.matches().first().candidatePaths, QStringList { directory.filePath("Print/Logo.PNG") });
}

QTEST_GUILESS_MAIN(ImageLinkMatcherTests)

#include "imagelinkmatchertests.moc"
