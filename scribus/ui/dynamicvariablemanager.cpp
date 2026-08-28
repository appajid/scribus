/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "dynamicvariablemanager.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

#include "dynamicvariable.h"
#include "marks.h"
#include "scribusdoc.h"

namespace
{
constexpr int IdRole = Qt::UserRole;
constexpr int BuiltInRole = Qt::UserRole + 1;

class VariableEditDialog : public QDialog
{
public:
	VariableEditDialog(const QString& name, const QString& value, QWidget* parent)
		: QDialog(parent)
	{
		setWindowTitle(name.isEmpty() ? tr("Add Variable") : tr("Edit Variable"));
		auto* layout = new QVBoxLayout(this);
		auto* form = new QFormLayout;
		m_name = new QLineEdit(name, this);
		m_value = new QLineEdit(value, this);
		form->addRow(tr("Name:"), m_name);
		form->addRow(tr("Value:"), m_value);
		layout->addLayout(form);
		auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
		connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
		layout->addWidget(buttons);
	}

	QString name() const { return m_name->text().trimmed(); }
	QString value() const { return m_value->text(); }

private:
	QLineEdit* m_name {nullptr};
	QLineEdit* m_value {nullptr};
};
}

DynamicVariableManager::DynamicVariableManager(ScribusDoc* doc, QWidget* parent)
	: QDialog(parent), m_doc(doc)
{
	setWindowTitle(tr("Variables"));
	resize(640, 360);
	auto* layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel(tr("Built-in variables are resolved from the document. User-defined variables can be reused anywhere in the text."), this));

	m_table = new QTableWidget(this);
	m_table->setColumnCount(3);
	m_table->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Current Value")});
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	layout->addWidget(m_table);

	auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	QPushButton* addButton = buttons->addButton(tr("Add"), QDialogButtonBox::ActionRole);
	m_editButton = buttons->addButton(tr("Edit"), QDialogButtonBox::ActionRole);
	m_deleteButton = buttons->addButton(tr("Delete"), QDialogButtonBox::ActionRole);
	connect(addButton, &QPushButton::clicked, this, [this]() { addVariable(); });
	connect(m_editButton, &QPushButton::clicked, this, [this]() { editVariable(); });
	connect(m_deleteButton, &QPushButton::clicked, this, [this]() { deleteVariable(); });
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_table, &QTableWidget::itemSelectionChanged, this, [this]() { updateButtons(); });
	connect(m_table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem*) { editVariable(); });
	layout->addWidget(buttons);
	refresh();
}

QString DynamicVariableManager::selectedId() const
{
	const int row = m_table->currentRow();
	return row >= 0 ? m_table->item(row, 0)->data(IdRole).toString() : QString();
}

void DynamicVariableManager::refresh()
{
	m_table->setRowCount(0);
	if (!m_doc)
		return;

	auto addRow = [this](const DynamicVariable& variable, bool builtIn) {
		const int row = m_table->rowCount();
		m_table->insertRow(row);
		auto* nameItem = new QTableWidgetItem(variable.name);
		nameItem->setData(IdRole, variable.id);
		nameItem->setData(BuiltInRole, builtIn);
		m_table->setItem(row, 0, nameItem);
		m_table->setItem(row, 1, new QTableWidgetItem(DynamicVariableResolver::displayNameForType(variable.type)));
		m_table->setItem(row, 2, new QTableWidgetItem(builtIn ? m_doc->resolveDynamicVariable(variable.id) : variable.value));
	};

	for (const DynamicVariable& variable : DynamicVariableResolver::builtInVariables())
		addRow(variable, true);
	for (const DynamicVariable& variable : m_doc->dynamicVariables())
		addRow(variable, false);
	updateButtons();
}

void DynamicVariableManager::updateButtons()
{
	const int row = m_table->currentRow();
	const bool editable = row >= 0 && !m_table->item(row, 0)->data(BuiltInRole).toBool();
	m_editButton->setEnabled(editable);
	m_deleteButton->setEnabled(editable);
}

void DynamicVariableManager::addVariable()
{
	VariableEditDialog dialog(QString(), QString(), this);
	while (dialog.exec() == QDialog::Accepted)
	{
		if (dialog.name().isEmpty())
		{
			QMessageBox::warning(this, tr("Invalid Variable"), tr("The variable name cannot be empty."));
			continue;
		}
		if (!m_doc->addDynamicVariable(dialog.name(), dialog.value()).isEmpty())
		{
			m_doc->changed();
			refresh();
			return;
		}
		QMessageBox::warning(this, tr("Duplicate Variable"), tr("A variable with that name already exists."));
	}
}

void DynamicVariableManager::editVariable()
{
	const QString id = selectedId();
	const DynamicVariable* variable = m_doc ? m_doc->dynamicVariable(id) : nullptr;
	if (!variable)
		return;
	VariableEditDialog dialog(variable->name, variable->value, this);
	while (dialog.exec() == QDialog::Accepted)
	{
		if (m_doc->updateDynamicVariable(id, dialog.name(), dialog.value()))
		{
			m_doc->changed();
			m_doc->regionsChanged()->update(QRectF());
			refresh();
			return;
		}
		QMessageBox::warning(this, tr("Invalid Variable"), tr("Variable names must be non-empty and unique."));
	}
}

void DynamicVariableManager::deleteVariable()
{
	const QString id = selectedId();
	const DynamicVariable* variable = m_doc ? m_doc->dynamicVariable(id) : nullptr;
	if (!variable)
		return;
	QString message = tr("Delete the variable “%1”?").arg(variable->name);
	Mark* mark = m_doc->getDynamicVariableMark(id);
	if (mark && m_doc->isMarkUsed(mark))
		message += QStringLiteral("\n\n") + tr("Existing references will remain in the document and display no value until the deletion is undone.");
	if (QMessageBox::warning(this, tr("Delete Variable"), message, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
		return;
	m_doc->removeDynamicVariable(id);
	m_doc->changed();
	m_doc->regionsChanged()->update(QRectF());
	refresh();
}
