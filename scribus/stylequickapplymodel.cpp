/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "stylequickapplymodel.h"

#include <algorithm>
#include <optional>

#include <QRegularExpression>
#include <QStringList>

namespace
{
struct ParsedQuery
{
	QStringList terms;
	std::optional<StyleSearchType> type;
};

ParsedQuery parseQuery(const QString& query)
{
	QString value = query.simplified();
	ParsedQuery parsed;
	const QList<QPair<QString, StyleSearchType>> prefixes = {
		{ QStringLiteral("paragraph:"), StyleSearchType::paragraph },
		{ QStringLiteral("para:"), StyleSearchType::paragraph },
		{ QStringLiteral("p:"), StyleSearchType::paragraph },
		{ QStringLiteral("character:"), StyleSearchType::character },
		{ QStringLiteral("char:"), StyleSearchType::character },
		{ QStringLiteral("c:"), StyleSearchType::character },
		{ QStringLiteral("object:"), StyleSearchType::object },
		{ QStringLiteral("obj:"), StyleSearchType::object },
		{ QStringLiteral("o:"), StyleSearchType::object },
		{ QStringLiteral("table:"), StyleSearchType::table },
		{ QStringLiteral("tb:"), StyleSearchType::table },
		{ QStringLiteral("cell:"), StyleSearchType::cell },
		{ QStringLiteral("cl:"), StyleSearchType::cell },
	};
	for (const auto& prefix : prefixes)
	{
		if (value.startsWith(prefix.first, Qt::CaseInsensitive))
		{
			parsed.type = prefix.second;
			value = value.sliced(prefix.first.length()).trimmed();
			break;
		}
	}
	parsed.terms = value.split(QLatin1Char(' '), Qt::SkipEmptyParts);
	return parsed;
}

int subsequenceScore(const QString& candidate, const QString& term)
{
	int candidateIndex = 0;
	int firstMatch = -1;
	int previousMatch = -1;
	int gapCount = 0;
	for (const QChar character : term)
	{
		const int match = candidate.indexOf(character, candidateIndex, Qt::CaseInsensitive);
		if (match < 0)
			return -1;
		if (firstMatch < 0)
			firstMatch = match;
		if (previousMatch >= 0)
			gapCount += match - previousMatch - 1;
		previousMatch = match;
		candidateIndex = match + 1;
	}
	return 400 + firstMatch * 4 + gapCount * 3 + candidate.length() - term.length();
}

int termScore(const QString& candidate, const QString& term)
{
	if (candidate.compare(term, Qt::CaseInsensitive) == 0)
		return 0;
	if (candidate.startsWith(term, Qt::CaseInsensitive))
		return 100 + candidate.length() - term.length();

	const QStringList words = candidate.split(
		QRegularExpression(QStringLiteral("[\\s_\\-/]+")), Qt::SkipEmptyParts);
	for (const QString& word : words)
	{
		if (word.startsWith(term, Qt::CaseInsensitive))
			return 200 + word.length() - term.length();
	}

	const int substring = candidate.indexOf(term, 0, Qt::CaseInsensitive);
	if (substring >= 0)
		return 300 + substring * 2 + candidate.length() - term.length();
	return subsequenceScore(candidate, term);
}
}

QList<StyleSearchItem> StyleQuickApplyModel::matches(const QList<StyleSearchItem>& styles, const QString& query)
{
	const ParsedQuery parsed = parseQuery(query);
	struct RankedStyle
	{
		StyleSearchItem style;
		int score {0};
	};
	QList<RankedStyle> ranked;
	for (const StyleSearchItem& style : styles)
	{
		if (parsed.type.has_value() && style.type != parsed.type.value())
			continue;

		int score = 0;
		bool matchesAll = true;
		for (const QString& term : parsed.terms)
		{
			const int currentScore = termScore(style.name, term);
			if (currentScore < 0)
			{
				matchesAll = false;
				break;
			}
			score += currentScore;
		}
		if (matchesAll)
			ranked.append({ style, score });
	}

	std::sort(ranked.begin(), ranked.end(), [](const RankedStyle& left, const RankedStyle& right) {
		if (left.score != right.score)
			return left.score < right.score;
		if (left.style.favorite != right.style.favorite)
			return left.style.favorite;
		const bool leftRecent = left.style.recentRank >= 0;
		const bool rightRecent = right.style.recentRank >= 0;
		if (leftRecent != rightRecent)
			return leftRecent;
		if (leftRecent && left.style.recentRank != right.style.recentRank)
			return left.style.recentRank < right.style.recentRank;
		const int nameOrder = QString::compare(left.style.name, right.style.name, Qt::CaseInsensitive);
		if (nameOrder != 0)
			return nameOrder < 0;
		return static_cast<int>(left.style.type) < static_cast<int>(right.style.type);
	});

	QList<StyleSearchItem> result;
	result.reserve(ranked.size());
	for (const RankedStyle& item : ranked)
		result.append(item.style);
	return result;
}
