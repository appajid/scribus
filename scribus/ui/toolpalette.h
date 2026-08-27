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
class QGridLayout;
class QLabel;
class QMenu;
class QToolButton;
class QVBoxLayout;

class AutoformButtonGroup;
class ScribusDoc;
class ScribusMainWindow;
class ScrAction;

/**
  * @brief InDesign-like dockable Tools palette.
  *
  * Provides a grouped, checkable grid of QToolButtons bound to the same
  * ScrAction objects used by the legacy ModeToolBar, plus sub-tool flyout
  * menus (autoforms, polygon side presets, line variants, calligraphic pen
  * settings) and a tool help label at the bottom.
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
	/** @brief Add a section header label to the tools grid. */
	QLabel* addSectionHeader(const QString &text, int *row);
	/** @brief Add a tool button bound to the named tool action. */
	QToolButton* addToolButtonEntry(const QString &actionName, int row, int col);
	/** @brief Set the number of polygon corners from a flyout preset. */
	void setPolygonSides(int sides);

	QHash<QString, QToolButton*> m_buttons;
	QHash<QToolButton*, ScrAction*> m_buttonActions;
	QList<QLabel*> m_sectionHeaders;
	QStringList m_sectionHeaderTexts;
	QLabel* toolHelpLabel { nullptr };
	QGridLayout* grid { nullptr };
	QMenu* insertPolygonButtonMenu { nullptr };
	QMenu* lineButtonMenu { nullptr };
	QAction* idPolygonPropertiesAction { nullptr };
	AutoformButtonGroup* autoFormButtonGroup { nullptr };
	ScribusMainWindow* m_ScMW { nullptr };
	ScribusDoc* m_doc { nullptr };
};

#endif // TOOLPALETTE_H