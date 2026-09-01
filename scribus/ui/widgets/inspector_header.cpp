/***************************************************************************
 *   Copyright (C) 2026 by the Scribus Team                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "inspector_header.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "iconmanager.h"
#include "scribusapp.h"

InspectorHeader::InspectorHeader(const QString& iconName, QWidget* parent)
	: QWidget(parent),
	  m_iconName(iconName)
{
	setObjectName(QStringLiteral("inspectorHeader"));
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

	m_iconLabel = new QLabel(this);
	m_iconLabel->setObjectName(QStringLiteral("inspectorContextIcon"));
	m_iconLabel->setAlignment(Qt::AlignCenter);
	m_iconLabel->setFixedSize(28, 28);

	m_titleLabel = new QLabel(this);
	m_titleLabel->setObjectName(QStringLiteral("inspectorContextTitle"));
	m_titleLabel->setTextFormat(Qt::PlainText);

	m_subtitleLabel = new QLabel(this);
	m_subtitleLabel->setObjectName(QStringLiteral("inspectorContextSubtitle"));
	m_subtitleLabel->setTextFormat(Qt::PlainText);
	m_subtitleLabel->setWordWrap(true);

	auto* textLayout = new QVBoxLayout();
	textLayout->setContentsMargins(0, 0, 0, 0);
	textLayout->setSpacing(1);
	textLayout->addWidget(m_titleLabel);
	textLayout->addWidget(m_subtitleLabel);

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(10, 8, 10, 8);
	layout->setSpacing(8);
	layout->addWidget(m_iconLabel, 0, Qt::AlignTop);
	layout->addLayout(textLayout, 1);

	iconSetChange();
	connect(ScQApp, &ScribusQApp::iconSetChanged, this, &InspectorHeader::iconSetChange);
}

void InspectorHeader::setTitle(const QString& title)
{
	m_titleLabel->setText(title);
	setAccessibleName(title);
}

void InspectorHeader::setSubtitle(const QString& subtitle)
{
	m_subtitleLabel->setText(subtitle);
	setAccessibleDescription(subtitle);
}

void InspectorHeader::iconSetChange()
{
	m_iconLabel->setPixmap(IconManager::instance().loadPixmap(m_iconName, 20));
}
