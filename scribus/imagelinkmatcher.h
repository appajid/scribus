/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef IMAGELINKMATCHER_H
#define IMAGELINKMATCHER_H

#include <memory>

#include <QMultiHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include "deferredtask.h"
#include "scribusapi.h"

class QDirIterator;

struct SCRIBUS_API ImageLinkMatch
{
	QString linkPath;
	QStringList candidatePaths;

	bool isUnique() const { return candidatePaths.size() == 1; }
	bool isAmbiguous() const { return candidatePaths.size() > 1; }
};

/**
 * Find files whose names match the file names in linkPaths.
 *
 * Exact-case names are preferred. A case-insensitive fallback is used only
 * when no exact-case match exists, making documents moved between filesystems
 * with different case rules recoverable without hiding real ambiguities.
 */
SCRIBUS_API QVector<ImageLinkMatch> findImageLinkMatches(const QStringList& linkPaths,
	const QString& searchDirectory, bool recursive = true);

/**
 * Incrementally find replacement candidates without blocking the user interface.
 *
 * The task examines a bounded number of files on every event-loop iteration. It
 * can therefore be connected to DeferredTask::finished and DeferredTask::aborted,
 * and cancelled through DeferredTask::cancel.
 */
class SCRIBUS_API ImageLinkSearchTask : public DeferredTask
{
public:
	ImageLinkSearchTask(QObject* parent, const QStringList& linkPaths,
		const QString& searchDirectory, bool recursive = true);
	~ImageLinkSearchTask() override;

	void start() override;
	const QVector<ImageLinkMatch>& matches() const { return m_matches; }
	qsizetype scannedFileCount() const { return m_scannedFileCount; }

protected slots:
	void next() override;

private:
	void compileResults();

	QStringList m_linkPaths;
	QString m_searchDirectory;
	bool m_recursive {true};
	std::unique_ptr<QDirIterator> m_iterator;
	QSet<QString> m_requestedExactNames;
	QSet<QString> m_requestedFoldedNames;
	QMultiHash<QString, QString> m_exactNames;
	QMultiHash<QString, QString> m_foldedNames;
	QVector<ImageLinkMatch> m_matches;
	qsizetype m_scannedFileCount {0};
};

#endif
