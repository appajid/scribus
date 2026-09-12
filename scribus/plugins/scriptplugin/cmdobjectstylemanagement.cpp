/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "cmdobjectstylemanagement.h"
#include "cmdutil.h"

#include "pyesstring.h"
#include "scribus.h"
#include "scribuscore.h"
#include "scribusdoc.h"
#include "styles/objectstyle.h"
#include "ui/stylemanager.h"

namespace
{
void appendObjectStyle(StyleSet<ObjectStyle>& destination, const ObjectStyle& source)
{
	ObjectStyle style(source);
	style.setContext(nullptr);
	ObjectStyle* storedStyle = destination.create(style);
	if (style.isDefaultStyle())
		destination.makeDefault(storedStyle);
}

void refreshObjectStyleUi(ScribusMainWindow* mainWindow, ScribusDoc* doc)
{
	if (mainWindow->styleMgr())
		mainWindow->styleMgr()->setDoc(doc);
}
}

PyObject *scribus_renameobjectstyle(PyObject* /* self */, PyObject* args)
{
	PyESString oldName;
	PyESString newName;
	if (!PyArg_ParseTuple(args, "eses", "utf-8", oldName.ptr(), "utf-8", newName.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	const QString sourceName = QString::fromUtf8(oldName.c_str());
	const QString destinationName = QString::fromUtf8(newName.c_str());
	if (sourceName.isEmpty() || destinationName.isEmpty())
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("Object Style names cannot be empty.", "python error").toUtf8().constData());
		return nullptr;
	}

	ScribusMainWindow* mainWindow = ScCore->primaryMainWindow();
	ScribusDoc* doc = mainWindow->doc;
	if (!doc->objectStyles().contains(sourceName))
	{
		PyErr_SetString(NotFoundError, QObject::tr("Object Style not found.", "python error").toUtf8().constData());
		return nullptr;
	}
	if (doc->objectStyle(sourceName).isDefaultStyle())
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("The default Object Style cannot be renamed.", "python error").toUtf8().constData());
		return nullptr;
	}
	if (sourceName == destinationName)
		Py_RETURN_NONE;
	if (doc->objectStyles().contains(destinationName))
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("An Object Style with the new name already exists.", "python error").toUtf8().constData());
		return nullptr;
	}

	StyleSet<ObjectStyle> renamedStyles;
	for (int i = 0; i < doc->objectStyles().count(); ++i)
	{
		ObjectStyle style(doc->objectStyles()[i]);
		if (style.name() == sourceName)
			style.setName(destinationName);
		if (style.parent() == sourceName)
			style.setParent(destinationName);
		appendObjectStyle(renamedStyles, style);
	}

	doc->applyObjectStyleChanges(renamedStyles, { { sourceName, destinationName } });
	refreshObjectStyleUi(mainWindow, doc);
	Py_RETURN_NONE;
}

PyObject *scribus_deleteobjectstyle(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	PyESString replacement;
	if (!PyArg_ParseTuple(args, "es|es", "utf-8", name.ptr(), "utf-8", replacement.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	const QString styleName = QString::fromUtf8(name.c_str());
	const QString replacementName = QString::fromUtf8(replacement.c_str());
	ScribusMainWindow* mainWindow = ScCore->primaryMainWindow();
	ScribusDoc* doc = mainWindow->doc;
	if (!doc->objectStyles().contains(styleName))
	{
		PyErr_SetString(NotFoundError, QObject::tr("Object Style not found.", "python error").toUtf8().constData());
		return nullptr;
	}
	if (doc->objectStyle(styleName).isDefaultStyle())
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("The default Object Style cannot be deleted.", "python error").toUtf8().constData());
		return nullptr;
	}
	if (replacementName == styleName)
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("A deleted Object Style cannot replace itself.", "python error").toUtf8().constData());
		return nullptr;
	}
	if (!replacementName.isEmpty() && !doc->objectStyles().contains(replacementName))
	{
		PyErr_SetString(NotFoundError, QObject::tr("Replacement Object Style not found.", "python error").toUtf8().constData());
		return nullptr;
	}

	StyleSet<ObjectStyle> remainingStyles;
	for (int i = 0; i < doc->objectStyles().count(); ++i)
	{
		const ObjectStyle& currentStyle = doc->objectStyles()[i];
		if (currentStyle.name() == styleName)
			continue;
		ObjectStyle style(currentStyle);
		if (style.parent() == styleName)
		{
			QString newParent = replacementName;
			if (!newParent.isEmpty() && !currentStyle.canInherit(newParent))
				newParent.clear();
			style.setParent(newParent);
		}
		appendObjectStyle(remainingStyles, style);
	}

	doc->applyObjectStyleChanges(remainingStyles, { { styleName, replacementName } });
	refreshObjectStyleUi(mainWindow, doc);
	Py_RETURN_NONE;
}
