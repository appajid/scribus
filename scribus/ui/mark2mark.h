#ifndef MARK2MARK_H
#define MARK2MARK_H

#include "markinsert.h"
#include "ui_mark2mark.h"

class SCRIBUS_API Mark2Mark : public MarkInsert, private Ui::Mark2MarkDlg
{
    Q_OBJECT

public:
	explicit Mark2Mark(const QList<Mark*>& marks, Mark* omitMark = nullptr, QWidget *parent = nullptr);
	void values(QString& label, Mark* &mrk) override;
	void setValues(const QString label, const Mark* mrk) override;
	void crossReferenceValues(QString& label, Mark*& mrk, CrossReferenceFormat& format, QString& prefix, QString& suffix) const;
	void setCrossReferenceValues(const QString& label, const Mark* mrk, CrossReferenceFormat format,
		const QString& prefix, const QString& suffix);

protected:
    void changeEvent(QEvent *e) override;

private:
	void languageChange();
};

#endif // MARK2MARK_H
