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

#include <QAction>
#include <QEnterEvent>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QStatusBar>
#include <QToolBox>
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

	m_categoryBox = new QToolBox(content);
	m_categoryBox->setObjectName("toolCategories");
	m_categoryBox->setMinimumWidth(82);

	QVBoxLayout* navigate = addToolSection("Navigate");
	addToolButtonEntry("toolsSelect", navigate);
	addToolButtonEntry("toolsEditContents", navigate);
	addToolButtonEntry("toolsEditWithStoryEditor", navigate);
	addToolButtonEntry("toolsZoom", navigate);

	QVBoxLayout* frames = addToolSection("Frames");
	addToolButtonEntry("toolsInsertTextFrame", frames);
	addToolButtonEntry("toolsInsertImageFrame", frames);
	addToolButtonEntry("toolsInsertTable", frames);
	addToolButtonEntry("toolsInsertRenderFrame", frames);

	QVBoxLayout* draw = addToolSection("Draw");
	QToolButton* shapeBtn = addToolButtonEntry("toolsInsertShape", draw);
	QToolButton* polygonBtn = addToolButtonEntry("toolsInsertPolygon", draw);
	addToolButtonEntry("toolsInsertArc", draw);
	addToolButtonEntry("toolsInsertSpiral", draw);
	QToolButton* lineBtn = addToolButtonEntry("toolsInsertLine", draw);
	addToolButtonEntry("toolsInsertBezier", draw);
	addToolButtonEntry("toolsInsertFreehandLine", draw);
	QToolButton* calliBtn = addToolButtonEntry("toolsInsertCalligraphicLine", draw);

	QVBoxLayout* modify = addToolSection("Modify");
	addToolButtonEntry("toolsRotate", modify);
	addToolButtonEntry("toolsCopyProperties", modify);
	addToolButtonEntry("toolsLinkTextFrame", modify);
	addToolButtonEntry("toolsUnlinkTextFrame", modify);

	QVBoxLayout* inspect = addToolSection("Inspect");
	addToolButtonEntry("toolsEyeDropper", inspect);
	addToolButtonEntry("toolsMeasurements", inspect);

	QVBoxLayout* interactivePdf = addToolSection("Interactive PDF");
	addToolButtonEntry("toolsPDFPushButton", interactivePdf);
	addToolButtonEntry("toolsPDFCheckBox", interactivePdf);
	addToolButtonEntry("toolsPDFRadioButton", interactivePdf);
	addToolButtonEntry("toolsPDFTextField", interactivePdf);
	addToolButtonEntry("toolsPDFComboBox", interactivePdf);
	addToolButtonEntry("toolsPDFListBox", interactivePdf);
	addToolButtonEntry("toolsPDFAnnotText", interactivePdf);
	addToolButtonEntry("toolsPDFAnnotLink", interactivePdf);

	vbox->addWidget(m_categoryBox);

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
	for (int i = 0; i < m_categoryBox->count() && i < m_categoryTexts.count(); ++i)
		m_categoryBox->setItemText(i, tr(m_categoryTexts[i].toUtf8().constData()));

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

	QString message = name;
	if (!sct.isEmpty())
		message += " (" + sct + ")";
	if (!status.isEmpty())
		message += " — " + status;
	if (m_ScMW && m_ScMW->statusBar())
		m_ScMW->statusBar()->showMessage(message, 3000);
}

QVBoxLayout* ToolPalette::addToolSection(const QString &headerText)
{
	QWidget* page = new QWidget(m_categoryBox);
	QVBoxLayout* layout = new QVBoxLayout(page);
	layout->setContentsMargins(2, 4, 2, 4);
	layout->setSpacing(2);
	layout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
	m_categoryBox->addItem(page, tr(headerText.toUtf8().constData()));
	m_categoryTexts.append(headerText);
	return layout;
}

QToolButton* ToolPalette::addToolButtonEntry(const QString &actionName, QVBoxLayout* sectionLayout)
{
	QPointer<ScrAction> action = m_ScMW->scrActions.value(actionName);
	ToolPaletteButton* btn = new ToolPaletteButton(this, action);
	btn->setObjectName("toolButton");
	btn->setDefaultAction(action);
	btn->setIconSize(QSize(22, 22));
	btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
	btn->setMinimumSize(38, 32);
	sectionLayout->addWidget(btn, 0, Qt::AlignHCenter);
	m_buttons.insert(actionName, btn);
	m_buttonActions.insert(btn, action);
	connect(action, &QAction::toggled, this, [this, action](bool on) {
		if (on)
			updateToolHelp(action);
	});
	return btn;
}
