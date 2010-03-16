#include <QApplication>
#include <QAction>
#include <QToolBar>
#include <QTimer>
#include <QMainWindow>

#include "qsvtextedit.h"
#include "qsvsyntaxhighlighterbase.h"
#include "qsvtextoperationswidget.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QMainWindow w;
	
	QsvSyntaxHighlighterBase *s             = new QsvSyntaxHighlighterBase;
	QsvTextEdit              *e             = new QsvTextEdit(&w, s);
	QsvTextOperationsWidget *textOpetations = new QsvTextOperationsWidget(e);

	QToolBar *b = w.addToolBar( "" );
	b->addAction( w.tr("Find"), textOpetations, SLOT(showSearch()))
	 ->setShortcut(QKeySequence("Ctrl+F"));
	b->addAction( w.tr("Replace"), textOpetations, SLOT(showReplace()))
	 ->setShortcut(QKeySequence("Ctrl+R"));
	b->addAction( w.tr("Fing next"), textOpetations, SLOT(searchNext()))
	 ->setShortcut(QKeySequence("F3"));
	b->setMovable(false);
	b->addAction( w.tr("Fing prev"), textOpetations, SLOT(searchPrev()))
			->setShortcut(QKeySequence("Shift+F3"));
	b->setMovable(false);

#if 0
	e->setPlainText(
"#include <stdio.h>\n\n"
"int main()       \n"
"{       \n"
"	{ printf(\"Hello world!\\n\"); }           \n"
"		}\n" );
	e->removeModifications();
#else
	e->loadFile("qsvtextedit.cpp");
//	e->newDocument();
#endif
	
	// tests for defaults
	e->setShowLineNumbers(true);
	e->setShowMargins(true);
	e->setTabSize(8);
	e->setTabIndents(true);
	e->setInsertSpacesInsteadOfTabs(true);
	e->setShowWhiteSpace(true);
		
	w.setCentralWidget(e);
	w.showMaximized();
	a.connect(&a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()));
	return a.exec();
}
