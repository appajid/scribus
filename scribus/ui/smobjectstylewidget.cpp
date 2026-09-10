/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#include "smobjectstylewidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "colorcombo.h"
#include "commonstrings.h"
#include "scribusdoc.h"
#include "scrspinbox.h"
#include "styles/objectstyle.h"
#include "units.h"
#include "widgets/combo_blendmode.h"

namespace
{
void selectData(QComboBox* combo, int value)
{
	const int index = combo->findData(value);
	combo->setCurrentIndex(index >= 0 ? index : 0);
}

void selectData(QComboBox* combo, const QString& value)
{
	const int index = combo->findData(value);
	combo->setCurrentIndex(index >= 0 ? index : 0);
}
}

SMObjectStyleWidget::SMObjectStyleWidget(QWidget* parent)
	: QWidget(parent)
{
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(8, 8, 8, 8);
	mainLayout->setSpacing(10);

	m_messageLabel = new QLabel(this);
	m_messageLabel->setWordWrap(true);
	m_messageLabel->setProperty("secondaryText", true);
	mainLayout->addWidget(m_messageLabel);

	auto* parentRow = new QWidget(this);
	auto* parentLayout = new QHBoxLayout(parentRow);
	parentLayout->setContentsMargins(0, 0, 0, 0);
	m_parentLabel = new QLabel(parentRow);
	m_parentCombo = new QComboBox(parentRow);
	m_parentCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	parentLayout->addWidget(m_parentLabel);
	parentLayout->addWidget(m_parentCombo, 1);
	mainLayout->addWidget(parentRow);

	m_editor = new QWidget(this);
	auto* editorLayout = new QHBoxLayout(m_editor);
	editorLayout->setContentsMargins(0, 0, 0, 0);
	editorLayout->setSpacing(10);

	m_fillGroup = new QGroupBox(this);
	m_fillGroup->setProperty("section", true);
	auto* fillLayout = new QFormLayout(m_fillGroup);
	m_fillColor = new ColorCombo(ColorCombo::fancyPixmaps, m_fillGroup);
	m_fillShade = new ScrSpinBox(0.0, 100.0, m_fillGroup, SC_PERCENT);
	m_fillOpacity = new ScrSpinBox(0.0, 100.0, m_fillGroup, SC_PERCENT);
	m_fillBlend = new ComboBlendMode(m_fillGroup);
	addOverrideRow(fillLayout, tr("Color"), m_fillColor, m_fillColorOverride);
	addOverrideRow(fillLayout, tr("Shade"), m_fillShade, m_fillShadeOverride);
	addOverrideRow(fillLayout, tr("Opacity"), m_fillOpacity, m_fillOpacityOverride);
	addOverrideRow(fillLayout, tr("Blend mode"), m_fillBlend, m_fillBlendOverride);

	m_strokeGroup = new QGroupBox(this);
	m_strokeGroup->setProperty("section", true);
	auto* strokeLayout = new QFormLayout(m_strokeGroup);
	m_lineColor = new ColorCombo(ColorCombo::fancyPixmaps, m_strokeGroup);
	m_lineShade = new ScrSpinBox(0.0, 100.0, m_strokeGroup, SC_PERCENT);
	m_lineWidth = new ScrSpinBox(0.0, 300.0, m_strokeGroup, SC_PT);
	m_lineStyle = new QComboBox(m_strokeGroup);
	for (int style = Qt::SolidLine; style <= Qt::DashDotDotLine; ++style)
		m_lineStyle->addItem(CommonStrings::translatePenStyleName(static_cast<Qt::PenStyle>(style)), style);
	m_lineCap = new QComboBox(m_strokeGroup);
	m_lineCap->addItem(tr("Flat"), static_cast<int>(Qt::FlatCap));
	m_lineCap->addItem(tr("Square"), static_cast<int>(Qt::SquareCap));
	m_lineCap->addItem(tr("Round"), static_cast<int>(Qt::RoundCap));
	m_lineJoin = new QComboBox(m_strokeGroup);
	m_lineJoin->addItem(tr("Miter"), static_cast<int>(Qt::MiterJoin));
	m_lineJoin->addItem(tr("Bevel"), static_cast<int>(Qt::BevelJoin));
	m_lineJoin->addItem(tr("Round"), static_cast<int>(Qt::RoundJoin));
	m_lineOpacity = new ScrSpinBox(0.0, 100.0, m_strokeGroup, SC_PERCENT);
	m_lineBlend = new ComboBlendMode(m_strokeGroup);
	m_customLineStyle = new QComboBox(m_strokeGroup);
	addOverrideRow(strokeLayout, tr("Color"), m_lineColor, m_lineColorOverride);
	addOverrideRow(strokeLayout, tr("Shade"), m_lineShade, m_lineShadeOverride);
	addOverrideRow(strokeLayout, tr("Width"), m_lineWidth, m_lineWidthOverride);
	addOverrideRow(strokeLayout, tr("Type"), m_lineStyle, m_lineStyleOverride);
	addOverrideRow(strokeLayout, tr("Cap"), m_lineCap, m_lineCapOverride);
	addOverrideRow(strokeLayout, tr("Join"), m_lineJoin, m_lineJoinOverride);
	addOverrideRow(strokeLayout, tr("Opacity"), m_lineOpacity, m_lineOpacityOverride);
	addOverrideRow(strokeLayout, tr("Blend mode"), m_lineBlend, m_lineBlendOverride);
	addOverrideRow(strokeLayout, tr("Line style"), m_customLineStyle, m_customLineStyleOverride);

	m_shapeGroup = new QGroupBox(this);
	m_shapeGroup->setProperty("section", true);
	auto* shapeLayout = new QFormLayout(m_shapeGroup);
	m_cornerRadius = new ScrSpinBox(0.0, 3000.0, m_shapeGroup, SC_PT);
	addOverrideRow(shapeLayout, tr("Corner radius"), m_cornerRadius, m_cornerRadiusOverride);

	auto* leftColumn = new QVBoxLayout();
	leftColumn->addWidget(m_fillGroup);
	leftColumn->addWidget(m_shapeGroup);
	leftColumn->addStretch();
	editorLayout->addLayout(leftColumn, 1);
	editorLayout->addWidget(m_strokeGroup, 1);
	mainLayout->addWidget(m_editor, 1);

	connect(m_parentCombo, &QComboBox::currentIndexChanged, this, [this]() { emitChanged(); });
	languageChange();
}

QWidget* SMObjectStyleWidget::addOverrideRow(QFormLayout* layout, const QString& label, QWidget* editor, QCheckBox*& overrideBox)
{
	auto* row = new QWidget(this);
	auto* rowLayout = new QHBoxLayout(row);
	rowLayout->setContentsMargins(0, 0, 0, 0);
	rowLayout->setSpacing(6);
	overrideBox = new QCheckBox(tr("Override"), row);
	overrideBox->setToolTip(tr("Store this value in the style instead of inheriting it"));
	rowLayout->addWidget(overrideBox);
	rowLayout->addWidget(editor, 1);
	layout->addRow(label, row);
	connect(overrideBox, &QCheckBox::toggled, this, [this, editor](bool checked) {
		editor->setEnabled(checked);
		emitChanged();
	});
	connectEditor(editor);
	return row;
}

void SMObjectStyleWidget::connectEditor(QWidget* editor)
{
	if (auto* combo = qobject_cast<QComboBox*>(editor))
		connect(combo, &QComboBox::currentIndexChanged, this, [this]() { emitChanged(); });
	else if (auto* spin = qobject_cast<QDoubleSpinBox*>(editor))
		connect(spin, &QDoubleSpinBox::valueChanged, this, [this]() { emitChanged(); });
}

void SMObjectStyleWidget::setOverrideState(QCheckBox* overrideBox, QWidget* editor, bool inherited, bool forceOverride)
{
	overrideBox->setChecked(forceOverride || !inherited);
	overrideBox->setEnabled(!forceOverride);
	editor->setEnabled(forceOverride || !inherited);
}

void SMObjectStyleWidget::emitChanged()
{
	if (!m_loading)
		emit changed();
}

void SMObjectStyleWidget::setDoc(ScribusDoc* doc)
{
	m_doc = doc;
	m_loading = true;
	m_fillColor->setColors(m_doc ? m_doc->PageColors : ColorList(), true);
	m_lineColor->setColors(m_doc ? m_doc->PageColors : ColorList(), true);
	m_customLineStyle->clear();
	m_customLineStyle->addItem(tr("None"), QString());
	if (m_doc)
	{
		QStringList lineStyles = m_doc->docLineStyles.keys();
		lineStyles.sort(Qt::CaseInsensitive);
		for (const QString& name : lineStyles)
			m_customLineStyle->addItem(name, name);
		setUnit(m_doc->unitIndex());
	}
	m_loading = false;
}

void SMObjectStyleWidget::setUnit(int unitIndex)
{
	if (unitIndex == m_unitIndex)
		return;
	m_unitIndex = unitIndex;
	m_lineWidth->setNewUnit(unitIndex);
	m_cornerRadius->setNewUnit(unitIndex);
}

void SMObjectStyleWidget::showStyle(ObjectStyle* style, const QList<ObjectStyle>& allStyles)
{
	if (!style)
		return;
	m_loading = true;
	m_messageLabel->hide();
	m_editor->setEnabled(true);
	m_parentCombo->setEnabled(!style->isDefaultStyle());
	m_parentCombo->clear();
	m_parentCombo->addItem(tr("None"), QString());
	if (!style->isDefaultStyle())
	{
		QStringList candidates;
		for (const ObjectStyle& candidate : allStyles)
		{
			if (candidate.name() != style->name() && style->canInherit(candidate.name()))
				candidates.append(candidate.name());
		}
		candidates.sort(Qt::CaseInsensitive);
		for (const QString& name : candidates)
			m_parentCombo->addItem(name == CommonStrings::DefaultObjectStyle ? CommonStrings::trDefaultObjectStyle : name, name);
	}
	selectData(m_parentCombo, style->parent());

	m_fillColor->setCurrentColor(style->fillColor());
	m_fillShade->setValue(style->fillShade());
	m_fillOpacity->setValue((1.0 - style->fillTransparency()) * 100.0);
	selectData(m_fillBlend, style->fillBlendMode());
	m_lineColor->setCurrentColor(style->lineColor());
	m_lineShade->setValue(style->lineShade());
	m_lineWidth->setValue(style->lineWidth() * unitGetRatioFromIndex(m_unitIndex));
	selectData(m_lineStyle, static_cast<int>(style->lineStyle()));
	selectData(m_lineCap, static_cast<int>(style->lineCap()));
	selectData(m_lineJoin, static_cast<int>(style->lineJoin()));
	m_lineOpacity->setValue((1.0 - style->lineTransparency()) * 100.0);
	selectData(m_lineBlend, style->lineBlendMode());
	selectData(m_customLineStyle, style->customLineStyle());
	m_cornerRadius->setValue(style->cornerRadius() * unitGetRatioFromIndex(m_unitIndex));

	const bool force = style->isDefaultStyle();
	setOverrideState(m_fillColorOverride, m_fillColor, style->isInhFillColor(), force);
	setOverrideState(m_fillShadeOverride, m_fillShade, style->isInhFillShade(), force);
	setOverrideState(m_fillOpacityOverride, m_fillOpacity, style->isInhFillTransparency(), force);
	setOverrideState(m_fillBlendOverride, m_fillBlend, style->isInhFillBlendMode(), force);
	setOverrideState(m_lineColorOverride, m_lineColor, style->isInhLineColor(), force);
	setOverrideState(m_lineShadeOverride, m_lineShade, style->isInhLineShade(), force);
	setOverrideState(m_lineWidthOverride, m_lineWidth, style->isInhLineWidth(), force);
	setOverrideState(m_lineStyleOverride, m_lineStyle, style->isInhLineStyle(), force);
	setOverrideState(m_lineCapOverride, m_lineCap, style->isInhLineCap(), force);
	setOverrideState(m_lineJoinOverride, m_lineJoin, style->isInhLineJoin(), force);
	setOverrideState(m_lineOpacityOverride, m_lineOpacity, style->isInhLineTransparency(), force);
	setOverrideState(m_lineBlendOverride, m_lineBlend, style->isInhLineBlendMode(), force);
	setOverrideState(m_customLineStyleOverride, m_customLineStyle, style->isInhCustomLineStyle(), force);
	setOverrideState(m_cornerRadiusOverride, m_cornerRadius, style->isInhCornerRadius(), force);
	m_loading = false;
}

void SMObjectStyleWidget::showMultipleStyles()
{
	m_loading = true;
	m_messageLabel->setText(tr("Multiple object styles are selected. Select one style to edit its appearance."));
	m_messageLabel->show();
	m_parentCombo->clear();
	m_parentCombo->setEnabled(false);
	m_editor->setEnabled(false);
	m_loading = false;
}

void SMObjectStyleWidget::updateStyle(ObjectStyle& style) const
{
	if (style.isDefaultStyle())
		style.setParent(QString());
	else
		style.setParent(m_parentCombo->currentData().toString());

	if (m_fillColorOverride->isChecked()) style.setFillColor(m_fillColor->currentColor()); else style.resetFillColor();
	if (m_fillShadeOverride->isChecked()) style.setFillShade(m_fillShade->value()); else style.resetFillShade();
	if (m_fillOpacityOverride->isChecked()) style.setFillTransparency(1.0 - m_fillOpacity->value() / 100.0); else style.resetFillTransparency();
	if (m_fillBlendOverride->isChecked()) style.setFillBlendMode(m_fillBlend->currentData().toInt()); else style.resetFillBlendMode();
	if (m_lineColorOverride->isChecked()) style.setLineColor(m_lineColor->currentColor()); else style.resetLineColor();
	if (m_lineShadeOverride->isChecked()) style.setLineShade(m_lineShade->value()); else style.resetLineShade();
	if (m_lineWidthOverride->isChecked()) style.setLineWidth(m_lineWidth->getValue(SC_PT)); else style.resetLineWidth();
	if (m_lineStyleOverride->isChecked()) style.setLineStyle(static_cast<Qt::PenStyle>(m_lineStyle->currentData().toInt())); else style.resetLineStyle();
	if (m_lineCapOverride->isChecked()) style.setLineCap(static_cast<Qt::PenCapStyle>(m_lineCap->currentData().toInt())); else style.resetLineCap();
	if (m_lineJoinOverride->isChecked()) style.setLineJoin(static_cast<Qt::PenJoinStyle>(m_lineJoin->currentData().toInt())); else style.resetLineJoin();
	if (m_lineOpacityOverride->isChecked()) style.setLineTransparency(1.0 - m_lineOpacity->value() / 100.0); else style.resetLineTransparency();
	if (m_lineBlendOverride->isChecked()) style.setLineBlendMode(m_lineBlend->currentData().toInt()); else style.resetLineBlendMode();
	if (m_customLineStyleOverride->isChecked()) style.setCustomLineStyle(m_customLineStyle->currentData().toString()); else style.resetCustomLineStyle();
	if (m_cornerRadiusOverride->isChecked()) style.setCornerRadius(m_cornerRadius->getValue(SC_PT)); else style.resetCornerRadius();
}

void SMObjectStyleWidget::languageChange()
{
	m_parentLabel->setText(tr("Based On:"));
	m_fillGroup->setTitle(tr("Fill"));
	m_strokeGroup->setTitle(tr("Stroke"));
	m_shapeGroup->setTitle(tr("Shape"));
	setAccessibleName(tr("Object style properties"));
}

void SMObjectStyleWidget::changeEvent(QEvent* event)
{
	if (event->type() == QEvent::LanguageChange)
		languageChange();
	QWidget::changeEvent(event);
}
