
#include <QTest>
//#include "testGlyphStore.h"
//#include "testIndex.h"
#include "testStoryText.h"
#include "runtests.h"

int RunTests::runTests(const QStringList& arguments)
{ 
	QList<QObject *> testObjects;
//	testObjects << new TestGlyphStore();
	testObjects << new TestStoryText();
//	testObjects << new TestIndex();
	int failed = 0;
	for (int i = 0; i < testObjects.count(); ++i)
	{
		int rc = QTest::qExec(testObjects[i], arguments);
		if (rc != 0)
		{
			qDebug() << "          <<<<<<<<<< FAIL!" << rc << ">>>>>>>>>";
			++failed;
		}
	}
	return failed;
}
