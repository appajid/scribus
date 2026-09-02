/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/


#include "propertiespalette_text.h"

#if defined(_MSC_VER) && !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

#include "appmodes.h"
#include "commonstrings.h"
#include "fontcombo.h"
#include "iconmanager.h"
#include "pageitem.h"
#include "pageitem_table.h"
#include "pageitem_textframe.h"
#include "propertywidget_advanced.h"
#include "propertywidget_alignment.h"
#include "propertywidget_distance.h"
#include "propertywidget_fontfeatures.h"
#include "propertywidget_hyphenation.h"
#include "propertywidget_orphans.h"
#include "propertywidget_paragraphstyle.h"
#include "propertywidget_pareffect.h"
#include "propertywidget_pathtext.h"
#include "propertywidget_text.h"
#include "scribus.h"
#include "scribusapp.h"
#include "selection.h"
#include "stylecombos.h"
#include "undomanager.h"
#include "widgets/section_container.h"

//using namespace std;

PropertiesPalette_Text::PropertiesPalette_Text( QWidget* parent) : QWidget(parent)
{
	characterStyleWidgets = new PropertyWidget_ParagraphStyle(this);
	characterStyleWidgets->hideParagraphStyle(true);
	scCharacterStyles = new SectionContainer(tr("Character Styles"), "SectionTPCharacterStyles", true, true);
	scCharacterStyles->setCanSaveState(true);
	scCharacterStyles->setWidget(characterStyleWidgets);
	scCharacterStyles->restorePreferences();

	paragraphStyleWidgets = new PropertyWidget_ParagraphStyle(this);
	paragraphStyleWidgets->hideCharacterStyle(true);
	scParagraphStyles = new SectionContainer(tr("Paragraph Styles"), "SectionTPParagraphStyles", true, true);
	scParagraphStyles->setCanSaveState(true);
	scParagraphStyles->setWidget(paragraphStyleWidgets);
	scParagraphStyles->restorePreferences();

	textWidgets = new PropertyWidget_Text(this);
	scText = new SectionContainer(tr("Font"), "SectionTPFont", true, true);
	scText->setCanSaveState(true);
	scText->setWidget(textWidgets);
	scText->restorePreferences();

	alignmentWidgets = new PropertyWidget_Alignment(this);
	scAlignment = new SectionContainer(tr("Alignment"), "SectionTPAlignment", true, true);
	scAlignment->setCanSaveState(true);
	scAlignment->setWidget(alignmentWidgets);
	scAlignment->restorePreferences();

	distanceWidgets = new PropertyWidget_Distance(this);
	scDistance = new SectionContainer(tr("Columns && Indentation"), "SectionTPColumnsMargins", true, false);
	scDistance->setCanSaveState(true);
	scDistance->setWidget(distanceWidgets);
	scDistance->restorePreferences();

	advancedWidgets = new PropertyWidget_Advanced(this);
	scAdvanced = new SectionContainer(tr("Typography"), "SectionTPTypography", true, false);
	scAdvanced->setCanSaveState(true);
	scAdvanced->setWidget(advancedWidgets);
	scAdvanced->restorePreferences();

	dropCapWidgets = new PropertyWidget_ParEffect(this);
	dropCapWidgets->setEffectMode(PropertyWidget_ParEffect::EffectMode::DropCaps);
	scDropCaps = new SectionContainer(tr("Drop Caps"), "SectionTPDropCaps", true, false);
	scDropCaps->setCanSaveState(true);
	scDropCaps->setWidget(dropCapWidgets);
	scDropCaps->restorePreferences();

	listWidgets = new PropertyWidget_ParEffect(this);
	listWidgets->setEffectMode(PropertyWidget_ParEffect::EffectMode::Lists);
	scLists = new SectionContainer(tr("Lists && Numbering"), "SectionTPLists", true, false);
	scLists->setCanSaveState(true);
	scLists->setWidget(listWidgets);
	scLists->restorePreferences();

	hyphenationWidget = new PropertyWidget_Hyphenation(this);
	scHyphenation = new SectionContainer(tr("Hyphenation && Language"), "SectionTPHyphenation", true, false);
	scHyphenation->setCanSaveState(true);
	scHyphenation->setWidget(hyphenationWidget);
	scHyphenation->restorePreferences();

	orphanBox = new PropertyWidget_Orphans(this);
	scOrphans = new SectionContainer(tr("Orphans && Widows"), "SectionTPOrphansWidows", true, false);
	scOrphans->setCanSaveState(true);
	scOrphans->setWidget(orphanBox);
	scOrphans->restorePreferences();

	fontfeaturesWidget = new PropertyWidget_FontFeatures(this);
	scFontFeatures = new SectionContainer(tr("OpenType Features"), "SectionTPOpenTypeFeatures", true, false);
	scFontFeatures->setCanSaveState(true);
	scFontFeatures->setWidget(fontfeaturesWidget);
	scFontFeatures->restorePreferences();

	pathTextWidgets = new PropertyWidget_PathText(this);
	scPathText = new SectionContainer(tr("Text on Path"), "SectionTPTextOnPath", true, false);
	scPathText->setCanSaveState(true);
	scPathText->setWidget(pathTextWidgets);
	scPathText->restorePreferences();

	textModeTabs = new QTabBar(this);
	textModeTabs->setObjectName(QStringLiteral("textInspectorTabs"));
	textModeTabs->setAccessibleName(tr("Text controls"));
	textModeTabs->setDrawBase(false);
	textModeTabs->setExpanding(true);
	textModeTabs->setUsesScrollButtons(false);
	textModeTabs->addTab(tr("Type"));
	textModeTabs->addTab(tr("Paragraph"));

	auto* typePage = new QWidget(this);
	auto* typeLayout = new QVBoxLayout(typePage);
	typeLayout->setContentsMargins(0, 0, 0, 0);
	typeLayout->setSpacing(6);
	typeLayout->addWidget(scCharacterStyles);
	typeLayout->addWidget(scText);
	typeLayout->addWidget(scAdvanced);
	typeLayout->addWidget(scFontFeatures);
	typeLayout->addWidget(scDropCaps);
	typeLayout->addWidget(scPathText);
	typeLayout->addStretch(1);

	auto* paragraphPage = new QWidget(this);
	auto* paragraphLayout = new QVBoxLayout(paragraphPage);
	paragraphLayout->setContentsMargins(0, 0, 0, 0);
	paragraphLayout->setSpacing(6);
	paragraphLayout->addWidget(scParagraphStyles);
	paragraphLayout->addWidget(scAlignment);
	paragraphLayout->addWidget(scDistance);
	paragraphLayout->addWidget(scLists);
	paragraphLayout->addWidget(scHyphenation);
	paragraphLayout->addWidget(scOrphans);
	paragraphLayout->addStretch(1);

	textModeStack = new QStackedWidget(this);
	textModeStack->setObjectName(QStringLiteral("textInspectorStack"));
	textModeStack->addWidget(typePage);
	textModeStack->addWidget(paragraphPage);
	connect(textModeTabs, &QTabBar::currentChanged, textModeStack, &QStackedWidget::setCurrentIndex);

	// Layout stack
	QVBoxLayout * lyt = new QVBoxLayout();
	lyt->setContentsMargins(0, 0, 0, 0);
	lyt->setSpacing(6);
	lyt->addWidget(textModeTabs);
	lyt->addWidget(textModeStack, 1);

	setLayout(lyt);
	setEnabled(false);
	iconSetChange();
	languageChange();

	connect(ScQApp, SIGNAL(iconSetChanged()), this, SLOT(iconSetChange()));
	connect(ScQApp, SIGNAL(localeChanged()), this, SLOT(localeChange()));

}

void PropertiesPalette_Text::setMainWindow(ScribusMainWindow* mw)
{
	m_ScMW = mw;

	textWidgets->setMainWindow(mw);
	advancedWidgets->setMainWindow(mw);
	alignmentWidgets->setMainWindow(mw);
	fontfeaturesWidget->setMainWindow(mw);
	distanceWidgets->setMainWindow(mw);
	hyphenationWidget->setMainWindow(mw);
	characterStyleWidgets->setMainWindow(mw);
	paragraphStyleWidgets->setMainWindow(mw);
	dropCapWidgets->setMainWindow(mw);
	listWidgets->setMainWindow(mw);
	pathTextWidgets->setMainWindow(mw);

	connect(m_ScMW, SIGNAL(UpdateRequest(int))     , this  , SLOT(handleUpdateRequest(int)));
}

void PropertiesPalette_Text::setDoc(ScribusDoc *d)
{
	if ((d == (ScribusDoc*) m_doc) || (m_ScMW && m_ScMW->scriptIsRunning()))
		return;

	if (m_doc)
	{
		disconnect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
		disconnect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
	}

	m_doc  = d;
	m_item = nullptr;

	m_unitRatio = m_doc->unitRatio();
	m_unitIndex = m_doc->unitIndex();

	m_haveDoc  = true;
	m_haveItem = false;

	textWidgets->setDoc(m_doc);
	advancedWidgets->setDoc(m_doc);
	alignmentWidgets->setDoc(m_doc);
	fontfeaturesWidget->setDoc(m_doc);
	distanceWidgets->setDoc(m_doc);
	dropCapWidgets->setDoc(m_doc);
	listWidgets->setDoc(m_doc);
	hyphenationWidget->setDoc(m_doc);
	characterStyleWidgets->setDoc(m_doc);
	paragraphStyleWidgets->setDoc(m_doc);
	orphanBox->setDoc(m_doc);
	pathTextWidgets->setDoc(m_doc);

	connect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
	connect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
}

void PropertiesPalette_Text::unsetDoc()
{
	if (m_doc)
	{
		disconnect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
		disconnect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
	}

	m_haveDoc  = false;
	m_haveItem = false;
	m_doc      = nullptr;
	m_item     = nullptr;

	textWidgets->setDoc(nullptr);
	advancedWidgets->setDoc(nullptr);
	alignmentWidgets->setDoc(nullptr);
	fontfeaturesWidget->setDoc(nullptr);
	distanceWidgets->setDoc(nullptr);
	hyphenationWidget->setDoc(nullptr);
	characterStyleWidgets->setDoc(nullptr);
	paragraphStyleWidgets->setDoc(nullptr);
	orphanBox->setDoc(nullptr);
	dropCapWidgets->setDoc(nullptr);
	listWidgets->setDoc(nullptr);
	pathTextWidgets->setDoc(nullptr);

	setEnabled(false);
}

void PropertiesPalette_Text::unsetItem()
{
	m_haveItem = false;
	m_item     = nullptr;
	handleSelectionChanged();
}

PageItem* PropertiesPalette_Text::currentItemFromSelection()
{
	PageItem *currentItem = nullptr;

	if (m_doc)
	{
		if (m_doc->m_Selection->count() > 1)
			currentItem = m_doc->m_Selection->itemAt(0);
		else if (m_doc->m_Selection->count() == 1)
			currentItem = m_doc->m_Selection->itemAt(0);
		if (currentItem  && currentItem->isTable() && m_doc->appMode == modeEditTable)
			currentItem = currentItem->asTable()->activeCell().textFrame();
	}

	return currentItem;
}

void PropertiesPalette_Text::handleSelectionChanged()
{
	if (!m_haveDoc || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	PageItem* currItem = currentItemFromSelection();
	if (m_doc->m_Selection->count() > 1)
	{
		setEnabled(false);
	}
	else
	{
		int itemType = currItem ? (int) currItem->itemType() : -1;
		m_haveItem = (itemType != -1);

		switch (itemType)
		{
		case -1:
			m_haveItem = false;
			m_item = nullptr;
			setEnabled(false);
			break;
		case PageItem::TextFrame:
		case PageItem::PathText:
			setEnabled(true);
			break;
		case PageItem::Table:
			setEnabled(m_doc->appMode == modeEditTable);
			break;
		default:
			setEnabled(false);
			break;
		}
	}
	if (currItem)
	{
		setCurrentItem(currItem);
	}
	updateGeometry();
}

void PropertiesPalette_Text::handleUpdateRequest(int updateFlags)
{
	textWidgets->handleUpdateRequest(updateFlags);
	characterStyleWidgets->handleUpdateRequest(updateFlags);
	paragraphStyleWidgets->handleUpdateRequest(updateFlags);
	dropCapWidgets->handleUpdateRequest(updateFlags);
	listWidgets->handleUpdateRequest(updateFlags);

}

void PropertiesPalette_Text::setCurrentItem(PageItem *item)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;
	//CB We shouldn't really need to process this if our item is the same one
	//maybe we do if the item has been changed by scripter.. but that should probably
	//set some status if so.
	//FIXME: This won't work until when a canvas deselect happens, m_item must be nullptr.
	//if (m_item == i)
	//	return;

	if (!m_doc)
		setDoc(item->doc());

	m_haveItem = false;
	m_item = item;

	if ((m_item->isGroup()) && (!m_item->isSingleSel))
	{
		setEnabled(false);
	}

	m_haveItem = true;

	if (!sender())
	{
		alignmentWidgets->handleSelectionChanged();
		distanceWidgets->handleSelectionChanged();
		dropCapWidgets->handleSelectionChanged();
		listWidgets->handleSelectionChanged();
	}

	if (m_item->isTextFrame() || m_item->isPathText() || m_item->isTable())
	{
		ParagraphStyle parStyle =  m_item->itemText.defaultStyle();
		if (m_doc->appMode == modeEdit || m_doc->appMode == modeEditTable)
			m_item->currentTextProps(parStyle);
		updateParagraphStyle(parStyle);
	}
	if (m_item->isOSGFrame())
	{
		setEnabled(false);
	}
	if (m_item->asSymbol())
	{
		setEnabled(false);
	}
}

void PropertiesPalette_Text::iconSetChange()
{
	// no local icons to change
}

void PropertiesPalette_Text::unitChange()
{
	if (!m_haveDoc)
		return;
	bool tmp = m_haveItem;
	m_haveItem = false;

	advancedWidgets->unitChange();
	alignmentWidgets->unitChange();
	fontfeaturesWidget->unitChange();
	distanceWidgets->unitChange();
	pathTextWidgets->unitChange();
	dropCapWidgets->unitChange();
	listWidgets->unitChange();
	textWidgets->unitChange();

	m_haveItem = tmp;
}

void PropertiesPalette_Text::localeChange()
{
	// Nothing to do here!
}

void PropertiesPalette_Text::showFontSize(double s)
{
	textWidgets->showFontSize(s);
}

void PropertiesPalette_Text::showLanguage(const QString& w)
{
	hyphenationWidget->showLanguage(w);
}

void PropertiesPalette_Text::updateCharStyle(const CharStyle& charStyle)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;

	advancedWidgets->updateCharStyle(charStyle);
	fontfeaturesWidget->updateCharStyle(charStyle);
	hyphenationWidget->updateCharStyle(charStyle);
	textWidgets->updateCharStyle(charStyle);
}

void PropertiesPalette_Text::updateParagraphStyle(const ParagraphStyle& newCurrent)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;

	advancedWidgets->updateStyle(newCurrent);
	fontfeaturesWidget->updateStyle(newCurrent);
	orphanBox->updateStyle(newCurrent);
	dropCapWidgets->updateStyle(newCurrent);
	listWidgets->updateStyle(newCurrent);
	hyphenationWidget->updateStyle(newCurrent);
	textWidgets->updateStyle(newCurrent);
	characterStyleWidgets->updateStyle(newCurrent);
	paragraphStyleWidgets->updateStyle(newCurrent);
	alignmentWidgets->updateStyle(newCurrent);
	distanceWidgets->updateStyle(newCurrent);
}

void PropertiesPalette_Text::updateTextStyles()
{
	characterStyleWidgets->updateTextStyles();
	paragraphStyleWidgets->updateTextStyles();
	dropCapWidgets->updateTextStyles();
	listWidgets->updateTextStyles();
}

void PropertiesPalette_Text::showAlignment(int e)
{
	alignmentWidgets->showAlignment(e);
}

void PropertiesPalette_Text::showDirection(int e)
{
	alignmentWidgets->showDirection(e);
}

void PropertiesPalette_Text::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
		return;
	}
	QWidget::changeEvent(e);
}

void PropertiesPalette_Text::languageChange()
{
	const int currentMode = textModeTabs->currentIndex();
	textModeTabs->setTabText(0, tr("Type"));
	textModeTabs->setTabText(1, tr("Paragraph"));
	textModeTabs->setCurrentIndex(currentMode);
	textModeTabs->setTabToolTip(0, tr("Character formatting and type effects"));
	textModeTabs->setTabToolTip(1, tr("Paragraph formatting and flow"));

	scCharacterStyles->setText(tr("Character Styles"));
	scParagraphStyles->setText(tr("Paragraph Styles"));
	scAdvanced->setText(tr("Typography"));
	scAlignment->setText(tr("Alignment && Direction"));
	scDistance->setText(tr("Indents, Spacing && Columns"));
	scFontFeatures->setText(tr("OpenType Features"));
	scHyphenation->setText(tr("Hyphenation && Language"));
	scOrphans->setText(tr("Keep Options"));
	scDropCaps->setText(tr("Drop Caps"));
	scLists->setText(tr("Lists && Numbering"));
	scPathText->setText(tr("Text on Path"));
	scText->setText(tr("Font && Character Color"));

	orphanBox->languageChange();
	distanceWidgets->languageChange();
	advancedWidgets->languageChange();
	alignmentWidgets->languageChange();
	pathTextWidgets->languageChange();
	fontfeaturesWidget->languageChange();
	hyphenationWidget->languageChange();
	textWidgets->languageChange();
	characterStyleWidgets->languageChange();
	paragraphStyleWidgets->languageChange();
	dropCapWidgets->languageChange();
	listWidgets->languageChange();
}
