/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include <QApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRegularExpression>
#include <QStyleHints>

#include "scconfig.h"

#include "api/api_application.h"
#include "iconmanager.h"
#include "splash.h"
#include "util.h"

ScSplashScreen::ScSplashScreen(const QPixmap & pixmap, const QRect messageRect, Qt::WindowFlags f ) : QSplashScreen( pixmap, f)
{
#if defined _WIN32
	QFont font("Lucida Sans Unicode", 9);
#elif defined(__INNOTEK_LIBC__)
	QFont font("WarpSans", 8);
#elif defined(Q_OS_MACOS)
	QFont font("Helvetica Regular", 11);
#else
	QFont font("DejaVu Sans", 8);
	if (!font.exactMatch())
		font.setFamily("Bitstream Vera Sans");
#endif
	setFont(font);
	m_messageRect = messageRect;
}

void ScSplashScreen::setStatus( const QString &message )
{
	static QRegularExpression rx("&\\S*");
	QString tmp(message);
	qsizetype f = 0;
	while (f != -1)
	{
		f = tmp.indexOf(rx);
		if (f != -1)
		{
			tmp.remove(f, 1);
			f = 0;
		}
	}

	showMessage(tmp);
}

void ScSplashScreen::drawContents(QPainter* painter)
{
	painter->setRenderHint(QPainter::Antialiasing, true);
	const bool isDark = QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
	const QColor textColor = isDark ? QColor(245, 245, 247) : QColor(32, 34, 38);
	const QColor secondaryColor = isDark ? QColor(190, 194, 201) : QColor(84, 88, 96);
	const QColor accentColor(10, 132, 255);

	const QPixmap appIcon = IconManager::instance().loadPixmap("app-icon", QSize(78, 78));
	painter->drawPixmap(QRect(38, 38, 78, 78), appIcon);

	QFont titleFont(font());
	titleFont.setPointSize(32);
	titleFont.setWeight(QFont::DemiBold);
	painter->setFont(titleFont);
	painter->setPen(textColor);
	painter->drawText(QRect(134, 42, 205, 48), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Scribus"));

	QFont taglineFont(font());
	taglineFont.setPointSize(16);
	taglineFont.setWeight(QFont::DemiBold);
	painter->setFont(taglineFont);
	painter->setPen(accentColor);
	painter->drawText(QRect(134, 88, 205, 28), Qt::AlignLeft | Qt::AlignVCenter, tr("Publish beautifully."));

	QFont bodyFont(font());
	bodyFont.setPointSize(13);
	painter->setFont(bodyFont);
	painter->setPen(secondaryColor);
	painter->drawText(QRect(134, 118, 205, 24), Qt::AlignLeft | Qt::AlignVCenter,
		tr("Version %1").arg(ScribusAPI::getVersion()));
	painter->drawText(QRect(38, 225, 286, 24), Qt::AlignLeft | Qt::AlignVCenter, message());

	const QRectF progressTrack(38, 262, 286, 8);
	painter->setPen(Qt::NoPen);
	painter->setBrush(isDark ? QColor(75, 78, 84) : QColor(205, 208, 214));
	painter->drawRoundedRect(progressTrack, 4, 4);
	painter->setBrush(accentColor);
	painter->drawRoundedRect(QRectF(progressTrack.x(), progressTrack.y(), progressTrack.width() * 0.66, progressTrack.height()), 4, 4);

	QFont footerFont(font());
	footerFont.setPointSize(11);
	painter->setFont(footerFont);
	painter->setPen(secondaryColor);
	painter->drawText(QRect(38, 291, 286, 24), Qt::AlignLeft | Qt::AlignVCenter,
		tr("Open Source Desktop Publishing"));
	if (ScribusAPI::isSVN())
		painter->drawText(QRect(38, 318, 286, 20), Qt::AlignLeft | Qt::AlignVCenter, tr("Development build"));
}
