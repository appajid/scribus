/*
 For general Scribus (>=1.3.2) copyright and licensing information please refer
 to the COPYING file provided with the program. Following this notice may exist
 a copyright and/or license notice that predates the release of Scribus 1.3.2
 for which a new license (GPL+exception) is in place.
 */
/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef OBJECTSTYLE_H
#define OBJECTSTYLE_H

#include <QString>
#include <QStringList>
#include <Qt>

#include "resourcecollection.h"
#include "style.h"
#include "styles/stylecontextproxy.h"

template<class STYLE> class StyleSet;

/**
 * Inheritable appearance properties shared by page-item object styles.
 * Geometry and object-type-specific behavior intentionally remain outside
 * this foundation so applying a style cannot resize or move existing items.
 */
class SCRIBUS_API ObjectStyle : public BaseStyle
{
public:
	ObjectStyle() : BaseStyle(), m_styleProxy(this)
	{
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
		m_##attr_NAME = attr_DEFAULT; \
		inh_##attr_NAME = true;
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF
	}

	ObjectStyle(const ObjectStyle& other);
	ObjectStyle& operator=(const ObjectStyle& other);

	void saxx(SaxHandler&, const Xml_string&) const override {}
	void saxx(SaxHandler&) const override {}

	QString displayName() const override;
	bool equiv(const BaseStyle& other) const override;
	void erase() override;
	void update(const StyleContext* context) override;

	void getNamedResources(ResourceCollection& lists) const;
	void replaceNamedResources(ResourceCollection& newNames);

#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	const attr_TYPE& attr_GETTER() const { validate(); return m_##attr_NAME; }
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF

#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	void set##attr_NAME(attr_TYPE value) { m_##attr_NAME = value; inh_##attr_NAME = false; }
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF

#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	void reset##attr_NAME() { m_##attr_NAME = attr_DEFAULT; inh_##attr_NAME = true; }
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF

#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	bool isInh##attr_NAME() const { return inh_##attr_NAME; }
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF

#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	bool isDef##attr_NAME() const \
	{ \
		if (!inh_##attr_NAME) \
			return true; \
		const auto* parentObjectStyle = dynamic_cast<const ObjectStyle*>(parentStyle()); \
		return parentObjectStyle && parentObjectStyle->isDef##attr_NAME(); \
	}
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF

private:
	StyleContextProxy m_styleProxy;

#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	attr_TYPE m_##attr_NAME; \
	bool inh_##attr_NAME;
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF
};

struct SCRIBUS_API ObjectStyleImportPlan
{
	QStringList styleNames;
	QStringList colorNames;
	QStringList lineStyleNames;
};

ObjectStyleImportPlan SCRIBUS_API buildObjectStyleImportPlan(const StyleSet<ObjectStyle>& sourceStyles,
														 const QStringList& selectedStyleNames);
QMap<QString, QString> SCRIBUS_API importObjectStyles(const StyleSet<ObjectStyle>& sourceStyles,
													 const QStringList& styleNames,
													 StyleSet<ObjectStyle>& destinationStyles,
													 bool renameOnClash,
													 const QMap<QString, QString>& lineStyleNames = {});

inline ObjectStyle& ObjectStyle::operator=(const ObjectStyle& other)
{
	static_cast<BaseStyle&>(*this) = static_cast<const BaseStyle&>(other);
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	m_##attr_NAME = other.m_##attr_NAME; \
	inh_##attr_NAME = other.inh_##attr_NAME;
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF
	m_contextversion = -1;
	return *this;
}

inline ObjectStyle::ObjectStyle(const ObjectStyle& other)
	: BaseStyle(other), m_styleProxy(this)
{
#define ATTRDEF(attr_TYPE, attr_GETTER, attr_NAME, attr_DEFAULT) \
	m_##attr_NAME = other.m_##attr_NAME; \
	inh_##attr_NAME = other.inh_##attr_NAME;
#include "objectstyle.attrdefs.cxx"
#undef ATTRDEF
	m_contextversion = -1;
}

#endif
