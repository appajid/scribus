/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "dynamicvariablemanager.h"

#include <QComboBox>
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
#include "styles/paragraphstyle.h"

namespace
{
constexpr int IdRole = Qt::UserRole;
constexpr int BuiltInRole = Qt::UserRole + 1;

QString runningHeaderModeLabel(const QString& mode)
{
	if (mode == DynamicVariableResolver::FirstOnPageMode)
		return QObject::tr("First on Page");
	if (mode == DynamicVariableResolver::LastOnPageMode)
		return QObject::tr("Last on Page");
	if (mode == DynamicVariableResolver::MostRecentMode)
		return QObject::tr("Most Recent");
	return QObject::tr("Unsupported");
}

class VariableEditDialog : public QDialog
{
public:
	VariableEditDialog(ScribusDoc* doc, const DynamicVariable* variable, QWidget* parent)
		: QDialog(parent)
	{
		const bool editing = variable != nullptr;
		setWindowTitle(editing ? tr("Edit Variable") : tr("Add Variable"));
		setMinimumWidth(440);
		auto* layout = new QVBoxLayout(this);
		m_description = new QLabel(this);
		m_description->setWordWrap(true);
		layout->addWidget(m_description);

		auto* form = new QFormLayout;
		m_type = new QComboBox(this);
		m_type->addItem(tr("User Defined"), DynamicVariableResolver::UserDefined);
		m_type->addItem(tr("Running Header"), DynamicVariableResolver::RunningHeader);
		m_type->setEnabled(!editing);
		m_name = new QLineEdit(variable ? variable->name : QString(), this);
		m_value = new QLineEdit(variable ? variable->value : QString(), this);
		m_paragraphStyle = new QComboBox(this);
		if (doc)
		{
			const auto& styles = doc->paragraphStyles();
			for (int i = 0; i < styles.count(); ++i)
				m_paragraphStyle->addItem(styles[i].name(), styles[i].name());
		}
		m_mode = new QComboBox(this);
		m_mode->addItem(tr("First matching paragraph on page"), DynamicVariableResolver::FirstOnPageMode);
		m_mode->addItem(tr("Last matching paragraph on page"), DynamicVariableResolver::LastOnPageMode);
		m_mode->addItem(tr("Most recent matching paragraph"), DynamicVariableResolver::MostRecentMode);

		form->addRow(tr("Type:"), m_type);
		form->addRow(tr("Name:"), m_name);
		m_valueLabel = new QLabel(tr("Value:"), this);
		form->addRow(m_valueLabel, m_value);
		m_styleLabel = new QLabel(tr("Paragraph style:"), this);
		form->addRow(m_styleLabel, m_paragraphStyle);
		m_modeLabel = new QLabel(tr("Use:"), this);
		form->addRow(m_modeLabel, m_mode);
		layout->addLayout(form);
		auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
		m_okButton = buttons->button(QDialogButtonBox::Ok);
		connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
		connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
		connect(m_type, &QComboBox::currentIndexChanged, this, [this]() { updateFields(); });
		connect(m_name, &QLineEdit::textChanged, this, [this]() { updateAcceptState(); });
		connect(m_paragraphStyle, &QComboBox::currentIndexChanged, this, [this]() { updateAcceptState(); });
		layout->addWidget(buttons);

		if (variable && variable->type == DynamicVariableResolver::RunningHeader)
		{
			m_type->setCurrentIndex(m_type->findData(DynamicVariableResolver::RunningHeader));
			m_paragraphStyle->setCurrentIndex(m_paragraphStyle->findData(variable->paragraphStyle));
			m_mode->setCurrentIndex(m_mode->findData(variable->runningHeaderMode));
		}
		updateFields();
		m_name->setFocus();
	}

	QString name() const { return m_name->text().trimmed(); }
	QString value() const { return m_value->text(); }
	bool isRunningHeader() const { return m_type->currentData().toString() == DynamicVariableResolver::RunningHeader; }
	QString paragraphStyle() const { return m_paragraphStyle->currentData().toString(); }
	DynamicVariable::RunningHeaderMode runningHeaderMode() const
	{
		return DynamicVariableResolver::runningHeaderModeFromString(m_mode->currentData().toString());
	}

private:
	void updateFields()
	{
		const bool runningHeader = isRunningHeader();
		m_valueLabel->setVisible(!runningHeader);
		m_value->setVisible(!runningHeader);
		m_styleLabel->setVisible(runningHeader);
		m_paragraphStyle->setVisible(runningHeader);
		m_modeLabel->setVisible(runningHeader);
		m_mode->setVisible(runningHeader);
		m_description->setText(runningHeader
			? tr("A running header displays text from paragraphs using a chosen style and updates automatically when pages reflow.")
			: tr("A user-defined variable stores reusable text that can be updated throughout the document."));
		updateAcceptState();
	}

	void updateAcceptState()
	{
		const bool runningHeaderFieldsValid = !isRunningHeader()
			|| (!paragraphStyle().isEmpty() && !m_mode->currentData().toString().isEmpty());
		m_okButton->setEnabled(!name().isEmpty() && runningHeaderFieldsValid);
	}

	QComboBox* m_type {nullptr};
	QLineEdit* m_name {nullptr};
	QLineEdit* m_value {nullptr};
	QComboBox* m_paragraphStyle {nullptr};
	QComboBox* m_mode {nullptr};
	QLabel* m_description {nullptr};
	QLabel* m_valueLabel {nullptr};
	QLabel* m_styleLabel {nullptr};
	QLabel* m_modeLabel {nullptr};
	QPushButton* m_okButton {nullptr};
};
}

DynamicVariableManager::DynamicVariableManager(ScribusDoc* doc, QWidget* parent)
	: QDialog(parent), m_doc(doc)
{
	setWindowTitle(tr("Variables"));
	resize(700, 400);
	auto* layout = new QVBoxLayout(this);
	auto* description = new QLabel(tr("Insert reusable document values or define running headers that follow styled text. Built-in variables are resolved automatically and cannot be edited."), this);
	description->setWordWrap(true);
	layout->addWidget(description);

	m_table = new QTableWidget(this);
	m_table->setColumnCount(3);
	m_table->setHorizontalHeaderLabels({tr("Name"), tr("Type"), tr("Value or Definition")});
	m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
	m_table->verticalHeader()->setVisible(false);
	m_table->setAlternatingRowColors(true);
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
		QString value = variable.value;
		if (builtIn)
			value = m_doc->resolveDynamicVariable(variable.id);
		else if (variable.type == DynamicVariableResolver::RunningHeader)
			value = tr("%1 — %2").arg(variable.paragraphStyle, runningHeaderModeLabel(variable.runningHeaderMode));
		m_table->setItem(row, 2, new QTableWidgetItem(value));
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
	if (!m_doc)
		return;
	VariableEditDialog dialog(m_doc, nullptr, this);
	while (dialog.exec() == QDialog::Accepted)
	{
		QString id;
		if (dialog.isRunningHeader())
			id = m_doc->addRunningHeaderVariable(dialog.name(), dialog.paragraphStyle(), dialog.runningHeaderMode());
		else
			id = m_doc->addDynamicVariable(dialog.name(), dialog.value());
		if (!id.isEmpty())
		{
			m_doc->changed();
			refresh();
			return;
		}
		QMessageBox::warning(this, tr("Invalid Variable"), dialog.isRunningHeader()
			? tr("Choose a unique, non-reserved name, an existing paragraph style, and a supported running-header mode.")
			: tr("Choose a unique, non-reserved variable name."));
	}
}

void DynamicVariableManager::editVariable()
{
	const QString id = selectedId();
	const DynamicVariable* variable = m_doc ? m_doc->dynamicVariable(id) : nullptr;
	if (!variable)
		return;
	VariableEditDialog dialog(m_doc, variable, this);
	while (dialog.exec() == QDialog::Accepted)
	{
		const bool updated = variable->type == DynamicVariableResolver::RunningHeader
			? m_doc->updateRunningHeaderVariable(id, dialog.name(), dialog.paragraphStyle(), dialog.runningHeaderMode())
			: m_doc->updateDynamicVariable(id, dialog.name(), dialog.value());
		if (updated)
		{
			m_doc->changed();
			m_doc->regionsChanged()->update(QRectF());
			refresh();
			return;
		}
		QMessageBox::warning(this, tr("Invalid Variable"), variable->type == DynamicVariableResolver::RunningHeader
			? tr("Choose a unique, non-reserved name, an existing paragraph style, and a supported running-header mode.")
			: tr("Choose a unique, non-reserved variable name."));
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
