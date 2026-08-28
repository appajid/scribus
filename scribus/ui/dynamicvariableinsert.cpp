/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "dynamicvariableinsert.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "dynamicvariable.h"
#include "scribusdoc.h"

DynamicVariableInsert::DynamicVariableInsert(ScribusDoc* doc, const PageItem* frame, QWidget* parent)
	: QDialog(parent), m_doc(doc), m_frame(frame)
{
	setWindowTitle(tr("Insert Variable"));
	auto* layout = new QVBoxLayout(this);
	auto* form = new QFormLayout;
	m_variables = new QComboBox(this);
	for (const DynamicVariable& variable : DynamicVariableResolver::builtInVariables())
		m_variables->addItem(variable.name, variable.id);
	if (m_doc && !m_doc->dynamicVariables().isEmpty())
	{
		m_variables->insertSeparator(m_variables->count());
		for (const DynamicVariable& variable : m_doc->dynamicVariables())
			m_variables->addItem(variable.name, variable.id);
	}
	form->addRow(tr("Variable:"), m_variables);
	m_preview = new QLabel(this);
	m_preview->setTextInteractionFlags(Qt::TextSelectableByMouse);
	form->addRow(tr("Current value:"), m_preview);
	layout->addLayout(form);
	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_variables, &QComboBox::currentIndexChanged, this, [this]() { updatePreview(); });
	layout->addWidget(buttons);
	updatePreview();
}

QString DynamicVariableInsert::variableId() const
{
	return m_variables->currentData().toString();
}

void DynamicVariableInsert::updatePreview()
{
	m_preview->setText(m_doc ? m_doc->resolveDynamicVariable(variableId(), m_frame) : QString());
}
