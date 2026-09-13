/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef IMAGELINKMATCHER_H
#define IMAGELINKMATCHER_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "scribusapi.h"

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

#endif
