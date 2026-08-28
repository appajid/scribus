/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "dynamicvariable.h"

#include <QDateTime>
#include <QFileInfo>
#include <QLocale>
#include <QObject>

#include "pageitem.h"
#include "scribusdoc.h"

const QString DynamicVariableResolver::UserDefined = QStringLiteral("user-defined");
const QString DynamicVariableResolver::DocumentTitle = QStringLiteral("document-title");
const QString DynamicVariableResolver::Author = QStringLiteral("author");
const QString DynamicVariableResolver::FileName = QStringLiteral("file-name");
const QString DynamicVariableResolver::PageCount = QStringLiteral("page-count");
const QString DynamicVariableResolver::CurrentPage = QStringLiteral("current-page");
const QString DynamicVariableResolver::CreationDate = QStringLiteral("creation-date");
const QString DynamicVariableResolver::ModificationDate = QStringLiteral("modification-date");

namespace
{
const QString BuiltInPrefix = QStringLiteral("builtin:");

DynamicVariable makeBuiltIn(const QString& type)
{
	DynamicVariable variable;
	variable.id = DynamicVariableResolver::idForType(type);
	variable.type = type;
	variable.name = DynamicVariableResolver::displayNameForType(type);
	return variable;
}

QString formatDateTime(const QDateTime& value)
{
	return value.isValid() ? QLocale().toString(value, QLocale::ShortFormat) : QString();
}
}

QList<DynamicVariable> DynamicVariableResolver::builtInVariables()
{
	return {
		makeBuiltIn(DocumentTitle),
		makeBuiltIn(Author),
		makeBuiltIn(FileName),
		makeBuiltIn(PageCount),
		makeBuiltIn(CurrentPage),
		makeBuiltIn(CreationDate),
		makeBuiltIn(ModificationDate)
	};
}

bool DynamicVariableResolver::isBuiltInId(const QString& id)
{
	return id.startsWith(BuiltInPrefix);
}

QString DynamicVariableResolver::displayNameForType(const QString& type)
{
	if (type == DocumentTitle)
		return QObject::tr("Document Title");
	if (type == Author)
		return QObject::tr("Author");
	if (type == FileName)
		return QObject::tr("Filename");
	if (type == PageCount)
		return QObject::tr("Total Page Count");
	if (type == CurrentPage)
		return QObject::tr("Current Page Number");
	if (type == CreationDate)
		return QObject::tr("Creation Date");
	if (type == ModificationDate)
		return QObject::tr("Modification Date");
	if (type == UserDefined)
		return QObject::tr("User Defined");
	return QObject::tr("Unknown");
}

QString DynamicVariableResolver::idForType(const QString& type)
{
	return BuiltInPrefix + type;
}

QString DynamicVariableResolver::typeForId(const QString& id)
{
	return isBuiltInId(id) ? id.mid(BuiltInPrefix.length()) : UserDefined;
}

QString DynamicVariableResolver::resolve(const ScribusDoc* doc, const QString& variableId, const PageItem* frame)
{
	if (!doc || variableId.isEmpty())
		return QString();

	if (!isBuiltInId(variableId))
	{
		const DynamicVariable* variable = doc->dynamicVariable(variableId);
		return variable ? variable->value : QString();
	}

	const QString type = typeForId(variableId);
	if (type == DocumentTitle)
		return doc->documentInfo().title();
	if (type == Author)
		return doc->documentInfo().author();
	if (type == FileName)
		return QFileInfo(doc->documentFileName()).fileName();
	if (type == PageCount)
		return QString::number(doc->DocPages.count());
	if (type == CurrentPage)
	{
		if (!frame || frame->OwnPage < 0)
			return QStringLiteral("#");
		int pageNumber = frame->OwnPage;
		// Match the existing page-number expansion's safe preview for a
		// master-page object that is not currently attached to a document page.
		if (!frame->OnMasterPage.isEmpty() && pageNumber >= doc->DocPages.count())
			pageNumber = 0;
		if (pageNumber >= doc->DocPages.count())
			return QStringLiteral("#");
		return doc->getSectionPageNumberForPageIndex(pageNumber);
	}
	if (type == CreationDate)
		return formatDateTime(doc->dynamicVariableCreationDate());
	if (type == ModificationDate)
	{
		const QFileInfo fileInfo(doc->documentFileName());
		if (doc->hasName && fileInfo.exists())
			return formatDateTime(fileInfo.lastModified());
		return formatDateTime(doc->dynamicVariableCreationDate());
	}
	return QString();
}
