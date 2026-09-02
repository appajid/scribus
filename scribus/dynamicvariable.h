/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#ifndef DYNAMICVARIABLE_H
#define DYNAMICVARIABLE_H

#include <QList>
#include <QString>

#include "scribusapi.h"

class PageItem;
class ScribusDoc;

struct SCRIBUS_API DynamicVariable
{
	enum class RunningHeaderMode
	{
		FirstOnPage,
		LastOnPage,
		MostRecent,
		Unsupported
	};

	QString id;
	QString type;
	QString name;
	QString value;
	QString paragraphStyle;
	QString runningHeaderMode;
};

class SCRIBUS_API DynamicVariableResolver
{
public:
	static const QString UserDefined;
	static const QString DocumentTitle;
	static const QString Author;
	static const QString FileName;
	static const QString PageCount;
	static const QString CurrentPage;
	static const QString CreationDate;
	static const QString ModificationDate;
	static const QString RunningHeader;
	static const QString FirstOnPageMode;
	static const QString LastOnPageMode;
	static const QString MostRecentMode;

	static QList<DynamicVariable> builtInVariables();
	static bool isBuiltInId(const QString& id);
	static QString displayNameForType(const QString& type);
	static QString idForType(const QString& type);
	static QString typeForId(const QString& id);
	static DynamicVariable::RunningHeaderMode runningHeaderModeFromString(const QString& mode);
	static QString runningHeaderModeToString(DynamicVariable::RunningHeaderMode mode);
	static QString resolve(const ScribusDoc* doc, const QString& variableId, const PageItem* frame = nullptr);
};

#endif
