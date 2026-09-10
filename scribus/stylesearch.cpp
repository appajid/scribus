/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "stylesearch.h"

#include <QList>

#include "scribusdoc.h"
#include "selection.h"

StyleSearch::StyleSearch(ScribusDoc *scribusDoc)
	: scribusDoc{scribusDoc}
{
}

void StyleSearch::update()
{
	styles.clear();
	if (!scribusDoc || !scribusDoc->m_Selection || scribusDoc->m_Selection->isEmpty())
		return;

	int n = scribusDoc->paragraphStyles().count();
	for (int i = 0; i < n; ++i )
	{
		const ParagraphStyle& style = scribusDoc->paragraphStyles()[i];
		const CharStyle& charStyle = style.charStyle();
		StyleSearchItem item;
		item.name = style.name();
		item.type = StyleSearchType::paragraph;
		item.fontFamily = charStyle.font().family();
		item.fontStyle = charStyle.font().style();
		item.fontSize = charStyle.fontSize() / 10.0;
		item.parentStyle = style.parent();
		item.paragraphAlignment = static_cast<int>(style.alignment());
		styles.append(item);
	}
	n = scribusDoc->charStyles().count();
	for (int i = 0; i < n; ++i )
	{
		const CharStyle& style = scribusDoc->charStyles()[i];
		StyleSearchItem item;
		item.name = style.name();
		item.type = StyleSearchType::character;
		item.fontFamily = style.font().family();
		item.fontStyle = style.font().style();
		item.fontSize = style.fontSize() / 10.0;
		item.parentStyle = style.parent();
		styles.append(item);
	}
}

/**
 * The implementation execute is based on the scripter's scribus_setparagraphstyle
 * and scribus_setcharstyle.
 */
void StyleSearch::execute(const StyleSearchItem& style)
{
	if (!scribusDoc || !scribusDoc->m_Selection || scribusDoc->m_Selection->isEmpty())
		return;

	if (style.type == StyleSearchType::paragraph)
	{
		if (!scribusDoc->paragraphStyles().contains(style.name))
			return;

		ParagraphStyle paragraphStyle;
		paragraphStyle.setParent(style.name);
		scribusDoc->itemSelection_ApplyParagraphStyle(paragraphStyle);
	}
	else if (style.type == StyleSearchType::character)
	{
		if (!scribusDoc->charStyles().contains(style.name))
			return;

		scribusDoc->itemSelection_SetNamedCharStyle(style.name);
	}
}
