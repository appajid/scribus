/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "contentpalette_page.h"

#include <QObject>
#include <QPushButton>
#include <QWidget>

#include "units.h"
#include "scribus.h"
#include "scribusdoc.h"
#include "pageitem.h"
#include "scpage.h"
#include "scraction.h"
#include "selection.h"

ContentPalette_Page::ContentPalette_Page( QWidget* parent)
	: QWidget(parent),
	  m_unitIndex(SC_PT)
{
	setupUi(this);
	setSizePolicy( QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
	label->setTextFormat(Qt::PlainText);
	label->setWordWrap(true);
	label->setAlignment(Qt::AlignLeft | Qt::AlignTop);

	pagePropertiesButton = new QPushButton(this);
	documentSetupButton = new QPushButton(this);
	verticalLayout_3->addWidget(pagePropertiesButton);
	verticalLayout_3->addWidget(documentSetupButton);
	verticalLayout_3->addStretch(1);

	languageChange();
}

void ContentPalette_Page::setMainWindow(ScribusMainWindow *mw)
{
	m_ScMW = mw;
	connect(pagePropertiesButton, &QPushButton::clicked, this, [this]() {
		if (m_ScMW && m_ScMW->scrActions["pageManageProperties"])
			m_ScMW->scrActions["pageManageProperties"]->trigger();
	});
	connect(documentSetupButton, &QPushButton::clicked, this, [this]() {
		if (m_ScMW && m_ScMW->scrActions["fileDocSetup150"])
			m_ScMW->scrActions["fileDocSetup150"]->trigger();
	});

	connect(m_ScMW, &ScribusMainWindow::UpdateRequest, this, &ContentPalette_Page::handleUpdateRequest);
	// connect(m_ScMW, SIGNAL(UpdateRequest(int)), this, SLOT(handleUpdateRequest(int)));
}

void ContentPalette_Page::setDoc(ScribusDoc *d)
{
	if((d == (ScribusDoc*) m_doc) || (m_ScMW && m_ScMW->scriptIsRunning()))
		return;

	if (m_doc)
	{
		disconnect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
		disconnect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
	}
	
	m_doc  = d;
	m_unitRatio   = m_doc->unitRatio();
	m_unitIndex   = m_doc->unitIndex();

	m_haveDoc  = true;
	m_haveItem = false;
	setEnabled(true);
	setLabelText();

	connect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
	connect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
}

void ContentPalette_Page::unsetDoc()
{
	if (m_doc)
	{
		disconnect(m_doc->m_Selection, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
		disconnect(m_doc             , SIGNAL(docChanged())      , this, SLOT(handleSelectionChanged()));
	}

	m_haveDoc  = false;
	m_haveItem = false;
	m_doc   = nullptr;

	setLabelText();
	setEnabled(false);
}

void ContentPalette_Page::unsetItem()
{
	m_haveItem = false;
	handleSelectionChanged();
}

void ContentPalette_Page::handleSelectionChanged()
{
	if (!m_haveDoc || !m_ScMW || m_ScMW->scriptIsRunning())
		return;

	if (m_doc->m_Selection->count() > 1)
		m_haveItem = true;
	setLabelText();
}

void ContentPalette_Page::handleUpdateRequest(int updateFlags)
{
	Q_UNUSED(updateFlags)
	setLabelText();
}

void ContentPalette_Page::setCurrentItem(PageItem *item)
{
	if (!m_ScMW || m_ScMW->scriptIsRunning())
		return;

	if (!m_doc)
		setDoc(item->doc());

	m_haveItem = true;
}

void ContentPalette_Page::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange)
	{
		languageChange();
		return;
	}
	QWidget::changeEvent(e);
}


void ContentPalette_Page::languageChange()
{
	retranslateUi(this);
	pagePropertiesButton->setText(tr("Page Properties..."));
	documentSetupButton->setText(tr("Document Setup..."));
	setLabelText();
}

void ContentPalette_Page::unitChange()
{
	if (!m_doc)
		return;

	m_unitRatio = m_doc->unitRatio();
	m_unitIndex = m_doc->unitIndex();
	setLabelText();
}

void ContentPalette_Page::setLabelText()
{
	if (!m_haveDoc || !m_doc || !m_doc->currentPage())
	{
		label->setText(tr("Open a document to inspect its page settings."));
		return;
	}

	label->setText(tr("Page %1 of %2\nMaster page: %3\n\nNo object selected. Page and document settings are available here.")
		.arg(m_doc->currentPageNumber() + 1)
		.arg(m_doc->DocPages.count())
		.arg(m_doc->currentPage()->masterPageName()));
}
