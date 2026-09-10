/*
 For general Scribus (>=1.3.2) copyright and licensing information please refer
 to the COPYING file provided with the program. Following this notice may exist
 a copyright and/or license notice that predates the release of Scribus 1.3.2
 for which a new license (GPL+exception) is in place.
 */

// Syntax: ATTRDEF(datatype, gettername, name, defaultvalue)

ATTRDEF(QString, fillColor, FillColor, QStringLiteral("None"))
ATTRDEF(double, fillShade, FillShade, 100.0)
ATTRDEF(QString, lineColor, LineColor, QStringLiteral("None"))
ATTRDEF(double, lineShade, LineShade, 100.0)
ATTRDEF(double, lineWidth, LineWidth, 0.0)
ATTRDEF(Qt::PenStyle, lineStyle, LineStyle, Qt::SolidLine)
ATTRDEF(Qt::PenCapStyle, lineCap, LineCap, Qt::FlatCap)
ATTRDEF(Qt::PenJoinStyle, lineJoin, LineJoin, Qt::MiterJoin)
ATTRDEF(double, fillTransparency, FillTransparency, 0.0)
ATTRDEF(double, lineTransparency, LineTransparency, 0.0)
ATTRDEF(int, fillBlendMode, FillBlendMode, 0)
ATTRDEF(int, lineBlendMode, LineBlendMode, 0)
ATTRDEF(double, cornerRadius, CornerRadius, 0.0)
ATTRDEF(QString, customLineStyle, CustomLineStyle, QString())
