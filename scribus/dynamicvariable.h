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
		FirstOnSpread,
		LastOnSpread,
		MostRecent,
		Unsupported
	};

	enum class RunningHeaderTextCase
	{
		AsEntered,
		Uppercase,
		Lowercase,
		TitleCase,
		Unsupported
	};

	enum class RunningHeaderFallback
	{
		NoFallback,
		Section,
		Document,
		Unsupported
	};

	QString id;
	QString type;
	QString name;
	QString value;
	QString paragraphStyle;
	QString runningHeaderMode;
	QString runningHeaderTextCase;
	QString runningHeaderFallback;
	bool removeTrailingPunctuation { false };
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
	static const QString FirstOnSpreadMode;
	static const QString LastOnSpreadMode;
	static const QString MostRecentMode;
	static const QString AsEnteredCase;
	static const QString UppercaseCase;
	static const QString LowercaseCase;
	static const QString TitleCaseCase;
	static const QString NoFallback;
	static const QString SectionFallback;
	static const QString DocumentFallback;

	static QList<DynamicVariable> builtInVariables();
	static bool isBuiltInId(const QString& id);
	static bool isKnownBuiltInId(const QString& id);
	static bool isReservedName(const QString& name);
	static QString displayNameForType(const QString& type);
	static QString idForType(const QString& type);
	static QString typeForId(const QString& id);
	static DynamicVariable::RunningHeaderMode runningHeaderModeFromString(const QString& mode);
	static QString runningHeaderModeToString(DynamicVariable::RunningHeaderMode mode);
	static DynamicVariable::RunningHeaderTextCase runningHeaderTextCaseFromString(const QString& textCase);
	static QString runningHeaderTextCaseToString(DynamicVariable::RunningHeaderTextCase textCase);
	static DynamicVariable::RunningHeaderFallback runningHeaderFallbackFromString(const QString& fallback);
	static QString runningHeaderFallbackToString(DynamicVariable::RunningHeaderFallback fallback);
	static QString resolve(const ScribusDoc* doc, const QString& variableId, const PageItem* frame = nullptr);
};

#endif
