/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
/***************************************************************************
                          toolpalette.cpp -  description
                             -------------------
    begin                : Fri Aug 28 2026
 ***************************************************************************/

#include "toolpalette.h"

#include <algorithm>

#include <QAction>
#include <QEnterEvent>
#include <QEvent>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QToolButton>
#include <QVBoxLayout>

#include "autoformbuttongroup.h"
#include "modetoolbar.h"
#include "polyprops.h"
#include "scraction.h"
#include "scribus.h"
#include "scribusapp.h"
#include "scribusdoc.h"

class ToolPaletteButton : public QToolButton
{
public:
	ToolPaletteButton(ToolPalette* palette, ScrAction* action)
		: QToolButton(palette), m_palette(palette), m_action(action)
	{
		setAutoRaise(false);
	}

protected:
	void enterEvent(QEnterEvent *e) override
	{
		if (m_palette && m_action)
			m_palette->updateToolHelp(m_action);
		QToolButton::enterEvent(e);
	}

	void mousePressEvent(QMouseEvent *e) override
	{
		if (m_palette && m_action)
			m_palette->updateToolHelp(m_action);
		QToolButton::mousePressEvent(e);
	}

private:
	ToolPalette* m_palette { nullptr };
	ScrAction* m_action { nullptr };
};

ToolPalette::ToolPalette(QWidget* parent) : DockPanelBase( tr("Tools"), "tool-select", parent)
{
	m_ScMW = qobject_cast<ScribusMainWindow*>(parent);
	if (!m_ScMW)
		return;
	// objectName/prefs context are derived from the dock title ("Tools")
	// by DockPanelBase; do not override them here as the Advanced Docking
	// System uses the objectName to save and restore the dock state.

	ValCount = 32;
	static double AutoShapes0[] = {0.0, 0.0, 0.0, 0.0, 100.0, 0.0, 100.0, 0.0, 100.0, 0.0, 100.0, 0.0,
								  100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 0.0, 100.0, 0.0, 100.0,
								  0.0, 100.0, 0.0, 100.0, 0.0, 0.0, 0.0, 0.0};
	ShapeVals = AutoShapes0;

	QWidget* content = new QWidget(this);
	QVBoxLayout* vbox = new QVBoxLayout(content);
	vbox->setContentsMargins(4, 4, 4, 4);
	vbox->setSpacing(2);

	grid = new QGridLayout();
	grid->setHorizontalSpacing(2);
	grid->setVerticalSpacing(2);

	int row = 0;

	addSectionHeader( "Select & Edit", &row );
	addToolButtonEntry("toolsSelect", row, 0);
	addToolButtonEntry("toolsEditContents", row, 1);
	addToolButtonEntry("toolsEditWithStoryEditor", row, 2);
	++row;

	addSectionHeader( "Insert", &row );
	addToolButtonEntry("toolsInsertTextFrame", row, 0);
	addToolButtonEntry("toolsInsertImageFrame", row, 1);
	addToolButtonEntry("toolsInsertRenderFrame", row, 2);
	addToolButtonEntry("toolsInsertTable", row, 3);
	++row;

	addSectionHeader( "Draw", &row );
	QToolButton* shapeBtn = addToolButtonEntry("toolsInsertShape", row, 0);
	QToolButton* polygonBtn = addToolButtonEntry("toolsInsertPolygon", row, 1);
	addToolButtonEntry("toolsInsertArc", row, 2);
	addToolButtonEntry("toolsInsertSpiral", row, 3);
	++row;
	QToolButton* lineBtn = addToolButtonEntry("toolsInsertLine", row, 0);
	addToolButtonEntry("toolsInsertBezier", row, 1);
	addToolButtonEntry("toolsInsertFreehandLine", row, 2);
	QToolButton* calliBtn = addToolButtonEntry("toolsInsertCalligraphicLine", row, 3);
	++row;

	addSectionHeader( "Modify", &row );
	addToolButtonEntry("toolsRotate", row, 0);
	addToolButtonEntry("toolsZoom", row, 1);
	addToolButtonEntry("toolsLinkTextFrame", row, 2);
	addToolButtonEntry("toolsUnlinkTextFrame", row, 3);
	++row;

	addSectionHeader( "Inspect", &row );
	addToolButtonEntry("toolsEyeDropper", row, 0);
	addToolButtonEntry("toolsCopyProperties", row, 1);
	++row;

	vbox->addLayout(grid);

	toolHelpLabel = new QLabel(content);
	toolHelpLabel->setObjectName("toolHelpLabel");
	toolHelpLabel->setWordWrap(true);
	toolHelpLabel->setTextFormat(Qt::RichText);
	toolHelpLabel->setTextInteractionFlags(Qt::NoTextInteraction);
	toolHelpLabel->setMinimumHeight(fontMetrics().lineSpacing() * 3);
	vbox->addWidget(toolHelpLabel);
	vbox->addStretch(1);

	setWidget(content);

	// --- sub-tool flyout menus ------------------------------------------------

	// Shape: reuse the shared AutoformButtonGroup of the legacy ModeToolBar so
	// that the shape sub-mode state (SubMode, ShapeVals, ValCount) is identical
	// in both bars and existing Path Draw behavior is preserved.
	if (m_ScMW->modeToolBar)
	{
		autoFormButtonGroup = m_ScMW->modeToolBar->getAutoformButtonGroup();
		if (autoFormButtonGroup)
		{
			shapeBtn->setMenu(autoFormButtonGroup);
			shapeBtn->setPopupMode(QToolButton::MenuButtonPopup);
			connect( autoFormButtonGroup, SIGNAL(FormSel(int,int,qreal*)), this, SLOT(SelShape(int,int,qreal*)) );
		}

		// Calligraphic line: reuse the shared angle/width pen settings.
		QMenu* calMenu = m_ScMW->modeToolBar->getCalligraphicMenu();
		if (calMenu)
		{
			calliBtn->setMenu(calMenu);
			calliBtn->setPopupMode(QToolButton::MenuButtonPopup);
		}
	}

	// Polygon: side presets plus the classic properties dialog.
	insertPolygonButtonMenu = new QMenu(this);
	for (int sides : {3, 4, 5, 6, 8, 10, 12, 24, 36, 64})
	{
		QAction* act = insertPolygonButtonMenu->addAction(QString("%1").arg(sides));
		act->setData(sides);
		connect(act, &QAction::triggered, this, [this, sides]() { setPolygonSides(sides); });
	}
	insertPolygonButtonMenu->addSeparator();
	idPolygonPropertiesAction = insertPolygonButtonMenu->addAction( tr("Properties..."), this, SLOT(GetPolyProps()) );
	polygonBtn->setMenu(insertPolygonButtonMenu);
	polygonBtn->setPopupMode(QToolButton::MenuButtonPopup);

	// Line: quick variants of the other drawing tools.
	lineButtonMenu = new QMenu(this);
	lineButtonMenu->addAction(m_ScMW->scrActions["toolsInsertLine"].data());
	lineButtonMenu->addAction(m_ScMW->scrActions["toolsInsertBezier"].data());
	lineButtonMenu->addAction(m_ScMW->scrActions["toolsInsertFreehandLine"].data());
	lineBtn->setMenu(lineButtonMenu);
	lineBtn->setPopupMode(QToolButton::MenuButtonPopup);

	// The tool actions default to checked=true (see ScrAction::setToggleAction);
	// clear that here so the palette starts with a single active tool instead of
	// every button shown pressed. AppModeHelper sets the correct exclusive state
	// whenever the active tool changes.
	for (auto it = m_buttons.constBegin(); it != m_buttons.constEnd(); ++it)
		it.value()->setChecked(false);
	if (QToolButton* selectBtn = m_buttons.value("toolsSelect"))
		selectBtn->setChecked(true);

	updateToolHelp(m_ScMW->scrActions.value("toolsSelect"));
}

ToolPalette::~ToolPalette()
{
}

void ToolPalette::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
		languageChange();
	else
		QWidget::changeEvent(e);
}

void ToolPalette::setDoc(ScribusDoc* doc)
{
	m_doc = doc;
}

void ToolPalette::GetPolyProps()
{
	ScribusDoc* doc = m_doc ? m_doc : m_ScMW->doc;
	if (!doc)
		return;
	PolygonProps* dia = new PolygonProps(m_ScMW,
		doc->itemToolPrefs().polyCorners,
		doc->itemToolPrefs().polyFactor,
		doc->itemToolPrefs().polyUseFactor,
		doc->itemToolPrefs().polyRotation,
		doc->itemToolPrefs().polyCurvature,
		doc->itemToolPrefs().polyInnerRot,
		doc->itemToolPrefs().polyOuterCurvature);
	if (dia->exec())
	{
		dia->getValues(
			&doc->itemToolPrefs().polyCorners,
			&doc->itemToolPrefs().polyFactor,
			&doc->itemToolPrefs().polyUseFactor,
			&doc->itemToolPrefs().polyRotation,
			&doc->itemToolPrefs().polyCurvature,
			&doc->itemToolPrefs().polyInnerRot,
			&doc->itemToolPrefs().polyOuterCurvature);
		m_ScMW->scrActions["toolsInsertPolygon"]->trigger();
	}
	delete dia;
}

void ToolPalette::setPolygonSides(int sides)
{
	ScribusDoc* doc = m_doc ? m_doc : m_ScMW->doc;
	if (!doc)
		return;
	doc->itemToolPrefs().polyCorners = sides;
	m_ScMW->scrActions["toolsInsertPolygon"]->trigger();
}

void ToolPalette::SelShape(int s, int c, qreal *vals)
{
	if (autoFormButtonGroup)
		m_ScMW->scrActions["toolsInsertShape"]->setIcon(QIcon(autoFormButtonGroup->getIconPixmap(s, 16)));
	SubMode = s;
	ValCount = c;
	ShapeVals = vals;
	m_ScMW->scrActions["toolsInsertShape"]->setChecked(false);
	m_ScMW->scrActions["toolsInsertShape"]->setChecked(true);
}

void ToolPalette::languageChange()
{
	for (int i = 0; i < m_sectionHeaders.count() && i < m_sectionHeaderTexts.count(); ++i)
		m_sectionHeaders[i]->setText(tr(m_sectionHeaderTexts[i].toUtf8().constData()));

	if (idPolygonPropertiesAction)
		idPolygonPropertiesAction->setText( tr("Properties...") );

	updateToolHelp(m_ScMW->scrActions["toolsSelect"]);
}

void ToolPalette::updateToolHelp(QAction* action)
{
	if (!action)
		return;
	ScrAction* scrAct = qobject_cast<ScrAction*>(action);
	QString name = scrAct ? scrAct->cleanMenuText() : action->text();
	QString sct = action->shortcut().toString(QKeySequence::NativeText);
	QString status = action->statusTip();
	if (!sct.isEmpty() && status.endsWith("(" + sct + ")"))
		status.chop(sct.length() + 2);

	QString text = "<qt><b>" + name.toHtmlEscaped() + "</b>";
	if (!sct.isEmpty())
		text += " (" + sct.toHtmlEscaped() + ")";
	text += "</qt>";
	if (!status.isEmpty())
		text += "<br/>" + status.toHtmlEscaped();
	toolHelpLabel->setText(text);
}

QLabel* ToolPalette::addSectionHeader(const QString &headerText, int *row)
{
	QLabel* label = new QLabel(tr(headerText.toUtf8().constData()), this);
	label->setObjectName("toolSectionHeader");
	QFont fnt(label->font());
	fnt.setBold(true);
	fnt.setPointSize(std::max(fnt.pointSize() - 1, 7));
	label->setFont(fnt);
	label->setContentsMargins(2, 4, 0, 0);
	grid->addWidget(label, *row, 0, 1, 4);
	m_sectionHeaders.append(label);
	m_sectionHeaderTexts.append(headerText);
	++(*row);
	return label;
}

QToolButton* ToolPalette::addToolButtonEntry(const QString &actionName, int row, int col)
{
	QPointer<ScrAction> action = m_ScMW->scrActions.value(actionName);
	ToolPaletteButton* btn = new ToolPaletteButton(this, action);
	btn->setObjectName("toolButton");
	btn->setDefaultAction(action);
	btn->setIconSize(QSize(22, 22));
	btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
	grid->addWidget(btn, row, col);
	m_buttons.insert(actionName, btn);
	m_buttonActions.insert(btn, action);
	connect(action, &QAction::toggled, this, [this, action](bool on) {
		if (on)
			updateToolHelp(action);
	});
	return btn;
}