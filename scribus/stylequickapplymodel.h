/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STYLEQUICKAPPLYMODEL_H
#define STYLEQUICKAPPLYMODEL_H

#include <QList>
#include <QString>

#include "scribusapi.h"

enum class StyleSearchType { paragraph, character, table, cell };

struct StyleSearchItem
{
	QString name;
	StyleSearchType type {StyleSearchType::paragraph};
	QString fontFamily;
	QString fontStyle;
	double fontSize {0.0};
	QString parentStyle;
	int paragraphAlignment {-1};
	bool favorite {false};
	int recentRank {-1};
};

class SCRIBUS_API StyleQuickApplyModel
{
public:
	static QList<StyleSearchItem> matches(const QList<StyleSearchItem>& styles, const QString& query);
};

#endif
