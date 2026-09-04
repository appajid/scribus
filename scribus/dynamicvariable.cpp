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
#include <QPointF>
#include <QVector>

#include <algorithm>

#include "pageitem.h"
#include "pageitemiterator.h"
#include "scribusdoc.h"
#include "styles/paragraphstyle.h"
#include "text/specialchars.h"
#include "text/storytext.h"

const QString DynamicVariableResolver::UserDefined = QStringLiteral("user-defined");
const QString DynamicVariableResolver::DocumentTitle = QStringLiteral("document-title");
const QString DynamicVariableResolver::Author = QStringLiteral("author");
const QString DynamicVariableResolver::FileName = QStringLiteral("file-name");
const QString DynamicVariableResolver::PageCount = QStringLiteral("page-count");
const QString DynamicVariableResolver::CurrentPage = QStringLiteral("current-page");
const QString DynamicVariableResolver::CreationDate = QStringLiteral("creation-date");
const QString DynamicVariableResolver::ModificationDate = QStringLiteral("modification-date");
const QString DynamicVariableResolver::RunningHeader = QStringLiteral("running-header");
const QString DynamicVariableResolver::FirstOnPageMode = QStringLiteral("first-on-page");
const QString DynamicVariableResolver::LastOnPageMode = QStringLiteral("last-on-page");
const QString DynamicVariableResolver::MostRecentMode = QStringLiteral("most-recent");

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

struct RunningHeaderCandidate
{
	QString text;
	QPointF position;
	int page { -1 };
	int itemOrder { 0 };
	int storyPosition { 0 };
};

QString appliedParagraphStyleName(const ParagraphStyle& style)
{
	if (style.hasParent())
	{
		const BaseStyle* parent = style.parentStyle();
		if (parent)
			return parent->name();
	}
	return style.name();
}

QString runningHeaderText(const StoryText& story, int paragraphStart, int paragraphEnd)
{
	QString text = story.text(paragraphStart, paragraphEnd - paragraphStart);
	text.remove(SpecialChars::COLBREAK);
	text.remove(SpecialChars::FRAMEBREAK);
	text.replace(SpecialChars::LINEBREAK, QLatin1Char(' '));
	return text.trimmed();
}

QString resolveRunningHeader(const ScribusDoc* doc, const DynamicVariable& variable,
	DynamicVariable::RunningHeaderMode mode, const PageItem* contextFrame)
{
	if (!contextFrame || contextFrame->OwnPage < 0 || contextFrame->OwnPage >= doc->DocPages.count())
		return QString();

	QVector<RunningHeaderCandidate> candidates;
	int itemOrder = 0;
	for (PageItemIterator it(doc->DocItems, PageItemIterator::IterateInGroups); *it; ++it, ++itemOrder)
	{
		PageItem* item = *it;
		if (!item || item == contextFrame || !item->isTextFrame() || item->OwnPage < 0
			|| item->OwnPage > contextFrame->OwnPage
			|| (mode != DynamicVariable::RunningHeaderMode::MostRecent && item->OwnPage != contextFrame->OwnPage))
			continue;
		if (item->itemText.isEmpty())
			continue;

		if (item->invalid)
			item->layout();
		const int first = item->firstInFrame();
		const int last = item->lastInFrame();
		if (first < 0 || last < first)
			continue;

		int position = first;
		while (position <= last && position < item->itemText.length())
		{
			const uint paragraph = item->itemText.nrOfParagraph(position);
			const int paragraphStart = item->itemText.startOfParagraph(paragraph);
			const int paragraphEnd = item->itemText.endOfParagraph(paragraph);

			// A paragraph which began in an earlier linked frame belongs to that
			// earlier page. This prevents a long heading from becoming a second
			// running-header candidate merely because it continues here.
			if (paragraphStart >= first && paragraphStart <= last
				&& appliedParagraphStyleName(item->itemText.paragraphStyle(paragraphStart)) == variable.paragraphStyle)
			{
				const QString text = runningHeaderText(item->itemText, paragraphStart, paragraphEnd);
				if (!text.isEmpty())
				{
					RunningHeaderCandidate candidate;
					candidate.text = text;
					candidate.position = QPointF(item->visualXPos(), item->visualYPos());
					candidate.page = item->OwnPage;
					candidate.itemOrder = itemOrder;
					candidate.storyPosition = paragraphStart;
					candidates.append(candidate);
				}
			}

			const int nextPosition = item->itemText.startOfNextParagraph(position);
			if (nextPosition <= position)
				break;
			position = nextPosition;
		}
	}

	if (candidates.isEmpty())
		return QString();

	std::stable_sort(candidates.begin(), candidates.end(), [](const RunningHeaderCandidate& left,
		const RunningHeaderCandidate& right) {
		if (left.page != right.page)
			return left.page < right.page;
		if (!qFuzzyCompare(left.position.y() + 1.0, right.position.y() + 1.0))
			return left.position.y() < right.position.y();
		if (!qFuzzyCompare(left.position.x() + 1.0, right.position.x() + 1.0))
			return left.position.x() < right.position.x();
		if (left.itemOrder != right.itemOrder)
			return left.itemOrder < right.itemOrder;
		return left.storyPosition < right.storyPosition;
	});

	return mode == DynamicVariable::RunningHeaderMode::FirstOnPage
		? candidates.constFirst().text
		: candidates.constLast().text;
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
	if (type == RunningHeader)
		return QObject::tr("Running Header");
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

DynamicVariable::RunningHeaderMode DynamicVariableResolver::runningHeaderModeFromString(const QString& mode)
{
	if (mode == FirstOnPageMode)
		return DynamicVariable::RunningHeaderMode::FirstOnPage;
	if (mode == LastOnPageMode)
		return DynamicVariable::RunningHeaderMode::LastOnPage;
	if (mode == MostRecentMode)
		return DynamicVariable::RunningHeaderMode::MostRecent;
	return DynamicVariable::RunningHeaderMode::Unsupported;
}

QString DynamicVariableResolver::runningHeaderModeToString(DynamicVariable::RunningHeaderMode mode)
{
	switch (mode)
	{
	case DynamicVariable::RunningHeaderMode::FirstOnPage:
		return FirstOnPageMode;
	case DynamicVariable::RunningHeaderMode::LastOnPage:
		return LastOnPageMode;
	case DynamicVariable::RunningHeaderMode::MostRecent:
		return MostRecentMode;
	case DynamicVariable::RunningHeaderMode::Unsupported:
		break;
	}
	return QString();
}

QString DynamicVariableResolver::resolve(const ScribusDoc* doc, const QString& variableId, const PageItem* frame)
{
	if (!doc || variableId.isEmpty())
		return QString();

	if (!isBuiltInId(variableId))
	{
		const DynamicVariable* variable = doc->dynamicVariable(variableId);
		if (!variable)
			return QString();
		if (variable->type == RunningHeader)
		{
			const DynamicVariable::RunningHeaderMode mode = runningHeaderModeFromString(variable->runningHeaderMode);
			if (variable->paragraphStyle.isEmpty()
				|| !doc->paragraphStyles().contains(variable->paragraphStyle)
				|| mode == DynamicVariable::RunningHeaderMode::Unsupported)
				return QString();
			if (!frame || frame->OwnPage < 0 || frame->OwnPage >= doc->DocPages.count())
				return QString();
			QString cachedValue;
			if (doc->runningHeaderCacheValue(variable->id, frame->OwnPage, cachedValue))
				return cachedValue;
			const QString value = resolveRunningHeader(doc, *variable, mode, frame);
			doc->setRunningHeaderCacheValue(variable->id, frame->OwnPage, value);
			return value;
		}
		return variable->value;
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
