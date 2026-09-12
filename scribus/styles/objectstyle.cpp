/*
 For general Scribus (>=1.3.2) copyright and licensing information please refer
 to the COPYING file provided with the program. Following this notice may exist
 a copyright and/or license notice that predates the release of Scribus 1.3.2
 for which a new license (GPL+exception) is in place.
 */

#include "objectstyle.h"

#include <functional>
#include <utility>

#include <QObject>
#include <QSet>
#include <QtGlobal>

#include "styles/styleset.h"

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

ObjectStyleImportPlan buildObjectStyleImportPlan(const StyleSet<ObjectStyle>& sourceStyles,
														 const QStringList& selectedStyleNames)
{
	ObjectStyleImportPlan plan;
	QSet<QString> visiting;
	QSet<QString> included;

	std::function<void(const QString&)> appendWithParents = [&](const QString& styleName)
	{
		if (styleName.isEmpty() || included.contains(styleName) || visiting.contains(styleName))
			return;
		const ObjectStyle* style = sourceStyles.getPointer(styleName);
		if (!style)
			return;

		visiting.insert(styleName);
		appendWithParents(style->parent());
		visiting.remove(styleName);
		if (!included.contains(styleName))
		{
			included.insert(styleName);
			plan.styleNames.append(styleName);
		}
	};

	for (const QString& styleName : selectedStyleNames)
		appendWithParents(styleName);

	QSet<QString> colors;
	QSet<QString> lineStyles;
	for (const QString& styleName : std::as_const(plan.styleNames))
	{
		const ObjectStyle* style = sourceStyles.getPointer(styleName);
		if (!style)
			continue;
		ObjectStyle localStyle(*style);
		localStyle.setContext(nullptr);
		if (!localStyle.isInhFillColor() && !localStyle.fillColor().isEmpty())
			colors.insert(localStyle.fillColor());
		if (!localStyle.isInhLineColor() && !localStyle.lineColor().isEmpty())
			colors.insert(localStyle.lineColor());
		if (!localStyle.isInhCustomLineStyle() && !localStyle.customLineStyle().isEmpty())
			lineStyles.insert(localStyle.customLineStyle());
	}
	plan.colorNames = colors.values();
	plan.lineStyleNames = lineStyles.values();
	plan.colorNames.sort();
	plan.lineStyleNames.sort();
	return plan;
}

QMap<QString, QString> importObjectStyles(const StyleSet<ObjectStyle>& sourceStyles,
													 const QStringList& styleNames,
													 StyleSet<ObjectStyle>& destinationStyles,
													 bool renameOnClash,
													 const QMap<QString, QString>& lineStyleNames)
{
	QMap<QString, QString> importedNames;
	ResourceCollection replacements;
	for (auto it = lineStyleNames.constBegin(); it != lineStyleNames.constEnd(); ++it)
		replacements.mapLineStyle(it.key(), it.value());

	for (const QString& sourceName : styleNames)
	{
		const ObjectStyle* sourceStyle = sourceStyles.getPointer(sourceName);
		if (!sourceStyle)
			continue;

		QString destinationName(sourceName);
		if (renameOnClash && destinationStyles.contains(destinationName))
			destinationName = destinationStyles.getUniqueCopyName(destinationName);
		importedNames.insert(sourceName, destinationName);
		replacements.mapObjectStyle(sourceName, destinationName);

		ObjectStyle importedStyle(*sourceStyle);
		importedStyle.setContext(nullptr);
		importedStyle.setDefaultStyle(false);
		importedStyle.setName(destinationName);
		if (!importedStyle.parent().isEmpty() && !importedNames.contains(importedStyle.parent())
			&& !destinationStyles.contains(importedStyle.parent()))
			importedStyle.setParent(QString());
		importedStyle.replaceNamedResources(replacements);

		const int destinationIndex = destinationStyles.find(destinationName);
		if (destinationIndex >= 0)
			destinationStyles[destinationIndex] = importedStyle;
		else
			destinationStyles.create(importedStyle);
	}
	destinationStyles.invalidate();
	return importedNames;
}
