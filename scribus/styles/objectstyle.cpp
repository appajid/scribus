/*
 For general Scribus (>=1.3.2) copyright and licensing information please refer
 to the COPYING file provided with the program. Following this notice may exist
 a copyright and/or license notice that predates the release of Scribus 1.3.2
 for which a new license (GPL+exception) is in place.
 */

#include "objectstyle.h"

#include <QObject>
#include <QtGlobal>

namespace
{
template<typename T>
bool equivalent(const T& left, const T& right)
{
	return left == right;
}

template<>
bool equivalent<double>(const double& left, const double& right)
{
	return qAbs(left - right) <= 0.000001;
}
}

QString ObjectStyle::displayName() const
{
	if (isDefaultStyle())
		return QObject::tr("Default Object Style", "object style");
	if (hasName() || !hasParent() || !m_context)
		return name();
	return parentStyle()->displayName() + QLatin1Char('+');
}

bool ObjectStyle::equiv(const BaseStyle& other) const
{
	other.validate();
	const auto* otherStyle = dynamic_cast<const ObjectStyle*>(&other);
	return otherStyle
		&& parent() == otherStyle->parent()
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
		&& inh_##attr_NAME == otherStyle->inh_##attr_NAME \
		&& (inh_##attr_NAME || equivalent(m_##attr_NAME, otherStyle->m_##attr_NAME))
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF
		;
}

void ObjectStyle::erase()
{
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	if (!inh_##attr_NAME) \
		reset##attr_NAME();
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF
}

void ObjectStyle::update(const StyleContext* context)
{
	BaseStyle::update(context);
	const auto* parentObjectStyle = dynamic_cast<const ObjectStyle*>(parentStyle());
	if (!parentObjectStyle)
		return;
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	if (inh_##attr_NAME) \
		m_##attr_NAME = parentObjectStyle->attr_GETTER();
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF
}

void ObjectStyle::getNamedResources(ResourceCollection& lists) const
{
	if (!parent().isEmpty())
		lists.collectObjectStyle(parent());
	for (const BaseStyle* style = parentStyle(); style; style = style->parentStyle())
		lists.collectObjectStyle(style->name());
	lists.collectColor(fillColor());
	lists.collectColor(lineColor());
	lists.collectLineStyle(customLineStyle());
}

void ObjectStyle::replaceNamedResources(ResourceCollection& newNames)
{
	auto replacement = newNames.objectStyles().constFind(parent());
	if (!parent().isEmpty() && replacement != newNames.objectStyles().constEnd())
		setParent(replacement.value());

	replacement = newNames.colors().constFind(fillColor());
	if (!isInhFillColor() && replacement != newNames.colors().constEnd())
		setFillColor(replacement.value());
	replacement = newNames.colors().constFind(lineColor());
	if (!isInhLineColor() && replacement != newNames.colors().constEnd())
		setLineColor(replacement.value());
	replacement = newNames.lineStyles().constFind(customLineStyle());
	if (!isInhCustomLineStyle() && replacement != newNames.lineStyles().constEnd())
		setCustomLineStyle(replacement.value());
}
