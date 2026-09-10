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
#include <QRegularExpression>
#include <QVector>

#include <algorithm>

#include "pageitem.h"
#include "pageitemiterator.h"
#include "marks.h"
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
const QString DynamicVariableResolver::FirstOnSpreadMode = QStringLiteral("first-on-spread");
const QString DynamicVariableResolver::LastOnSpreadMode = QStringLiteral("last-on-spread");
const QString DynamicVariableResolver::MostRecentMode = QStringLiteral("most-recent");
const QString DynamicVariableResolver::AsEnteredCase = QStringLiteral("as-entered");
const QString DynamicVariableResolver::UppercaseCase = QStringLiteral("uppercase");
const QString DynamicVariableResolver::LowercaseCase = QStringLiteral("lowercase");
const QString DynamicVariableResolver::TitleCaseCase = QStringLiteral("title-case");
const QString DynamicVariableResolver::NoFallback = QStringLiteral("none");
const QString DynamicVariableResolver::SectionFallback = QStringLiteral("section");
const QString DynamicVariableResolver::DocumentFallback = QStringLiteral("document");

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

QString titleCase(const QString& text)
{
	static const QRegularExpression wordPattern(QStringLiteral("(\\p{L})([\\p{L}\\p{M}'\\x{2019}]*)"));
	QString result;
	result.reserve(text.size());
	qsizetype previousEnd = 0;
	auto matches = wordPattern.globalMatch(text);
	while (matches.hasNext())
	{
		const QRegularExpressionMatch match = matches.next();
		result += text.mid(previousEnd, match.capturedStart() - previousEnd);
		result += match.captured(1).toUpper();
		result += match.captured(2).toLower();
		previousEnd = match.capturedEnd();
	}
	result += text.mid(previousEnd);
	return result;
}

QString formatRunningHeaderText(const QString& text, const DynamicVariable& variable,
	DynamicVariable::RunningHeaderTextCase textCase)
{
	QString result = text;
	if (variable.removeTrailingPunctuation)
	{
		static const QRegularExpression trailingPunctuation(QStringLiteral("[\\p{P}\\s]+$"));
		result.remove(trailingPunctuation);
	}

	switch (textCase)
	{
	case DynamicVariable::RunningHeaderTextCase::AsEntered:
		break;
	case DynamicVariable::RunningHeaderTextCase::Uppercase:
		result = result.toUpper();
		break;
	case DynamicVariable::RunningHeaderTextCase::Lowercase:
		result = result.toLower();
		break;
	case DynamicVariable::RunningHeaderTextCase::TitleCase:
		result = titleCase(result);
		break;
	case DynamicVariable::RunningHeaderTextCase::Unsupported:
		return QString();
	}
	return result;
}

bool containsRunningHeaderVariable(const StoryText& story, int paragraphStart, int paragraphEnd,
	const QString& variableId)
{
	for (int position = paragraphStart; position < paragraphEnd; ++position)
	{
		if (!story.hasMark(position))
			continue;
		const Mark* mark = story.mark(position);
		if (mark && mark->isType(MARKVariableTextType) && mark->getVariableId() == variableId)
			return true;
	}
	return false;
}

QVector<RunningHeaderCandidate> runningHeaderCandidates(const ScribusDoc* doc, const DynamicVariable& variable,
	const PageItem* contextFrame, int firstCandidatePage, int lastCandidatePage)
{
	QVector<RunningHeaderCandidate> candidates;
	if (firstCandidatePage > lastCandidatePage)
		return candidates;
	int itemOrder = 0;
	for (PageItemIterator it(doc->DocItems, PageItemIterator::IterateInGroups); *it; ++it, ++itemOrder)
	{
		PageItem* item = *it;
		if (!item || item == contextFrame || !item->isTextFrame()
			|| item->OwnPage < firstCandidatePage || item->OwnPage > lastCandidatePage)
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
				&& appliedParagraphStyleName(item->itemText.paragraphStyle(paragraphStart)) == variable.paragraphStyle
				&& !containsRunningHeaderVariable(item->itemText, paragraphStart, paragraphEnd, variable.id))
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

	return candidates;
}

QString resolveRunningHeader(const ScribusDoc* doc, const DynamicVariable& variable,
	DynamicVariable::RunningHeaderMode mode, DynamicVariable::RunningHeaderFallback fallback,
	const PageItem* contextFrame)
{
	if (!contextFrame || contextFrame->OwnPage < 0 || contextFrame->OwnPage >= doc->DocPages.count())
		return QString();

	int firstCandidatePage = contextFrame->OwnPage;
	int lastCandidatePage = contextFrame->OwnPage;
	if (mode == DynamicVariable::RunningHeaderMode::MostRecent)
		firstCandidatePage = 0;
	else if (mode == DynamicVariable::RunningHeaderMode::FirstOnSpread
		|| mode == DynamicVariable::RunningHeaderMode::LastOnSpread)
	{
		const int columns = qMax(1, doc->pageSets()[doc->pagePositioning()].Columns);
		const int spreadStart = contextFrame->OwnPage - doc->columnOfPage(contextFrame->OwnPage);
		firstCandidatePage = qMax(0, spreadStart);
		lastCandidatePage = qMin(doc->DocPages.count() - 1, spreadStart + columns - 1);
	}

	int sectionStart = contextFrame->OwnPage;
	int sectionEnd = contextFrame->OwnPage;
	if (fallback == DynamicVariable::RunningHeaderFallback::Section)
	{
		const int sectionKey = doc->getSectionKeyForPageIndex(contextFrame->OwnPage);
		const auto sectionIt = doc->sections().constFind(sectionKey);
		if (sectionKey < 0 || sectionIt == doc->sections().constEnd())
			return QString();
		sectionStart = static_cast<int>(sectionIt->fromindex);
		sectionEnd = static_cast<int>(sectionIt->toindex);
		firstCandidatePage = qMax(firstCandidatePage, sectionStart);
		lastCandidatePage = qMin(lastCandidatePage, sectionEnd);
	}

	const QVector<RunningHeaderCandidate> candidates = runningHeaderCandidates(doc, variable, contextFrame,
		firstCandidatePage, lastCandidatePage);
	if (!candidates.isEmpty())
	{
		return mode == DynamicVariable::RunningHeaderMode::FirstOnPage
			|| mode == DynamicVariable::RunningHeaderMode::FirstOnSpread
			? candidates.constFirst().text
			: candidates.constLast().text;
	}

	if (mode == DynamicVariable::RunningHeaderMode::MostRecent
		|| fallback == DynamicVariable::RunningHeaderFallback::NoFallback)
		return QString();

	const int fallbackFirstPage = fallback == DynamicVariable::RunningHeaderFallback::Section ? sectionStart : 0;
	const int fallbackLastPage = firstCandidatePage - 1;
	const QVector<RunningHeaderCandidate> fallbackCandidates = runningHeaderCandidates(doc, variable, contextFrame,
		fallbackFirstPage, fallbackLastPage);
	return fallbackCandidates.isEmpty() ? QString() : fallbackCandidates.constLast().text;
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

bool DynamicVariableResolver::isKnownBuiltInId(const QString& id)
{
	for (const DynamicVariable& variable : builtInVariables())
	{
		if (variable.id == id)
			return true;
	}
	return false;
}

bool DynamicVariableResolver::isReservedName(const QString& name)
{
	const QString candidate = name.trimmed();
	if (candidate.isEmpty() || isBuiltInId(candidate))
		return true;
	for (const DynamicVariable& variable : builtInVariables())
	{
		// Only stable, non-translated identifiers belong to the reserved
		// namespace. Reserving display labels would make an SLA valid or invalid
		// depending on the UI language used to open it.
		if (variable.type == candidate)
			return true;
	}
	return false;
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
	if (mode == FirstOnSpreadMode)
		return DynamicVariable::RunningHeaderMode::FirstOnSpread;
	if (mode == LastOnSpreadMode)
		return DynamicVariable::RunningHeaderMode::LastOnSpread;
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
	case DynamicVariable::RunningHeaderMode::FirstOnSpread:
		return FirstOnSpreadMode;
	case DynamicVariable::RunningHeaderMode::LastOnSpread:
		return LastOnSpreadMode;
	case DynamicVariable::RunningHeaderMode::MostRecent:
		return MostRecentMode;
	case DynamicVariable::RunningHeaderMode::Unsupported:
		break;
	}
	return QString();
}

DynamicVariable::RunningHeaderTextCase DynamicVariableResolver::runningHeaderTextCaseFromString(const QString& textCase)
{
	if (textCase == AsEnteredCase)
		return DynamicVariable::RunningHeaderTextCase::AsEntered;
	if (textCase == UppercaseCase)
		return DynamicVariable::RunningHeaderTextCase::Uppercase;
	if (textCase == LowercaseCase)
		return DynamicVariable::RunningHeaderTextCase::Lowercase;
	if (textCase == TitleCaseCase)
		return DynamicVariable::RunningHeaderTextCase::TitleCase;
	return DynamicVariable::RunningHeaderTextCase::Unsupported;
}

QString DynamicVariableResolver::runningHeaderTextCaseToString(DynamicVariable::RunningHeaderTextCase textCase)
{
	switch (textCase)
	{
	case DynamicVariable::RunningHeaderTextCase::AsEntered:
		return AsEnteredCase;
	case DynamicVariable::RunningHeaderTextCase::Uppercase:
		return UppercaseCase;
	case DynamicVariable::RunningHeaderTextCase::Lowercase:
		return LowercaseCase;
	case DynamicVariable::RunningHeaderTextCase::TitleCase:
		return TitleCaseCase;
	case DynamicVariable::RunningHeaderTextCase::Unsupported:
		break;
	}
	return QString();
}

DynamicVariable::RunningHeaderFallback DynamicVariableResolver::runningHeaderFallbackFromString(const QString& fallback)
{
	if (fallback.isEmpty() || fallback == NoFallback)
		return DynamicVariable::RunningHeaderFallback::NoFallback;
	if (fallback == SectionFallback)
		return DynamicVariable::RunningHeaderFallback::Section;
	if (fallback == DocumentFallback)
		return DynamicVariable::RunningHeaderFallback::Document;
	return DynamicVariable::RunningHeaderFallback::Unsupported;
}

QString DynamicVariableResolver::runningHeaderFallbackToString(DynamicVariable::RunningHeaderFallback fallback)
{
	switch (fallback)
	{
	case DynamicVariable::RunningHeaderFallback::NoFallback:
		return NoFallback;
	case DynamicVariable::RunningHeaderFallback::Section:
		return SectionFallback;
	case DynamicVariable::RunningHeaderFallback::Document:
		return DocumentFallback;
	case DynamicVariable::RunningHeaderFallback::Unsupported:
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
			const DynamicVariable::RunningHeaderTextCase textCase = runningHeaderTextCaseFromString(variable->runningHeaderTextCase);
			const DynamicVariable::RunningHeaderFallback fallback = runningHeaderFallbackFromString(variable->runningHeaderFallback);
			if (variable->paragraphStyle.isEmpty()
				|| !doc->paragraphStyles().contains(variable->paragraphStyle)
				|| mode == DynamicVariable::RunningHeaderMode::Unsupported
				|| textCase == DynamicVariable::RunningHeaderTextCase::Unsupported
				|| fallback == DynamicVariable::RunningHeaderFallback::Unsupported
				|| (mode == DynamicVariable::RunningHeaderMode::MostRecent
					&& fallback != DynamicVariable::RunningHeaderFallback::NoFallback))
				return QString();
			if (!frame || frame->OwnPage < 0 || frame->OwnPage >= doc->DocPages.count())
				return QString();
			QString cachedValue;
			if (doc->runningHeaderCacheValue(variable->id, frame->OwnPage, frame, cachedValue))
				return cachedValue;
			if (!doc->beginRunningHeaderResolution(variable->id, frame->OwnPage))
				return QString();
			const QString value = formatRunningHeaderText(resolveRunningHeader(doc, *variable, mode, fallback, frame), *variable, textCase);
			doc->endRunningHeaderResolution(variable->id, frame->OwnPage);
			doc->setRunningHeaderCacheValue(variable->id, frame->OwnPage, frame, value);
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
