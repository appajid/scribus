/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "cmdobjectstyleimport.h"
#include "cmdutil.h"

#include <utility>

#include <QRegularExpression>
#include <QSet>

#include "commonstrings.h"
#include "fileloader.h"
#include "pyesstring.h"
#include "scribus.h"
#include "scribuscore.h"
#include "scribusdoc.h"
#include "scribusstructs.h"
#include "styles/charstyle.h"
#include "styles/objectstyle.h"
#include "styles/paragraphstyle.h"
#include "ui/stylemanager.h"

namespace
{
QString uniqueImportedLineStyleName(const QString& requestedName, const QHash<QString, MultiLine>& destinationStyles)
{
	static const QRegularExpression suffixExpression(QStringLiteral("^(.*)\\s+\\((\\d+)\\)$"));
	const QRegularExpressionMatch match = suffixExpression.match(requestedName);
	QString prefix = requestedName;
	int suffix = 1;
	if (match.hasMatch())
	{
		prefix = match.captured(1);
		suffix = match.captured(2).toInt();
	}

	QString candidate;
	do
	{
		++suffix;
		candidate = QStringLiteral("%1 (%2)").arg(prefix).arg(suffix);
	}
	while (destinationStyles.contains(candidate));
	return candidate;
}
}

PyObject *scribus_importobjectstyles(PyObject* /* self */, PyObject* args, PyObject* keywords)
{
	char* keywordArgs[] = {
		const_cast<char*>("filename"),
		const_cast<char*>("styles"),
		const_cast<char*>("renameOnClash"),
		nullptr
	};
	PyESString fileName;
	PyObject* stylesObject = Py_None;
	int renameOnClash = 1;
	if (!PyArg_ParseTupleAndKeywords(args, keywords, "es|Op", keywordArgs,
		"utf-8", fileName.ptr(), &stylesObject, &renameOnClash))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	const QString sourcePath = QString::fromUtf8(fileName.c_str());
	if (sourcePath.isEmpty())
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("The source document path cannot be empty.", "python error").toUtf8().constData());
		return nullptr;
	}

	QStringList requestedStyles;
	if (stylesObject != Py_None)
	{
		if (PyUnicode_Check(stylesObject))
		{
			PyErr_SetString(PyExc_TypeError, QObject::tr("styles must be a sequence of strings, not a single string.", "python error").toUtf8().constData());
			return nullptr;
		}
		PyObject* sequence = PySequence_Fast(stylesObject,
			QObject::tr("styles must be a sequence of strings.", "python error").toUtf8().constData());
		if (!sequence)
			return nullptr;

		QSet<QString> seenStyles;
		const Py_ssize_t count = PySequence_Fast_GET_SIZE(sequence);
		for (Py_ssize_t index = 0; index < count; ++index)
		{
			PyObject* item = PySequence_Fast_GET_ITEM(sequence, index);
			if (!PyUnicode_Check(item))
			{
				Py_DECREF(sequence);
				PyErr_SetString(PyExc_TypeError, QObject::tr("styles must contain only strings.", "python error").toUtf8().constData());
				return nullptr;
			}
			const char* utf8Name = PyUnicode_AsUTF8(item);
			if (!utf8Name)
			{
				Py_DECREF(sequence);
				return nullptr;
			}
			const QString styleName = QString::fromUtf8(utf8Name);
			if (!styleName.isEmpty() && !seenStyles.contains(styleName))
			{
				seenStyles.insert(styleName);
				requestedStyles.append(styleName);
			}
		}
		Py_DECREF(sequence);
	}

	FileLoader sourceLoader(sourcePath);
	if (sourceLoader.testFile() == -1)
	{
		PyErr_SetString(PyExc_OSError, QObject::tr("The source document could not be read.", "python error").toUtf8().constData());
		return nullptr;
	}

	ScribusMainWindow* mainWindow = ScCore->primaryMainWindow();
	ScribusDoc* doc = mainWindow->doc;
	StyleSet<ParagraphStyle> unusedParagraphStyles;
	StyleSet<CharStyle> unusedCharacterStyles;
	QHash<QString, MultiLine> sourceLineStyles;
	StyleSet<ObjectStyle> sourceObjectStyles;
	doc->loadStylesFromFile(sourcePath, &unusedParagraphStyles, &unusedCharacterStyles,
		&sourceLineStyles, nullptr, nullptr, &sourceObjectStyles);

	if (stylesObject == Py_None)
	{
		for (int index = 0; index < sourceObjectStyles.count(); ++index)
		{
			const QString& styleName = sourceObjectStyles[index].name();
			if (!styleName.isEmpty())
				requestedStyles.append(styleName);
		}
	}
	else
	{
		for (const QString& styleName : std::as_const(requestedStyles))
		{
			if (!sourceObjectStyles.contains(styleName))
			{
				PyErr_SetString(NotFoundError, QObject::tr("Object style '%1' was not found in the source document.", "python error").arg(styleName).toUtf8().constData());
				return nullptr;
			}
		}
	}

	const ObjectStyleImportPlan plan = buildObjectStyleImportPlan(sourceObjectStyles, requestedStyles);
	QHash<QString, MultiLine> destinationLineStyles = doc->docLineStyles;
	ColorList destinationColors;
	destinationColors = doc->PageColors;
	QMap<QString, QString> importedLineStyleNames;
	QSet<QString> neededColors;
	for (const QString& lineStyleName : plan.lineStyleNames)
	{
		if (!sourceLineStyles.contains(lineStyleName))
		{
			if (!destinationLineStyles.contains(lineStyleName))
				importedLineStyleNames.insert(lineStyleName, QString());
			continue;
		}

		QString destinationName = lineStyleName;
		if (renameOnClash && destinationLineStyles.contains(destinationName))
			destinationName = uniqueImportedLineStyleName(destinationName, destinationLineStyles);
		const MultiLine importedLineStyle = sourceLineStyles.value(lineStyleName);
		destinationLineStyles.insert(destinationName, importedLineStyle);
		importedLineStyleNames.insert(lineStyleName, destinationName);
		for (const SingleLine& line : importedLineStyle)
		{
			if (!line.Color.isEmpty() && line.Color != CommonStrings::None)
				neededColors.insert(line.Color);
		}
	}
	for (const QString& colorName : plan.colorNames)
	{
		if (!colorName.isEmpty() && colorName != CommonStrings::None)
			neededColors.insert(colorName);
	}

	if (!neededColors.isEmpty())
	{
		ColorList sourceColors;
		if (sourceLoader.readColors(sourceColors))
		{
			for (const QString& colorName : std::as_const(neededColors))
			{
				if (!destinationColors.contains(colorName) && sourceColors.contains(colorName))
					destinationColors.insert(colorName, sourceColors[colorName]);
			}
		}
	}

	StyleSet<ObjectStyle> destinationStyles;
	destinationStyles.redefine(doc->objectStyles(), true);
	const QMap<QString, QString> importedNames = importObjectStyles(sourceObjectStyles, plan.styleNames,
		destinationStyles, renameOnClash != 0, importedLineStyleNames);
	if (!importedNames.isEmpty())
	{
		doc->applyObjectStyleImport(destinationStyles, destinationColors, destinationLineStyles);
	}

	PyObject* result = PyDict_New();
	if (!result)
		return nullptr;
	for (auto it = importedNames.constBegin(); it != importedNames.constEnd(); ++it)
	{
		PyObject* sourceName = PyUnicode_FromString(it.key().toUtf8().constData());
		PyObject* destinationName = PyUnicode_FromString(it.value().toUtf8().constData());
		if (!sourceName || !destinationName || PyDict_SetItem(result, sourceName, destinationName) != 0)
		{
			Py_XDECREF(sourceName);
			Py_XDECREF(destinationName);
			Py_DECREF(result);
			return nullptr;
		}
		Py_DECREF(sourceName);
		Py_DECREF(destinationName);
	}
	return result;
}
