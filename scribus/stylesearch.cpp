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
		auto style = scribusDoc->paragraphStyles()[i];
		styles.append({style.name(), StyleSearchType::paragraph});
	}
	n = scribusDoc->charStyles().count();
	for (int i = 0; i < n; ++i )
	{
		auto style = scribusDoc->charStyles()[i];
		styles.append({style.name(), StyleSearchType::character});
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
