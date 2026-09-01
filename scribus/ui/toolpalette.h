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
#include <QString>
#include <QStringList>

class QAction;
class QEvent;
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
  * Provides persistent Adobe-style primary tools. Closely related tools
  * share a segmented flyout button, and the last selected member becomes
  * the primary action while all commands remain the shared ScrAction objects.
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
	/** @brief Show concise tool help in the status bar. */
	void updateToolHelp(QAction* action);

protected:
	/** @brief Add a tool button bound to the named tool action. */
	QToolButton* addToolButtonEntry(const QString &actionName, QVBoxLayout* layout);
	/** @brief Attach related shared actions as a segmented flyout. */
	QMenu* configureToolGroup(QToolButton* button, const QStringList &actionNames);
	/** @brief Add a visual separator between tool families. */
	void addToolSeparator(QVBoxLayout* layout);
	/** @brief Set the number of polygon corners from a flyout preset. */
	void setPolygonSides(int sides);

	QHash<QString, QToolButton*> m_buttons;
	QMenu* insertPolygonButtonMenu { nullptr };
	QMenu* lineButtonMenu { nullptr };
	QMenu* calligraphicSettingsMenu { nullptr };
	QAction* idPolygonPropertiesAction { nullptr };
	AutoformButtonGroup* autoFormButtonGroup { nullptr };
	ScribusMainWindow* m_ScMW { nullptr };
	ScribusDoc* m_doc { nullptr };
};

#endif // TOOLPALETTE_H
