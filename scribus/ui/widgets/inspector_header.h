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

#ifndef INSPECTOR_HEADER_H
#define INSPECTOR_HEADER_H

#include <QWidget>

class QLabel;

class InspectorHeader : public QWidget
{
	Q_OBJECT

public:
	explicit InspectorHeader(const QString& iconName, QWidget* parent = nullptr);

	void setTitle(const QString& title);
	void setSubtitle(const QString& subtitle);

public slots:
	void iconSetChange();

private:
	QString m_iconName;
	QLabel* m_iconLabel { nullptr };
	QLabel* m_titleLabel { nullptr };
	QLabel* m_subtitleLabel { nullptr };
};

#endif // INSPECTOR_HEADER_H
