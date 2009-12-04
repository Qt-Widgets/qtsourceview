#include "qsvtextoperationswidget.h"
#include "ui_searchform.h"
#include "ui_replaceform.h"

#include <QLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QTextDocument>

#include <QDebug>

QsvTextOperationsWidget::QsvTextOperationsWidget( QWidget *parent )
	: QObject(parent)
{
	m_gotoLine = NULL;
	m_search   = NULL;
	m_replace  = NULL;
	m_document = NULL;
	searchFoundColor	= QColor( "#DDDDFF" ); //QColor::fromRgb( 220, 220, 255)
	searchNotFoundColor	= QColor( "#FFAAAA" ); //QColor::fromRgb( 255, 102, 102) "#FF6666"

	QTextEdit *t = qobject_cast<QTextEdit*>(parent);
	if (t) {
		m_document = t->document();
	}
	else {
		QPlainTextEdit *pt = qobject_cast<QPlainTextEdit*>(parent);
		if (pt) {
			m_document = pt->document();
		}
	}
}

void QsvTextOperationsWidget::initSearchWidget()
{
	m_search = new QWidget( (QWidget*) parent() );
	searchFormUi = new Ui::searchForm();
	searchFormUi->setupUi(m_search);
	searchFormUi->lineEdit->setFont( m_search->parentWidget()->font() );
	// otherwise, it inherits the default font from the editor - fixed
	m_search->setFont(QFont("sans"));
	m_search->adjustSize();
	m_search->hide();

	connect( searchFormUi->lineEdit,SIGNAL(textChanged(QString)), this, SLOT(on_searchText_modified(QString)));
	connect( searchFormUi->closeButton, SIGNAL(clicked()), this, SLOT(showSearch()));
}

void QsvTextOperationsWidget::initReplaceWidget()
{
	m_replace = new QWidget( (QWidget*) parent() );
	replaceFormUi = new Ui::replaceForm();
	replaceFormUi->setupUi(m_replace);
	replaceFormUi->optionsGroupBox->hide();
	connect(replaceFormUi->moreButton,SIGNAL(clicked()),this,SLOT(adjustReplaceSize()));
	m_replace->setFont(QFont("sans"));
	m_replace->adjustSize();
	m_replace->hide();

}

void QsvTextOperationsWidget::adjustReplaceSize()
{
	m_replace->adjustSize();
	showBottomWidget(m_replace);
}

QFlags<QTextDocument::FindFlag> QsvTextOperationsWidget::getSearchFlags()
{
	QFlags<QTextDocument::FindFlag> f;

	// one can never be too safe
	if (!searchFormUi){
		qDebug("%s:%d - searchFormUi not available, memory problems?", __FILE__, __LINE__ );
		return f;
	}

	if (searchFormUi->caseSensitiveCheckBox->isChecked())
		f = f | QTextDocument::FindCaseSensitively;
	if (searchFormUi->wholeWorldsCheckbox->isChecked())
		f = f | QTextDocument::FindWholeWords;
	return f;
}

QTextCursor	QsvTextOperationsWidget::getTextCursor()
{
	QTextCursor searchCursor;
	QTextEdit *t = qobject_cast<QTextEdit*>(parent());
	if (t) {
		searchCursor = t->textCursor();
	} else {
		QPlainTextEdit *pt = qobject_cast<QPlainTextEdit*>(parent());
		if (pt) {
			searchCursor = pt->textCursor();
		}
	}
	return searchCursor;
}

void	QsvTextOperationsWidget::setTextCursor(QTextCursor c)
{
	QTextEdit *t = qobject_cast<QTextEdit*>(parent());
	if (t) {
		t->setTextCursor(c);
	} else {
		QPlainTextEdit *pt = qobject_cast<QPlainTextEdit*>(parent());
		if (pt) {
			pt->setTextCursor(c);
		}
	}
}

void QsvTextOperationsWidget::showSearch()
{
	if (!m_search)
		initSearchWidget();
	if (m_replace && m_replace->isVisible())
		m_replace->hide();
	QWidget *parent = qobject_cast<QWidget*>(this->parent());
	if (m_search->isVisible()) {
		m_search->hide();
		if (parent)
			parent->setFocus();
		return;
	}

	m_searchCursor = getTextCursor();
	searchFormUi->lineEdit->setFocus();
	showBottomWidget(m_search);
}

void	QsvTextOperationsWidget::showReplace()
{
	if (!m_replace)
		initReplaceWidget();
	if (m_search && m_search->isVisible())
		m_search->hide();
	QWidget *parent = qobject_cast<QWidget*>(this->parent());
	if (m_replace->isVisible()) {
		m_replace->hide();
		if (parent)
			parent->setFocus();
		return;
	}

	showBottomWidget(m_replace);
}

void	QsvTextOperationsWidget::showBottomWidget(QWidget* w)
{
	QWidget *parent = qobject_cast<QWidget*>(this->parent());
	QRect r = parent->rect();
	r.adjust(10, 0, -10, 0);
	r.setHeight(w->height());
	r.moveBottom(parent->rect().height()-10);
	w->setGeometry(r);
	w->show();
}

void QsvTextOperationsWidget::on_searchText_modified(QString s)
{
	issue_search( s, m_searchCursor, getSearchFlags() );
}

bool	QsvTextOperationsWidget::issue_search( const QString &text, QTextCursor newCursor, QFlags<QTextDocument::FindFlag> findOptions  )
{
	QTextCursor c = m_document->find( text, newCursor, findOptions );
	bool found = ! c.isNull();
	qDebug() << findOptions;

	//lets try again, from the start
	if (!found) {
		c.movePosition(findOptions.testFlag(QTextDocument::FindBackward)? QTextCursor::End : QTextCursor::Start);
		c = m_document->find(text, c, findOptions);
		found = ! c.isNull();
	}

	QPalette p = searchFormUi->lineEdit->palette();
	if (found) {
		p.setColor(QPalette::Base, searchFoundColor);
	} else {
		if (!text.isEmpty())
			p.setColor(QPalette::Base, searchNotFoundColor);
		else
			p.setColor(QPalette::Base,
				searchFormUi->lineEdit->style()->standardPalette().base().color()
			);
		c =  m_searchCursor;
	}
	searchFormUi->lineEdit->setPalette(p);
	setTextCursor( c );
	return found;
}
