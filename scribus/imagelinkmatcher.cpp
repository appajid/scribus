/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "imagelinkmatcher.h"

#include <algorithm>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMultiHash>

QVector<ImageLinkMatch> findImageLinkMatches(const QStringList& linkPaths,
	const QString& searchDirectory, bool recursive)
{
	QMultiHash<QString, QString> exactNames;
	QMultiHash<QString, QString> foldedNames;
	const QDirIterator::IteratorFlags iteratorFlags = recursive
		? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
	QDirIterator iterator(searchDirectory,
		QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, iteratorFlags);
	while (iterator.hasNext())
	{
		const QFileInfo candidate(iterator.next());
		const QString absolutePath = candidate.absoluteFilePath();
		exactNames.insert(candidate.fileName(), absolutePath);
		foldedNames.insert(candidate.fileName().toCaseFolded(), absolutePath);
	}

	QVector<ImageLinkMatch> results;
	results.reserve(linkPaths.size());
	for (const QString& linkPath : linkPaths)
	{
		ImageLinkMatch result;
		result.linkPath = QDir::cleanPath(QFileInfo(linkPath).absoluteFilePath());
		const QString fileName = QFileInfo(linkPath).fileName();
		result.candidatePaths = exactNames.values(fileName);
		if (result.candidatePaths.isEmpty())
			result.candidatePaths = foldedNames.values(fileName.toCaseFolded());
		result.candidatePaths.removeDuplicates();
		std::sort(result.candidatePaths.begin(), result.candidatePaths.end());
		results.append(result);
	}
	return results;
}
