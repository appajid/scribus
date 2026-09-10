/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/

#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QHash>
#include <QPointer>
#include <QSet>

class QEvent;

#include "styleitem.h"
#include "ui/scrpalettebase.h"
#include "ui_stylemanager.h"

class  StyleItem;
class  ScribusDoc;
class  ShortcutWidget;
class  ScrAction;
struct Keys;

class SCRIBUS_API StyleManager : public ScrPaletteBase, Ui::StyleManager
{
	Q_OBJECT
public:
	StyleManager(QWidget* parent = nullptr, const char *name = "StyleManager");
	~StyleManager();

	void addStyle(StyleItem *item);
	void refreshLists();

	void showAsNewParagraphStyle();
	void showAsNewCharacterStyle();
	void showAsNewLineStyle();
	void showAsEditParagraphStyle(const QString& name);
	void showAsEditCharacterStyle(const QString& name);
	void showAsEditLineStyle(const QString& name);
	void showAsEditObjectStyle(const QString& name);
	/**
	 * @brief Reload all style data from the document and rebuild the
	 * style view tree.
	 *
	 * Call this when styles have been modified outside the Style Manager
	 * (e.g. by the scripter or by preferences dialogs) and the UI needs
	 * to reflect the current state of the document's styles.
	 *
	 * This clears the style view, reloads each StyleItem from the
	 * document, and recreates the tree and associated shortcut actions.
	 */
	void reloadStyles();

	QMap<QString,Keys> keyMap();

public slots:
	void setDoc(ScribusDoc *doc);
	void languageChange();
	void unitChange();

signals:
	void closed();

protected:
	void changeEvent(QEvent *e) override;
	void hideEvent(QHideEvent *e) override;
	void closeEvent(QCloseEvent *e) override;
	void showEvent(QShowEvent *e) override;

private:
	QList<StyleItem*>   m_items;
	StyleItem          *m_item { nullptr };
	QGridLayout        *m_layout { nullptr };
	QTabWidget         *m_widget { nullptr };
	ShortcutWidget     *m_shortcutWidget { nullptr };
	QString             m_currentType;
	QMenu              *m_newPopup { nullptr };
	QMenu              *m_rightClickPopup { nullptr };

	QAction            *m_rcpNewId { nullptr };
	QAction            *m_rcpDeleteId { nullptr };
	QAction            *m_rcpEditId { nullptr };
	QAction            *m_rcpCloneId { nullptr };
	QAction            *m_rcpDefaultStylesFirstId { nullptr };
	QAction            *m_rcpNewSeparator { nullptr };
	ScrAction	*m_selectedStyleAction { nullptr };

	//! \brief Show each default style at the top of its list instead of in
	//! alphabetical order. Remembered between sessions.
	bool                m_defaultStylesFirst { true };
	bool                m_isEditMode { true };
	bool                m_isStoryEditMode { false };
	QPoint              m_editPosition;
	QString             m_rcStyle;
	QString             m_rcType;
	QString             m_exitEditModeOk;
	QString             m_enterEditModeOk;
	QString             m_doneText;
	QString             m_editText;

	ScribusDoc         *m_doc { nullptr };
	PrefsContext       *m_prefs { nullptr };

	QMap<QString, QPointer<ScrAction> > m_styleActions;

	static const int     NAME_COL     = 0;
	static const int     SHORTCUT_COL = 1;
	static const QString SEPARATOR;

	void insertShortcutPage(QTabWidget *twidget);

	bool nameIsUnique(const QString &name);

	/**
	 * @brief Counts how far @a style sits below the top of its family tree.
	 *
	 * A style that is not based on anything is at depth 0, a style based on that
	 * one is at depth 1, and so on. If a style names a parent that is not in the
	 * list at all, counting stops and the style is treated as a top level one.
	 * The list of styles already seen is there so that two styles naming each
	 * other as parent cannot send this in circles.
	 */
	static int styleDepth(const StyleName& style, const QHash<QString, QString>& parentOf);

	/**
	 * @brief Puts parent styles ahead of the styles based on them.
	 *
	 * The list arrives in alphabetical order. The style list shows each style
	 * underneath the one it is based on, so a parent has to be added to the tree
	 * before any of its children. Working through the list one generation at a
	 * time does that, and leaves the names alphabetical within each generation.
	 */
	static void sortStylesByDepth(QList<StyleName>& styles);

	/**
	 * @brief Moves the default style, if there is one, to the front of @a styles.
	 *
	 * A default style is never based on another style, so bringing it to the
	 * front cannot separate any other style from its parent.
	 */
	void moveDefaultStyleToTop(QList<StyleName>& styles) const;

	/**
	 * @brief Puts the "New ..." entry at the top of the right click menu.
	 *
	 * Replaces whatever New entry is there at the moment. An empty @a text asks
	 * for the submenu listing every style type, which is what suits a right
	 * click on empty space; any other text becomes a single entry for creating
	 * one particular type.
	 */
	void setNewMenuEntry(const QString& text);

	// will be used to map plural style name to it's singular
	QMap<QString, QString> m_styleClassesPS;
	QMap<QString, QString> m_styleClassesSP;

	// QPair.first will be the type name (null if nothing is selected or
	// if there are styles from more than one type in the selection)
	// QPair.second will be the selected styles
	QPair<QString, QStringList> namesFromSelection();
	void loadType(const QString &name);
	void addNewType(StyleItem *item, bool loadFromDoc = true);
	void createNewStyle(const QString &typeName, const QString &fromParent = QString());
	void reloadStyleView(bool loadFromDoc = true); // are the styles loaded from doc or from tmp cache
	bool shortcutExists(const QString &keys);
	void updateActionName(const QString &oldName, const QString &newName);
	/* QPair.first = type name and QPair.second = style name */
	void setSelection(const QList<QPair<QString, QString> > &selected);
	void setOkButtonText();

	void editStyleByName(int rcType, const QString &name);

	template<class ItemType> ItemType* item();

private slots:
	void slotOk();
	void slotApply();
	void slotDelete();
	/**
	 * @brief Find and remove all styles not in use by any document content.
	 *
	 * Scans all page items, master page items, frame items, and pattern
	 * items to determine which styles are actually referenced. Any style
	 * that is not referenced and is not a default style is offered for
	 * removal. Parent styles of used styles are always preserved.
	 *
	 * Operates through the Style Manager's normal edit/apply flow:
	 * enters edit mode, calls deleteStyles() on temp copies for each
	 * StyleItem, then commits via slotOk(). The user is shown a
	 * confirmation dialog before any styles are removed.
	 */
	void slotDeleteUnused();
	void slotImport();
	void slotEdit();
	void slotClone();
	void slotNew();
	//! \brief Open "New Style" widget from "New" GUI button.
	void slotNewPopup(QAction *);
	/*! \brief Overloaded slot for context QMenu handling.
	QActions inserted by QMenu::addAction() can handle triggered() related slots only
	*/
	void slotNewPopup();
	void slotScrap();
	void slotRightClick(const QPoint &point);
	void slotDoubleClick(QTreeWidgetItem * item, int column);

	void slotNameChanged(const QString& name);
	void slotShortcutChanged(const QString& shortcut);
	void slotSetupWidget();

	void slotApplyStyle(QTreeWidgetItem *item);
	void slotApplyStyle(QTreeWidgetItem *newitem, QTreeWidgetItem *olditem);
	void slotApplyStyle(QTreeWidgetItem *item, int column);

	void slotDocSelectionChanged();
	void slotDocStylesChanged();

	void slotToggleDefaultStylesFirst(bool checked);
	void slotDirty();
	void slotClean();

	void slotApplyStyle(const QString& keyString); // keyString == styleClass$$$$styleName
};

#endif
