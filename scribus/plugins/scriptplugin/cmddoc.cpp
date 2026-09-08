/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#include "cmddoc.h"
#include "cmdutil.h"
#include "documentchecker.h"
#include "documentinformation.h"
#include "dynamicvariable.h"
#include "marks.h"
#include "pageitem.h"
#include "pyesstring.h"
#include "scribuscore.h"
#include "scribusdoc.h"
#include "scribusview.h"
#include "util.h"
#include "units.h"

#include <QApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>

namespace
{
QString dynamicVariableId(ScribusDoc* doc, const QString& identifier)
{
	if (DynamicVariableResolver::isBuiltInId(identifier))
		return DynamicVariableResolver::isKnownBuiltInId(identifier) ? identifier : QString();
	for (const DynamicVariable& variable : DynamicVariableResolver::builtInVariables())
	{
		if (variable.type == identifier)
			return variable.id;
	}
	if (doc->dynamicVariable(identifier))
		return identifier;
	return doc->dynamicVariableIdByName(identifier);
}

QString userDynamicVariableId(ScribusDoc* doc, const QString& identifier)
{
	const QString id = dynamicVariableId(doc, identifier);
	return (id.isEmpty() || DynamicVariableResolver::isBuiltInId(id)) ? QString() : id;
}

PyObject* dynamicVariableNotFound(const QString& identifier)
{
	PyErr_SetString(NotFoundError, QObject::tr("Dynamic variable '%1' was not found.", "python error").arg(identifier).toUtf8().constData());
	return nullptr;
}
}

PyObject *scribus_newdocument(PyObject* /* self */, PyObject* args)
{
	double topMargin, bottomMargin, leftMargin, rightMargin;
	double pageWidth, pageHeight;
	int orientation, firstPageNr, unit, pagesType, firstPageOrder, numPages;
	int bindingDirection = 0;

	PyObject *p, *m;

	if ((!PyArg_ParseTuple(args, "OOiiiiii|i", &p, &m, &orientation,
											&firstPageNr, &unit,
											&pagesType,
											&firstPageOrder,
											&numPages, &bindingDirection)) ||
						(!PyArg_ParseTuple(p, "dd", &pageWidth, &pageHeight)) ||
						(!PyArg_ParseTuple(m, "dddd", &leftMargin, &rightMargin,
												&topMargin, &bottomMargin)))
		return nullptr;
	if (numPages <= 0)
		numPages = 1;
	if (pagesType == 0)
	{
		firstPageOrder = 0;
	}
	if (pagesType < firstPageOrder)
	{
		PyErr_SetString(ScribusException, QObject::tr("firstPageOrder is bigger than allowed.","python error").toUtf8().constData());
		return nullptr;
	}


	pageWidth  = value2pts(pageWidth, unit);
	pageHeight = value2pts(pageHeight, unit);
	if (orientation == 1)
	{
		double x = pageWidth;
		pageWidth = pageHeight;
		pageHeight = x;
	}
	leftMargin   = value2pts(leftMargin, unit);
	rightMargin  = value2pts(rightMargin, unit);
	topMargin    = value2pts(topMargin, unit);
	bottomMargin = value2pts(bottomMargin, unit);

	bool ret = ScCore->primaryMainWindow()->doFileNew(pageWidth, pageHeight,
								topMargin, leftMargin, rightMargin, bottomMargin,
								// autoframes. It's disabled in python
								// columnDistance, numberCols, autoframes,
								0, 1, false,
								pagesType, unit, firstPageOrder,
								orientation, firstPageNr, QSizeF(), true,
								numPages, true, 0, bindingDirection);
	ScCore->primaryMainWindow()->doc->setPageSetFirstPage(pagesType, firstPageOrder);

	return PyLong_FromLong(static_cast<long>(ret));
}

PyObject *scribus_newdoc(PyObject* /* self */, PyObject* args)
{
	qDebug("WARNING: newDoc() procedure is obsolete, it will be removed in a forthcoming release. Use newDocument() instead.");
	double b, h, lr, tpr, btr, rr, ebr;
	int unit, ds, fsl, fNr, ori;
	PyObject *p, *m;
	if ((!PyArg_ParseTuple(args, "OOiiiii", &p, &m, &ori, &fNr, &unit, &ds, &fsl)) ||
	        (!PyArg_ParseTuple(p, "dd", &b, &h)) ||
	        (!PyArg_ParseTuple(m, "dddd", &lr, &rr, &tpr, &btr)))
		return nullptr;
	b = value2pts(b, unit);
	h = value2pts(h, unit);
	if (ori == 1)
	{
		ebr = b;
		b = h;
		h = ebr;
	}
	/*! \todo Obsolete! In the case of no facing pages use only firstpageleft
	scripter is not new-page-size ready.
	What is it: don't allow to use wrong FSL constant in the case of
	onesided document. */
	if (ds != 1 && fsl > 0)
		fsl = 0;
	// end of hack

	tpr = value2pts(tpr, unit);
	lr  = value2pts(lr, unit);
	rr  = value2pts(rr, unit);
	btr = value2pts(btr, unit);
	bool ret = ScCore->primaryMainWindow()->doFileNew(b, h, tpr, lr, rr, btr, 0, 1, false, ds, unit, fsl, ori, fNr, QSizeF(), true);
	//	qApp->processEvents();
	return PyLong_FromLong(static_cast<long>(ret));
}

PyObject *scribus_getbleeds(PyObject */* self */, PyObject* /*args*/)
{
	if (!checkHaveDocument())
		return nullptr;

	const MarginStruct& bleeds = ScCore->primaryMainWindow()->doc->bleedsVal();

	return Py_BuildValue("(dddd)",
		PointToValue(bleeds.left()),
		PointToValue(bleeds.right()),
		PointToValue(bleeds.top()),
		PointToValue(bleeds.bottom()));
}

PyObject *scribus_setbleeds(PyObject */* self */, PyObject *args)
{
	double lr, tpr, btr, rr;
	if (!PyArg_ParseTuple(args, "dddd", &lr, &rr, &tpr, &btr))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	MarginStruct bleeds(ValueToPoint(tpr), ValueToPoint(lr), ValueToPoint(btr), ValueToPoint(rr));

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	ScribusView* currentView = ScCore->primaryMainWindow()->view;
	currentDoc->setBleeds(bleeds);
	currentView->reformPages();
	currentDoc->setModified(true);
	currentView->DrawNew();
	Py_RETURN_NONE;
}

PyObject* scribus_getmargins(PyObject*/* self */, PyObject* /*args*/)
{
	if (!checkHaveDocument())
		return nullptr;

	const MarginStruct& margins = ScCore->primaryMainWindow()->doc->marginsVal();

	return Py_BuildValue("(dddd)",
		PointToValue(margins.left()),
		PointToValue(margins.right()),
		PointToValue(margins.top()),
		PointToValue(margins.bottom()));
}

PyObject *scribus_setmargins(PyObject* /* self */, PyObject* args)
{
	double lr, tpr, btr, rr;
	if (!PyArg_ParseTuple(args, "dddd", &lr, &rr, &tpr, &btr))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	MarginStruct margins(ValueToPoint(tpr), ValueToPoint(lr), ValueToPoint(btr), ValueToPoint(rr));

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	ScribusView* currentView = ScCore->primaryMainWindow()->view;
	currentDoc->setMargins(margins);
	currentView->reformPages();
	currentDoc->setModified(true);
	currentView->GotoPage(currentDoc->currentPageNumber());
	currentView->DrawNew();

	Py_RETURN_NONE;
}

PyObject* scribus_getbaseline(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;

	const GuidesPrefs& guides = ScCore->primaryMainWindow()->doc->guidesPrefs();

	return Py_BuildValue("(dd)",
		PointToValue(guides.valueBaselineGrid),
		PointToValue(guides.offsetBaselineGrid));
}

PyObject *scribus_setbaseline(PyObject* /* self */, PyObject* args)
{
	double grid, offset;
	if (!PyArg_ParseTuple(args, "dd", &grid, &offset))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	ScribusView* currentView = ScCore->primaryMainWindow()->view;
	currentDoc->guidesPrefs().valueBaselineGrid = ValueToPoint(grid);
	currentDoc->guidesPrefs().offsetBaselineGrid = ValueToPoint(offset);
	//currentView->reformPages();
	currentDoc->setModified(true);
	//currentView->GotoPage(currentDoc->currentPageNumber());
	currentView->DrawNew();

	Py_RETURN_NONE;
}

PyObject *scribus_closedoc(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	ScCore->primaryMainWindow()->doc->setModified(false);
	bool ret = ScCore->primaryMainWindow()->slotFileClose();
	QApplication::processEvents();
	return PyLong_FromLong(static_cast<long>(ret));
}

PyObject *scribus_havedoc(PyObject* /* self */)
{
	return PyLong_FromLong(static_cast<long>(ScCore->primaryMainWindow()->HaveDoc));
}

PyObject *scribus_opendoc(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	if (!PyArg_ParseTuple(args, "es", "utf-8", name.ptr()))
		return nullptr;
	bool ret = ScCore->primaryMainWindow()->loadDoc(QString::fromUtf8(name.c_str()));
	if (!ret)
	{
		PyErr_SetString(ScribusException, QObject::tr("Failed to open document: %1","python error").arg(name.c_str()).toUtf8().constData());
		return nullptr;
	}
	return PyBool_FromLong(static_cast<long>(true));
//	Py_INCREF(Py_True); // compatibility: return true, not none, on success
//	return Py_True;
//	Py_RETURN_TRUE;
}

PyObject *scribus_savedoc(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	ScCore->primaryMainWindow()->slotFileSave();
	Py_RETURN_NONE;
}

PyObject *scribus_revertdoc(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	ScCore->primaryMainWindow()->slotFileRevert();
	Py_RETURN_NONE;
}

PyObject *scribus_getdocname(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	if (! ScCore->primaryMainWindow()->doc->hasName)
	{
		return PyUnicode_FromString("");
	}
	return PyUnicode_FromString(ScCore->primaryMainWindow()->doc->documentFileName().toUtf8());
}

PyObject *scribus_savedocas(PyObject* /* self */, PyObject* args)
{
	PyESString fileName;
	if (!PyArg_ParseTuple(args, "es", "utf-8", fileName.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	bool ret = ScCore->primaryMainWindow()->DoFileSave(QString::fromUtf8(fileName.c_str()));
	if (!ret)
	{
		PyErr_SetString(ScribusException, QObject::tr("Failed to save document.","python error").toUtf8().constData());
		return nullptr;
	}
	return PyBool_FromLong(static_cast<long>(true));
//	Py_INCREF(Py_True); // compatibility: return true, not none, on success
//	return Py_True;
//	Py_RETURN_TRUE;
}

PyObject *scribus_setinfo(PyObject* /* self */, PyObject* args)
{
	char *Author;
	char *Title;
	char *Desc;
	// z means string, but None becomes a nullptr value. QString()
	// will correctly handle nullptr.
	if (!PyArg_ParseTuple(args, "zzz", &Author, &Title, &Desc))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	DocumentInformation& docInfo = ScCore->primaryMainWindow()->doc->documentInfo();
	docInfo.setAuthor(QString::fromUtf8(Author));
	docInfo.setTitle(QString::fromUtf8(Title));
	docInfo.setComments(QString::fromUtf8(Desc));
	ScCore->primaryMainWindow()->slotDocCh();

	Py_RETURN_NONE;
}

PyObject *scribus_getinfo(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	if (! ScCore->primaryMainWindow()->doc->hasName)
	{
		return PyUnicode_FromString("");
	}

	const DocumentInformation& docInfo = ScCore->primaryMainWindow()->doc->documentInfo();
	return Py_BuildValue("(sss)",
				docInfo.author().toUtf8().data(),
				docInfo.title().toUtf8().data(),
				docInfo.comments().toUtf8().data());
}

PyObject *scribus_setunit(PyObject* /* self */, PyObject* args)
{
	int e;
	if (!PyArg_ParseTuple(args, "i", &e))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	if ((e < UNITMIN) || (e > UNITMAX))
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("Unit out of range. Use one of the scribus.UNIT_* constants.","python error").toUtf8().constData());
		return nullptr;
	}
	ScCore->primaryMainWindow()->slotChangeUnit(e);

	Py_RETURN_NONE;
}

PyObject *scribus_getunit(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	return PyLong_FromLong(static_cast<long>(ScCore->primaryMainWindow()->doc->unitIndex()));
}

PyObject *scribus_pointstodocunit(PyObject* /* self */, PyObject *args)
{
    double points;
    if (!PyArg_ParseTuple(args, "d", &points))
        return nullptr;
    if (!checkHaveDocument())
        return nullptr;

    return Py_BuildValue("d", PointToValue(points));
}

PyObject *scribus_docunittopoints(PyObject* /* self */, PyObject *args)
{
    double value;
    if (!PyArg_ParseTuple(args, "d", &value))
        return nullptr;
    if (!checkHaveDocument())
        return nullptr;

    return Py_BuildValue("d", ValueToPoint(value));
}

PyObject *scribus_stringvaluetopoints(PyObject* /* self */, PyObject *args)
{
    PyESString strValue;
    if (!PyArg_ParseTuple(args, "es", "utf-8", strValue.ptr()))
        return nullptr;

    QString qv = QString::fromUtf8(strValue.c_str());

    int uIdx = unitIndexFromString(qv);
    double value = unitValueFromString(qv);
    double points = value / unitGetRatioFromIndex(uIdx);

    return Py_BuildValue("d", points);
}

PyObject *scribus_loadstylesfromfile(PyObject* /* self */, PyObject *args)
{
	PyESString fileName;
	if (!PyArg_ParseTuple(args, "es", "utf-8", fileName.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	ScCore->primaryMainWindow()->doc->loadStylesFromFile(QString::fromUtf8(fileName.c_str()));

	Py_RETURN_NONE;
}

PyObject *scribus_setdoctype(PyObject* /* self */, PyObject* args)
{
	int fp, fsl;
	if (!PyArg_ParseTuple(args, "ii", &fp, &fsl))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	ScribusView* currentView = ScCore->primaryMainWindow()->view;

	if (currentDoc->pagePositioning() == fp)
		currentDoc->setPageSetFirstPage(currentDoc->pagePositioning(), fsl);
	currentView->reformPages();
	currentView->GotoPage(currentDoc->currentPageNumber()); // is this needed?
	currentView->DrawNew();   // is this needed?
	//CB TODO ScCore->primaryMainWindow()->pagePalette->RebuildPage(); // is this needed?
	ScCore->primaryMainWindow()->slotDocCh();

	Py_RETURN_NONE;
}

PyObject *scribus_closemasterpage(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	ScCore->primaryMainWindow()->view->hideMasterPage();

	Py_RETURN_NONE;
}

PyObject *scribus_masterpagenames(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	const ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;

	PyObject* names = PyList_New(currentDoc->MasterPages.count());
	QMap<QString,int>::const_iterator it(currentDoc->MasterNames.constBegin());
	QMap<QString,int>::const_iterator itEnd(currentDoc->MasterNames.constEnd());
	int n = 0;
	for ( ; it != itEnd; ++it )
	{
		PyList_SET_ITEM(names, n++, PyUnicode_FromString(it.key().toUtf8().data()) );
	}
	return names;
}

PyObject *scribus_editmasterpage(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	if (!PyArg_ParseTuple(args, "es", "utf-8", name.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	const QString masterPageName(name.c_str());
	const QMap<QString,int>& masterNames(ScCore->primaryMainWindow()->doc->MasterNames);
	const QMap<QString,int>::const_iterator it(masterNames.find(masterPageName));
	if ( it == masterNames.constEnd() )
	{
		PyErr_SetString(PyExc_ValueError, "Master page not found");
		return nullptr;
	}
	ScCore->primaryMainWindow()->view->showMasterPage(*it);

	Py_RETURN_NONE;
}

PyObject* scribus_createmasterpage(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	if (!PyArg_ParseTuple(args, "es", "utf-8", name.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	const QString masterPageName(name.c_str());

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	if (currentDoc->MasterNames.contains(masterPageName))
	{
		PyErr_SetString(PyExc_ValueError, "Master page already exists");
		return nullptr;
	}
	currentDoc->addMasterPage(currentDoc->MasterPages.count(), masterPageName);

	Py_RETURN_NONE;
}

PyObject* scribus_createfacingmasterpair(PyObject* /* self */, PyObject* args)
{
	PyESString leftNameUtf8;
	PyESString rightNameUtf8;
	if (!PyArg_ParseTuple(args, "eses", "utf-8", leftNameUtf8.ptr(), "utf-8", rightNameUtf8.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	const QString leftName = QString::fromUtf8(leftNameUtf8.c_str()).trimmed();
	const QString rightName = QString::fromUtf8(rightNameUtf8.c_str()).trimmed();
	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	if (currentDoc->pageSets()[currentDoc->pagePositioning()].Columns != 2)
	{
		PyErr_SetString(PyExc_ValueError, "Facing master pairs require a facing-page document");
		return nullptr;
	}
	if (leftName.isEmpty() || rightName.isEmpty() || leftName == rightName)
	{
		PyErr_SetString(PyExc_ValueError, "Left and right master page names must be different and non-empty");
		return nullptr;
	}
	if (currentDoc->MasterNames.contains(leftName) || currentDoc->MasterNames.contains(rightName))
	{
		PyErr_SetString(PyExc_ValueError, "A master page with one of these names already exists");
		return nullptr;
	}
	if (!currentDoc->addMasterPagePair(leftName, rightName))
	{
		PyErr_SetString(PyExc_RuntimeError, "Could not create the facing master pair");
		return nullptr;
	}

	PyObject* leftNameObject = PyUnicode_FromString(leftName.toUtf8().constData());
	PyObject* rightNameObject = PyUnicode_FromString(rightName.toUtf8().constData());
	return Py_BuildValue("(NN)", leftNameObject, rightNameObject);
}

PyObject* scribus_deletemasterpage(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	if (!PyArg_ParseTuple(args, "es", "utf-8", name.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	const QString masterPageName(name.c_str());

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	if (!currentDoc->MasterNames.contains(masterPageName))
	{
		PyErr_SetString(PyExc_ValueError, "Master page does not exist");
		return nullptr;
	}
	if (masterPageName == "Normal")
	{
		PyErr_SetString(PyExc_ValueError, "Can not delete the Normal master page");
		return nullptr;
	}
	bool oldMode = currentDoc->masterPageMode();
	currentDoc->setMasterPageMode(true);
	ScCore->primaryMainWindow()->deletePage2(currentDoc->MasterNames[masterPageName]);
	currentDoc->setMasterPageMode(oldMode);

	Py_RETURN_NONE;
}

PyObject *scribus_getmasterpage(PyObject* /* self */, PyObject* args)
{
	int e;
	if (!PyArg_ParseTuple(args, "i", &e))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	e--;

	const ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	if ((e < 0) || (e > static_cast<int>(currentDoc->Pages->count())-1))
	{
		PyErr_SetString(PyExc_IndexError, QObject::tr("Page number out of range: '%1'.","python error").arg(e+1).toUtf8().constData());
		return nullptr;
	}
	return PyUnicode_FromString(currentDoc->DocPages.at(e)->masterPageName().toUtf8());
}

PyObject* scribus_applymasterpage(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	int page = 0;
	if (!PyArg_ParseTuple(args, "esi", "utf-8", name.ptr(), &page))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	const QString masterPageName(name.c_str());

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	if (!currentDoc->MasterNames.contains(masterPageName))
	{
		PyErr_SetString(PyExc_ValueError, QObject::tr("Master page does not exist: '%1'","python error").arg(masterPageName).toUtf8().constData());
		return nullptr;
	}
	if ((page < 1) || (page > static_cast<int>(currentDoc->Pages->count())))
	{
		PyErr_SetString(PyExc_IndexError, QObject::tr("Page number out of range: %1.","python error").arg(page).toUtf8().constData());
		return nullptr;
	}

	if (!currentDoc->applyMasterPage(masterPageName, page-1))
	{
		PyErr_SetString(ScribusException, QObject::tr("Failed to apply masterpage '%1' on page: %2","python error").arg(masterPageName).arg(page).toUtf8().constData());
		return nullptr;
	}
	Py_RETURN_NONE;
}

PyObject* scribus_exportdocumentcheck(PyObject* /* self */, PyObject* args, PyObject* kw)
{
	PyESString targetFilenameArg;
	PyESString checkProfileNameArg;
	bool showNonPrintingLayerErrors = false;
	char *kwargs[] = {const_cast<char*>("jsonFilename"), const_cast<char*>("checkProfileName"),
		const_cast<char*>("nonPrintingLayers"), nullptr};
	if (!PyArg_ParseTupleAndKeywords(args, kw, "|es$esp", kwargs,
			"utf-8", targetFilenameArg.ptr(), "utf-8", checkProfileNameArg.ptr(),
			&showNonPrintingLayerErrors))
		return nullptr;

	if (!checkHaveDocument())
		return nullptr;
	const QString targetFileName(targetFilenameArg.c_str());
	const QString checkProfileName(checkProfileNameArg.c_str());

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;

	if (checkProfileName.isEmpty())
		DocumentChecker::checkDocument(currentDoc);
	else
		DocumentChecker::checkDocument(currentDoc, checkProfileName);

	// "Standard" Errors
	// (Taken from PreflightError in scribusstruct.h)
	QMap<PreflightError, QString> errorsList = {
		{PreflightError::MissingGlyph, "MissingGlyph"},
		{PreflightError::TextOverflow, "TextOverflow"},
		{PreflightError::ObjectNotOnPage, "ObjectNotOnPage"},
		{PreflightError::MissingImage, "MissingImage"},
		{PreflightError::ImageDPITooLow, "ImageDPITooLow"},
		{PreflightError::Transparency, "Transparency"},
		{PreflightError::PDFAnnotField, "PDFAnnotField"},
		{PreflightError::PlacedPDF, "PlacedPDF"},
		{PreflightError::ImageDPITooHigh, "ImageDPITooHigh"},
		{PreflightError::ImageIsGIF, "ImageIsGIF"},
		{PreflightError::BlendMode, "BlendMode"},
		{PreflightError::WrongFontInAnnotation, "WrongFontInAnnotation"},
		{PreflightError::NotCMYKOrSpot, "NotCMYKOrSpot"},
		{PreflightError::DeviceColorsAndOutputIntent, "DeviceColorsAndOutputIntent"},
		{PreflightError::FontNotEmbedded, "FontNotEmbedded"},
		{PreflightError::EmbeddedFontIsOpenType, "EmbeddedFontIsOpenType"},
		{PreflightError::OffConflictLayers, "OffConflictLayers"},
		{PreflightError::PartFilledImageFrame, "PartFilledImageFrame"},
		{PreflightError::MarksChanged, "MarksChanged"},
		{PreflightError::AppliedMasterDifferentSide, "AppliedMasterDifferentSide"},
		{PreflightError::EmptyTextFrame, "EmptyTextFrame"},
		{PreflightError::ImageHasProgressiveEncoding, "ImageHasProgressiveEncoding"},
		{PreflightError::MissingStyle, "MissingStyle"},
		{PreflightError::BrokenCrossReference, "BrokenCrossReference"},
	};
	// Custom Errors
	// "DocumentModifiedAfterMarksUpdate"

	QJsonObject json;

	if (currentDoc->notesChanged())
	{
		json["marks"] = QJsonObject{{"", "DocumentModifiedAfterMarksUpdate"}};
	}
	else
	{
		json["marks"] = QJsonObject{};
	}

	QJsonArray jsonLayers;
	for (const auto& [layerId, layerErrors]: currentDoc->docLayerErrors.asKeyValueRange())
	{
		for (const auto& [key, errorLevel]: layerErrors.asKeyValueRange())
		{
			jsonLayers.push_back(QJsonObject{{"layer", currentDoc->layerName(layerId)}, {"error", errorsList.value(key)}});
		}
	}
	json["layers"] = jsonLayers;

	QMap<int, QVector<QPair<QString, QString>>> pagesWithErrors;

	for (auto [pageItem, itemError]: currentDoc->masterItemErrors.asKeyValueRange())
	{
		if (!showNonPrintingLayerErrors && !currentDoc->layerPrintable(pageItem->m_layerID))
			continue;
		const int pageNumber = pageItem->OwnPage;
		for (auto [errorCode, errorLevel]: itemError.asKeyValueRange())
			pagesWithErrors[pageNumber].push_back({pageItem->itemName(), errorsList.value(errorCode)});
	}

	QJsonObject jsonMasterPages;
	for (auto [pageNumber, value]: pagesWithErrors.asKeyValueRange())
	{
		if (pageNumber < 0 || pageNumber >= currentDoc->MasterPages.count())
			continue;
		QJsonArray pageItems;
		for (const auto& [item, error]: value)
		{
			pageItems.push_back(QJsonObject{{{"item", item}, {"error", error}}});
		}
		jsonMasterPages[currentDoc->MasterPages.at(pageNumber)->pageName()] = pageItems;
	}
	json["masterPages"] = jsonMasterPages;

	pagesWithErrors.clear();

	for (auto [pageNumber, pageErrors]: currentDoc->pageErrors.asKeyValueRange())
	{
		pagesWithErrors[pageNumber] = {};

		for (auto [errorCode, value]: pageErrors.asKeyValueRange())
		{
			pagesWithErrors[pageNumber].push_back({"", errorsList.value(errorCode)});
		}
	}
	QJsonArray jsonFreeItems;
	for (auto [pageItem, itemErrors]: currentDoc->docItemErrors.asKeyValueRange())
	{
		if (!showNonPrintingLayerErrors && !currentDoc->layerPrintable(pageItem->m_layerID))
			continue;
		if (pageItem->OwnPage == -1)
		{
			jsonFreeItems.push_back(pageItem->itemName());
			continue;
		}
		for (auto [errorCode, errorLevel]: itemErrors.asKeyValueRange())
			pagesWithErrors[pageItem->OwnPage].push_back({pageItem->itemName(), errorsList.value(errorCode)});
	}

	QJsonObject jsonPages;
	for (auto [pageNumber, value]: pagesWithErrors.asKeyValueRange())
	{
		QJsonArray pageItems;
		for (const auto& [item, error]: value)
		{
			pageItems.push_back(QJsonObject{{{"item", item}, {"error", error}}});
		}
		jsonPages[QString::number(pageNumber + 1)] = pageItems;
	}
	json["pages"] = jsonPages;

	QJsonObject jsonStyles;
	for (auto [styleName, styleErrors] : currentDoc->docStyleErrors.asKeyValueRange())
	{
		QJsonArray styleErrorList;
		for (auto [errorCode, value] : styleErrors.asKeyValueRange())
			styleErrorList.push_back(errorsList.value(errorCode));
		jsonStyles[styleName] = styleErrorList;
	}
	json["styles"] = jsonStyles;

	json["freeItems"] = jsonFreeItems;

	if (targetFileName.isEmpty())
		return PyUnicode_FromString(QJsonDocument(json).toJson().constData());

	QFile saveFile(targetFileName);
	if (!saveFile.open(QIODevice::WriteOnly))
	{
		PyErr_SetString(ScribusException, QObject::tr("Failed to open the file '%1' for writing","python error").arg(targetFileName).toUtf8().constData());
		return nullptr;
	}
	saveFile.write(QJsonDocument(json).toJson());

	Py_RETURN_NONE;
}

/*! HACK: this removes "warning: 'blah' defined but not used" compiler warnings
with header files structure untouched (docstrings are kept near declarations)
PV */
void cmddocdocwarnings()
{
	QStringList s;
	s << scribus_applymasterpage__doc__
	  << scribus_closedoc__doc__
	  << scribus_closemasterpage__doc__
	  << scribus_createmasterpage__doc__
	  << scribus_deletemasterpage__doc__
	  << scribus_editmasterpage__doc__
	  << scribus_getbaseline__doc__ 
	  << scribus_getbleeds__doc__ 
	  << scribus_getdocname__doc__
	  << scribus_getinfo__doc__
	  << scribus_getmargins__doc__
	  << scribus_getmasterpage__doc__
	  << scribus_getunit__doc__ 
	  << scribus_havedoc__doc__
	  << scribus_loadstylesfromfile__doc__
	  << scribus_masterpagenames__doc__ 
	  << scribus_newdoc__doc__ 
	  << scribus_newdocument__doc__
	  << scribus_opendoc__doc__
	  << scribus_revertdoc__doc__
	  << scribus_savedoc__doc__
	  << scribus_savedocas__doc__
	  << scribus_setbaseline__doc__
	  << scribus_setbleeds__doc__
	  << scribus_setdoctype__doc__ 
	  << scribus_setinfo__doc__
	  << scribus_setmargins__doc__
	  << scribus_setunit__doc__;
}

PyObject *scribus_getrtl(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	return PyBool_FromLong(static_cast<long>(ScCore->primaryMainWindow()->doc->isRTL()));
}

PyObject *scribus_setrtl(PyObject* /* self */, PyObject* args)
{
	int rtl = 0;
	if (!PyArg_ParseTuple(args, "p", &rtl))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	currentDoc->setRTL(rtl != 0);
	currentDoc->setModified(true);
	Py_RETURN_NONE;
}

PyObject *scribus_createcrossreferencetarget(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	PyESString objectName;
	int position = -1;
	if (!PyArg_ParseTuple(args, "es|esi", "utf-8", name.ptr(), "utf-8", objectName.ptr(), &position))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	PageItem* item = GetUniqueItem(QString::fromUtf8(objectName.c_str()));
	if (!item)
		return nullptr;
	if (!item->isTextFrame())
	{
		PyErr_SetString(WrongFrameTypeError, QObject::tr("Cannot insert a cross-reference target into a non-text frame.", "python error").toUtf8().constData());
		return nullptr;
	}
	if (position < -1 || position > item->itemText.length())
	{
		PyErr_SetString(PyExc_IndexError, QObject::tr("Insert index out of bounds.", "python error").toUtf8().constData());
		return nullptr;
	}

	const QString targetName = QString::fromUtf8(name.c_str()).trimmed();
	if (targetName.isEmpty() || currentDoc->crossReferenceTarget(targetName))
	{
		PyErr_SetString(NameExistsError, QObject::tr("A cross-reference target named '%1' already exists, or the name is empty.", "python error").arg(targetName).toUtf8().constData());
		return nullptr;
	}
	Mark* target = currentDoc->insertCrossReferenceTarget(targetName, item, position);
	if (!target)
	{
		PyErr_SetString(ScribusException, QObject::tr("The cross-reference target could not be inserted.", "python error").toUtf8().constData());
		return nullptr;
	}
	return PyUnicode_FromString(target->label.toUtf8().constData());
}

PyObject *scribus_insertcrossreference(PyObject* /* self */, PyObject* args)
{
	PyESString targetName;
	PyESString objectName;
	PyESString label;
	int position = -1;
	if (!PyArg_ParseTuple(args, "es|esies", "utf-8", targetName.ptr(), "utf-8", objectName.ptr(), &position,
		"utf-8", label.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	PageItem* item = GetUniqueItem(QString::fromUtf8(objectName.c_str()));
	if (!item)
		return nullptr;
	if (!item->isTextFrame())
	{
		PyErr_SetString(WrongFrameTypeError, QObject::tr("Cannot insert a cross-reference into a non-text frame.", "python error").toUtf8().constData());
		return nullptr;
	}
	if (position < -1 || position > item->itemText.length())
	{
		PyErr_SetString(PyExc_IndexError, QObject::tr("Insert index out of bounds.", "python error").toUtf8().constData());
		return nullptr;
	}

	const QString requestedTarget = QString::fromUtf8(targetName.c_str()).trimmed();
	if (!currentDoc->crossReferenceTarget(requestedTarget))
	{
		PyErr_SetString(NotFoundError, QObject::tr("Cross-reference target '%1' was not found.", "python error").arg(requestedTarget).toUtf8().constData());
		return nullptr;
	}
	Mark* reference = currentDoc->insertCrossReferencePageNumber(requestedTarget, item, position,
		QString::fromUtf8(label.c_str()));
	if (!reference)
	{
		PyErr_SetString(ScribusException, QObject::tr("The page reference could not be inserted.", "python error").toUtf8().constData());
		return nullptr;
	}
	return PyUnicode_FromString(reference->label.toUtf8().constData());
}

PyObject *scribus_getcrossreferencepage(PyObject* /* self */, PyObject* args)
{
	PyESString targetName;
	if (!PyArg_ParseTuple(args, "es", "utf-8", targetName.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;
	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	const QString requestedTarget = QString::fromUtf8(targetName.c_str()).trimmed();
	if (!currentDoc->crossReferenceTarget(requestedTarget))
	{
		PyErr_SetString(NotFoundError, QObject::tr("Cross-reference target '%1' was not found.", "python error").arg(requestedTarget).toUtf8().constData());
		return nullptr;
	}
	return PyUnicode_FromString(currentDoc->crossReferencePageNumber(requestedTarget).toUtf8().constData());
}

PyObject *scribus_listcrossreferencetargets(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	const QStringList targets = ScCore->primaryMainWindow()->doc->marksLabelsList(MARKAnchorType);
	PyObject* list = PyList_New(targets.size());
	if (!list)
		return nullptr;
	for (int i = 0; i < targets.size(); ++i)
		PyList_SET_ITEM(list, i, PyUnicode_FromString(targets.at(i).toUtf8().constData()));
	return list;
}

PyObject *scribus_createvariable(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	PyESString value;
	if (!PyArg_ParseTuple(args, "eses", "utf-8", name.ptr(), "utf-8", value.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	const QString variableName = QString::fromUtf8(name.c_str());
	const QString id = currentDoc->addDynamicVariable(variableName, QString::fromUtf8(value.c_str()));
	if (id.isEmpty())
	{
		PyErr_SetString(NameExistsError, QObject::tr("A dynamic variable named '%1' already exists, or the name is empty or reserved.", "python error").arg(variableName).toUtf8().constData());
		return nullptr;
	}
	currentDoc->changed();
	return PyUnicode_FromString(id.toUtf8().constData());
}

PyObject *scribus_createrunningheadervariable(PyObject* /* self */, PyObject* args)
{
	PyESString name;
	PyESString paragraphStyle;
	PyESString mode;
	PyESString textCase;
	PyESString fallback;
	int removeTrailingPunctuation = 0;
	if (!PyArg_ParseTuple(args, "eseses|espes", "utf-8", name.ptr(), "utf-8", paragraphStyle.ptr(), "utf-8", mode.ptr(),
		"utf-8", textCase.ptr(), &removeTrailingPunctuation, "utf-8", fallback.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	const QString variableName = QString::fromUtf8(name.c_str());
	const QString styleName = QString::fromUtf8(paragraphStyle.c_str());
	const auto headerMode = DynamicVariableResolver::runningHeaderModeFromString(QString::fromUtf8(mode.c_str()));
	QString textCaseName = QString::fromUtf8(textCase.c_str());
	if (textCaseName.isEmpty())
		textCaseName = DynamicVariableResolver::AsEnteredCase;
	const auto headerTextCase = DynamicVariableResolver::runningHeaderTextCaseFromString(textCaseName);
	QString fallbackName = QString::fromUtf8(fallback.c_str());
	if (fallbackName.isEmpty())
		fallbackName = DynamicVariableResolver::NoFallback;
	const auto headerFallback = DynamicVariableResolver::runningHeaderFallbackFromString(fallbackName);
	const QString id = currentDoc->addRunningHeaderVariable(variableName, styleName, headerMode, headerTextCase,
		removeTrailingPunctuation != 0, headerFallback);
	if (id.isEmpty())
	{
		PyErr_SetString(ScribusException, QObject::tr("The running header name, paragraph style, mode, fallback, or text formatting is invalid or already in use.", "python error").toUtf8().constData());
		return nullptr;
	}
	currentDoc->changed();
	return PyUnicode_FromString(id.toUtf8().constData());
}

PyObject *scribus_deletevariable(PyObject* /* self */, PyObject* args)
{
	PyESString identifier;
	if (!PyArg_ParseTuple(args, "es", "utf-8", identifier.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	const QString requested = QString::fromUtf8(identifier.c_str());
	const QString id = userDynamicVariableId(currentDoc, requested);
	if (id.isEmpty())
		return dynamicVariableNotFound(requested);
	currentDoc->removeDynamicVariable(id);
	currentDoc->changed();
	Py_RETURN_NONE;
}

PyObject *scribus_getvariable(PyObject* /* self */, PyObject* args)
{
	PyESString identifier;
	PyESString objectName;
	if (!PyArg_ParseTuple(args, "es|es", "utf-8", identifier.ptr(), "utf-8", objectName.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	PageItem* contextFrame = nullptr;
	const QString contextName = QString::fromUtf8(objectName.c_str());
	if (!contextName.isEmpty())
	{
		contextFrame = GetUniqueItem(contextName);
		if (!contextFrame)
			return nullptr;
	}
	const QString requested = QString::fromUtf8(identifier.c_str());
	const QString id = dynamicVariableId(currentDoc, requested);
	if (id.isEmpty())
		return dynamicVariableNotFound(requested);
	return PyUnicode_FromString(currentDoc->resolveDynamicVariable(id, contextFrame).toUtf8().constData());
}

PyObject *scribus_insertvariable(PyObject* /* self */, PyObject* args)
{
	PyESString identifier;
	PyESString objectName;
	int position = -1;
	if (!PyArg_ParseTuple(args, "es|esi", "utf-8", identifier.ptr(), "utf-8", objectName.ptr(), &position))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	PageItem* item = GetUniqueItem(QString::fromUtf8(objectName.c_str()));
	if (!item)
		return nullptr;
	if (!item->isTextFrame() && !item->isPathText())
	{
		PyErr_SetString(WrongFrameTypeError, QObject::tr("Cannot insert a dynamic variable into a non-text frame.", "python error").toUtf8().constData());
		return nullptr;
	}
	if (position < -1 || position > item->itemText.length())
	{
		PyErr_SetString(PyExc_IndexError, QObject::tr("Insert index out of bounds.", "python error").toUtf8().constData());
		return nullptr;
	}

	const QString requested = QString::fromUtf8(identifier.c_str());
	const QString id = dynamicVariableId(currentDoc, requested);
	if (id.isEmpty())
		return dynamicVariableNotFound(requested);

	Mark* mark = currentDoc->getDynamicVariableMark(id);
	if (!mark)
	{
		QString label;
		if (DynamicVariableResolver::isBuiltInId(id))
			label = DynamicVariableResolver::displayNameForType(DynamicVariableResolver::typeForId(id));
		else
			label = currentDoc->dynamicVariable(id)->name;
		getUniqueName(label, currentDoc->marksLabelsList(MARKVariableTextType), QStringLiteral("_"));
		MarkData data;
		data.itemName = item->itemName();
		data.variableId = id;
		data.text = currentDoc->resolveDynamicVariable(id, item);
		mark = currentDoc->newMark();
		mark->setValues(label, item->OwnPage, MARKVariableTextType, data);
	}
	if (position < 0)
		position = item->itemText.length();
	item->itemText.insertMark(mark, position);
	item->invalidateLayout();
	currentDoc->changed();
	currentDoc->flag_updateMarksLabels = true;
	return PyUnicode_FromString(id.toUtf8().constData());
}

PyObject *scribus_listvariables(PyObject* /* self */)
{
	if (!checkHaveDocument())
		return nullptr;
	const ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	const auto& variables = currentDoc->dynamicVariables();
	PyObject* list = PyList_New(variables.size());
	if (!list)
		return nullptr;
	int index = 0;
	for (auto it = variables.constBegin(); it != variables.constEnd(); ++it)
	{
		const DynamicVariable& variable = it.value();
		PyObject* tuple = Py_BuildValue("(sss)", variable.id.toUtf8().constData(), variable.name.toUtf8().constData(), variable.value.toUtf8().constData());
		if (!tuple)
		{
			Py_DECREF(list);
			return nullptr;
		}
		PyList_SET_ITEM(list, index++, tuple);
	}
	return list;
}

PyObject *scribus_renamevariable(PyObject* /* self */, PyObject* args)
{
	PyESString identifier;
	PyESString newName;
	if (!PyArg_ParseTuple(args, "eses", "utf-8", identifier.ptr(), "utf-8", newName.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	const QString requested = QString::fromUtf8(identifier.c_str());
	const QString id = userDynamicVariableId(currentDoc, requested);
	if (id.isEmpty())
		return dynamicVariableNotFound(requested);
	const DynamicVariable variable = *currentDoc->dynamicVariable(id);
	if (!currentDoc->updateDynamicVariable(id, QString::fromUtf8(newName.c_str()), variable.value))
	{
		PyErr_SetString(NameExistsError, QObject::tr("The new dynamic variable name is empty or already in use.", "python error").toUtf8().constData());
		return nullptr;
	}
	currentDoc->changed();
	Py_RETURN_NONE;
}

PyObject *scribus_setvariable(PyObject* /* self */, PyObject* args)
{
	PyESString identifier;
	PyESString value;
	if (!PyArg_ParseTuple(args, "eses", "utf-8", identifier.ptr(), "utf-8", value.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	const QString requested = QString::fromUtf8(identifier.c_str());
	const QString id = userDynamicVariableId(currentDoc, requested);
	if (id.isEmpty())
		return dynamicVariableNotFound(requested);
	const DynamicVariable variable = *currentDoc->dynamicVariable(id);
	if (variable.type != DynamicVariableResolver::UserDefined)
	{
		PyErr_SetString(ScribusException, QObject::tr("The value of computed dynamic variable '%1' cannot be set.", "python error").arg(variable.name).toUtf8().constData());
		return nullptr;
	}
	if (!currentDoc->updateDynamicVariable(id, variable.name, QString::fromUtf8(value.c_str())))
	{
		PyErr_SetString(ScribusException, QObject::tr("The dynamic variable could not be updated.", "python error").toUtf8().constData());
		return nullptr;
	}
	currentDoc->changed();
	Py_RETURN_NONE;
}

PyObject *scribus_setrunningheadervariable(PyObject* /* self */, PyObject* args)
{
	PyESString identifier;
	PyESString name;
	PyESString paragraphStyle;
	PyESString mode;
	PyESString textCase;
	PyESString fallback;
	int removeTrailingPunctuation = -1;
	if (!PyArg_ParseTuple(args, "eseseses|espes", "utf-8", identifier.ptr(), "utf-8", name.ptr(),
		"utf-8", paragraphStyle.ptr(), "utf-8", mode.ptr(), "utf-8", textCase.ptr(), &removeTrailingPunctuation,
		"utf-8", fallback.ptr()))
		return nullptr;
	if (!checkHaveDocument())
		return nullptr;

	ScribusDoc* currentDoc = ScCore->primaryMainWindow()->doc;
	const QString requested = QString::fromUtf8(identifier.c_str());
	const QString id = userDynamicVariableId(currentDoc, requested);
	if (id.isEmpty())
		return dynamicVariableNotFound(requested);
	const DynamicVariable* variable = currentDoc->dynamicVariable(id);
	if (!variable || variable->type != DynamicVariableResolver::RunningHeader)
	{
		PyErr_SetString(ScribusException, QObject::tr("Dynamic variable '%1' is not a running header.", "python error").arg(requested).toUtf8().constData());
		return nullptr;
	}
	const auto headerMode = DynamicVariableResolver::runningHeaderModeFromString(QString::fromUtf8(mode.c_str()));
	QString textCaseName = QString::fromUtf8(textCase.c_str());
	if (textCaseName.isEmpty())
		textCaseName = variable->runningHeaderTextCase;
	const auto headerTextCase = DynamicVariableResolver::runningHeaderTextCaseFromString(textCaseName);
	const bool removePunctuation = removeTrailingPunctuation < 0
		? variable->removeTrailingPunctuation : removeTrailingPunctuation != 0;
	QString fallbackName = QString::fromUtf8(fallback.c_str());
	if (fallbackName.isEmpty())
		fallbackName = variable->runningHeaderFallback;
	const auto headerFallback = DynamicVariableResolver::runningHeaderFallbackFromString(fallbackName);
	if (!currentDoc->updateRunningHeaderVariable(id, QString::fromUtf8(name.c_str()),
		QString::fromUtf8(paragraphStyle.c_str()), headerMode, headerTextCase, removePunctuation, headerFallback))
	{
		PyErr_SetString(ScribusException, QObject::tr("The running header name, paragraph style, mode, fallback, or text formatting is invalid or already in use.", "python error").toUtf8().constData());
		return nullptr;
	}
	currentDoc->changed();
	Py_RETURN_NONE;
}
