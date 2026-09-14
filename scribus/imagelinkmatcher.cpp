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

namespace
{
bool isWindowsImagePath(const QString& path)
{
	return (path.size() >= 3 && path.at(0).isLetter() && path.at(1) == QLatin1Char(':')
		&& (path.at(2) == QLatin1Char('/') || path.at(2) == QLatin1Char('\\')))
		|| path.startsWith(QLatin1String("\\\\")) || path.startsWith(QLatin1String("//"));
}

QString cleanImageSourcePath(QString path)
{
	if (isWindowsImagePath(path))
		path.replace(QLatin1Char('\\'), QLatin1Char('/'));
	return QDir::cleanPath(path);
}
}

QString imageLinkRelativePath(const QString& linkPath, const QString& sourceDirectory)
{
	if (linkPath.isEmpty() || sourceDirectory.isEmpty())
		return QString();
	const bool windowsSource = isWindowsImagePath(sourceDirectory);
	if (windowsSource != isWindowsImagePath(linkPath))
		return QString();
	if (!windowsSource && (!QDir::isAbsolutePath(sourceDirectory) || !QDir::isAbsolutePath(linkPath)))
		return QString();
	const QString link = cleanImageSourcePath(linkPath);
	QString source = cleanImageSourcePath(sourceDirectory);
	if (!source.endsWith(QLatin1Char('/')))
		source += QLatin1Char('/');
	if (!link.startsWith(source, windowsSource ? Qt::CaseInsensitive : Qt::CaseSensitive))
		return QString();
	return link.mid(source.size());
}

QStringList imageLinkSourceFolders(const QStringList& linkPaths)
{
	QStringList folders;
	QSet<QString> seen;
	for (const QString& path : linkPaths)
	{
		if (path.isEmpty())
			continue;
		QDir folder(QFileInfo(path).absolutePath());
		while (!seen.contains(folder.path()))
		{
			const QString folderPath = folder.path();
			seen.insert(folderPath);
			folders.append(folderPath);
			if (folder.isRoot())
				break;
			// filePath avoids creating "//.." at a root. Do not use cdUp(),
			// which can fail for the missing directories we need to recover.
			const QString parentPath = QDir::cleanPath(folder.filePath(QStringLiteral("..")));
			if (parentPath.size() >= folderPath.size())
				break;
			folder.setPath(parentPath);
		}
	}
	return folders;
}

QVector<ImageLinkMatch> findImageLinkMatches(const QStringList& linkPaths,
	const QString& searchDirectory, bool recursive)
{
	ImageLinkSearchTask search(nullptr, linkPaths, searchDirectory, recursive);
	search.start();
	search.runUntilFinished();
	return search.matches();
}

ImageLinkSearchTask::ImageLinkSearchTask(QObject* parent, const QStringList& linkPaths,
	const QString& searchDirectory, bool recursive, const QString& sourceDirectory)
	: DeferredTask(parent),
	  m_linkPaths(linkPaths),
	  m_searchDirectory(searchDirectory),
	  m_sourceDirectory(sourceDirectory),
	  m_recursive(recursive)
{
	for (const QString& linkPath : m_linkPaths)
	{
		const QString requestedPath = m_sourceDirectory.isEmpty() ? QFileInfo(linkPath).fileName()
			: imageLinkRelativePath(linkPath, m_sourceDirectory);
		m_requestedPaths.append(requestedPath);
		if (requestedPath.isEmpty())
			continue;
		m_requestedExactNames.insert(requestedPath);
		m_requestedFoldedNames.insert(requestedPath.toCaseFolded());
	}
}

ImageLinkSearchTask::~ImageLinkSearchTask() = default;

void ImageLinkSearchTask::start()
{
	const QDirIterator::IteratorFlags iteratorFlags = m_recursive
		? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
	m_iterator = std::make_unique<QDirIterator>(m_searchDirectory,
		QDir::Files | QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot, iteratorFlags);
	DeferredTask::start();
}

void ImageLinkSearchTask::next()
{
	// Count directory entries as work too: a files-only iterator can traverse
	// an entire tree of empty folders inside a single hasNext() call.
	constexpr int entriesPerStep = 64;
	int entriesProcessed = 0;
	while (entriesProcessed < entriesPerStep && m_iterator->hasNext())
	{
		const QFileInfo candidate(m_iterator->next());
		++entriesProcessed;
		if (!candidate.isFile())
			continue;
		++m_scannedFileCount;
		const QString candidatePath = m_sourceDirectory.isEmpty() ? candidate.fileName()
			: QDir(m_searchDirectory).relativeFilePath(candidate.absoluteFilePath());
		if (!m_requestedFoldedNames.contains(candidatePath.toCaseFolded()))
			continue;
		const QString absolutePath = candidate.absoluteFilePath();
		if (m_requestedExactNames.contains(candidatePath))
			m_exactNames.insert(candidatePath, absolutePath);
		m_foldedNames.insert(candidatePath.toCaseFolded(), absolutePath);
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
	for (qsizetype i = 0; i < m_linkPaths.size(); ++i)
	{
		ImageLinkMatch result;
		result.linkPath = QDir::cleanPath(QFileInfo(m_linkPaths.at(i)).absoluteFilePath());
		const QString requestedPath = m_requestedPaths.at(i);
		result.candidatePaths = m_exactNames.values(requestedPath);
		if (result.candidatePaths.isEmpty())
			result.candidatePaths = m_foldedNames.values(requestedPath.toCaseFolded());
		result.candidatePaths.removeDuplicates();
		std::sort(result.candidatePaths.begin(), result.candidatePaths.end());
		m_matches.append(result);
	}
}
