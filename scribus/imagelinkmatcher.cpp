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
	ImageLinkSearchTask search(nullptr, linkPaths, searchDirectory, recursive);
	search.start();
	search.runUntilFinished();
	return search.matches();
}

ImageLinkSearchTask::ImageLinkSearchTask(QObject* parent, const QStringList& linkPaths,
	const QString& searchDirectory, bool recursive)
	: DeferredTask(parent),
	  m_linkPaths(linkPaths),
	  m_searchDirectory(searchDirectory),
	  m_recursive(recursive)
{
	for (const QString& linkPath : m_linkPaths)
	{
		const QString fileName = QFileInfo(linkPath).fileName();
		m_requestedExactNames.insert(fileName);
		m_requestedFoldedNames.insert(fileName.toCaseFolded());
	}
}

ImageLinkSearchTask::~ImageLinkSearchTask() = default;

void ImageLinkSearchTask::start()
{
	const QDirIterator::IteratorFlags iteratorFlags = m_recursive
		? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
	m_iterator = std::make_unique<QDirIterator>(m_searchDirectory,
		QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, iteratorFlags);
	DeferredTask::start();
}

void ImageLinkSearchTask::next()
{
	constexpr int filesPerStep = 64;
	int filesProcessed = 0;
	while (filesProcessed < filesPerStep && m_iterator->hasNext())
	{
		const QFileInfo candidate(m_iterator->next());
		++filesProcessed;
		++m_scannedFileCount;
		if (!m_requestedFoldedNames.contains(candidate.fileName().toCaseFolded()))
			continue;
		const QString absolutePath = candidate.absoluteFilePath();
		if (m_requestedExactNames.contains(candidate.fileName()))
			m_exactNames.insert(candidate.fileName(), absolutePath);
		m_foldedNames.insert(candidate.fileName().toCaseFolded(), absolutePath);
	}
	if (m_iterator->hasNext())
		return;

	compileResults();
	done();
}

void ImageLinkSearchTask::compileResults()
{
	m_matches.clear();
	m_matches.reserve(m_linkPaths.size());
	for (const QString& linkPath : m_linkPaths)
	{
		ImageLinkMatch result;
		result.linkPath = QDir::cleanPath(QFileInfo(linkPath).absoluteFilePath());
		const QString fileName = QFileInfo(linkPath).fileName();
		result.candidatePaths = m_exactNames.values(fileName);
		if (result.candidatePaths.isEmpty())
			result.candidatePaths = m_foldedNames.values(fileName.toCaseFolded());
		result.candidatePaths.removeDuplicates();
		std::sort(result.candidatePaths.begin(), result.candidatePaths.end());
		m_matches.append(result);
	}
}
