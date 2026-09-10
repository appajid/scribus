/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#ifndef SMOBJECTSTYLEWIDGET_H
#define SMOBJECTSTYLEWIDGET_H

#include <QWidget>

class ColorCombo;
class ComboBlendMode;
class ObjectStyle;
class QCheckBox;
class QComboBox;
class QFormLayout;
class QGroupBox;
class QLabel;
class ScrSpinBox;
class ScribusDoc;

class SMObjectStyleWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SMObjectStyleWidget(QWidget* parent = nullptr);

	void setDoc(ScribusDoc* doc);
	void setUnit(int unitIndex);
	void showStyle(ObjectStyle* style, const QList<ObjectStyle>& allStyles);
	void showMultipleStyles();
	void updateStyle(ObjectStyle& style) const;
	void languageChange();

signals:
	void changed();

protected:
	void changeEvent(QEvent* event) override;

private:
	QWidget* addOverrideRow(QFormLayout* layout, const QString& label, QWidget* editor, QCheckBox*& overrideBox);
	void connectEditor(QWidget* editor);
	void setOverrideState(QCheckBox* overrideBox, QWidget* editor, bool inherited, bool forceOverride);
	void emitChanged();

	ScribusDoc* m_doc { nullptr };
	bool m_loading { false };
	int m_unitIndex { 0 };

	QLabel* m_messageLabel { nullptr };
	QLabel* m_parentLabel { nullptr };
	QComboBox* m_parentCombo { nullptr };
	QWidget* m_editor { nullptr };
	QGroupBox* m_fillGroup { nullptr };
	QGroupBox* m_strokeGroup { nullptr };
	QGroupBox* m_shapeGroup { nullptr };

	ColorCombo* m_fillColor { nullptr };
	ScrSpinBox* m_fillShade { nullptr };
	ScrSpinBox* m_fillOpacity { nullptr };
	ComboBlendMode* m_fillBlend { nullptr };
	ColorCombo* m_lineColor { nullptr };
	ScrSpinBox* m_lineShade { nullptr };
	ScrSpinBox* m_lineWidth { nullptr };
	QComboBox* m_lineStyle { nullptr };
	QComboBox* m_lineCap { nullptr };
	QComboBox* m_lineJoin { nullptr };
	ScrSpinBox* m_lineOpacity { nullptr };
	ComboBlendMode* m_lineBlend { nullptr };
	QComboBox* m_customLineStyle { nullptr };
	ScrSpinBox* m_cornerRadius { nullptr };

	QCheckBox* m_fillColorOverride { nullptr };
	QCheckBox* m_fillShadeOverride { nullptr };
	QCheckBox* m_fillOpacityOverride { nullptr };
	QCheckBox* m_fillBlendOverride { nullptr };
	QCheckBox* m_lineColorOverride { nullptr };
	QCheckBox* m_lineShadeOverride { nullptr };
	QCheckBox* m_lineWidthOverride { nullptr };
	QCheckBox* m_lineStyleOverride { nullptr };
	QCheckBox* m_lineCapOverride { nullptr };
	QCheckBox* m_lineJoinOverride { nullptr };
	QCheckBox* m_lineOpacityOverride { nullptr };
	QCheckBox* m_lineBlendOverride { nullptr };
	QCheckBox* m_customLineStyleOverride { nullptr };
	QCheckBox* m_cornerRadiusOverride { nullptr };
};

#endif
