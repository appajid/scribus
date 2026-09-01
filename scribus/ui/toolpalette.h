/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          toolpalette.h  -  description
                             -------------------
    begin                : Fri Aug 28 2026
 ***************************************************************************/

#ifndef TOOLPALETTE_H
#define TOOLPALETTE_H

#include "scribusapi.h"
#include "ui/docks/dock_panelbase.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

class QAction;
class QEvent;
class QMenu;
class QToolBox;
class QToolButton;
class QVBoxLayout;

class AutoformButtonGroup;
class ScribusDoc;
class ScribusMainWindow;
class ScrAction;

/**
  * @brief InDesign-like dockable Tools palette.
  *
  * Provides a compact category-at-a-time set of QToolButtons bound to the
  * same ScrAction objects used throughout Scribus, plus sub-tool flyout
  * menus for shapes, polygons, lines, and calligraphic settings.
  */
class SCRIBUS_API ToolPalette : public DockPanelBase
{
	Q_OBJECT

public:
	ToolPalette(QWidget* parent);
	~ToolPalette();

	void changeEvent(QEvent *e) override;
	/** @brief Update palette state for a newly active document. */
	void setDoc(ScribusDoc* doc);

	int SubMode { 0 };
	int ValCount { 0 };
	double *ShapeVals { nullptr };

public slots:
	void GetPolyProps();
	void SelShape(int s, int c, qreal *vals);
	void languageChange();

public:
	/** @brief Update the tool help label for the given action. */
	void updateToolHelp(QAction* action);

protected:
	/** @brief Add a compact, collapsible category to the palette. */
	QVBoxLayout* addToolSection(const QString &text);
	/** @brief Add a tool button bound to the named tool action. */
	QToolButton* addToolButtonEntry(const QString &actionName, QVBoxLayout* sectionLayout);
	/** @brief Set the number of polygon corners from a flyout preset. */
	void setPolygonSides(int sides);

	QHash<QString, QToolButton*> m_buttons;
	QHash<QToolButton*, ScrAction*> m_buttonActions;
	QStringList m_categoryTexts;
	QToolBox* m_categoryBox { nullptr };
	QMenu* insertPolygonButtonMenu { nullptr };
	QMenu* lineButtonMenu { nullptr };
	QAction* idPolygonPropertiesAction { nullptr };
	AutoformButtonGroup* autoFormButtonGroup { nullptr };
	ScribusMainWindow* m_ScMW { nullptr };
	ScribusDoc* m_doc { nullptr };
};

#endif // TOOLPALETTE_H
