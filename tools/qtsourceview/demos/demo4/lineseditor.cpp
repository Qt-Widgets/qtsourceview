#include <QPainter>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include <QScrollBar>
#include <QPushButton>
#include <QAction>
#include <QKeySequence>
#include <QTimer>
#include <QPalette>
#include <QFile>
#include <QTextStream>
#include <QStyle>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QDebug>

#include "privateblockdata.h"
#include "qsvsyntaxhighlighter.h"
#include "lineseditor.h"
#include "samplepanel.h"
#include "transparentwidget.h"
#include "ui_findwidget.h"

static const char * tabPixmap_img[] = 
{
/* width height ncolors cpp [x_hot y_hot] */
	"8 8 3 2 0 0",
/* colors */
	"  s none       m none  c none",
	"O s iconColor1 m black c black",
	"X s iconColor2 m black c #E0E0E0",
/* pixels */
	"  X     X       ",
	"    X     X     ",
	"      X     X   ",
	"        X     X ",
	"      X     X   ",
	"    X     X     ",
	"  X     X       ",
	"                ",
};

static const char * spacePixmap_img[] = 
{
/* width height ncolors cpp [x_hot y_hot] */
	"8 8 3 2 0 0",
/* colors */
	"  s none       m none  c none",
	"O s iconColor1 m black c black",
	"X s iconColor2 m black c #E0E0E0",
/* pixels */
	"                ",
	"                ",
	"                ",
	"                ",
	"                ",
	"      X         ",
	"      X X       ",
	"                ",
};

LinesEditor::LinesEditor( QWidget *p ) :QTextEdit(p)
{
	tabPixmap		= QPixmap( tabPixmap_img ); 
	spacePixmap		= QPixmap( spacePixmap_img ); 
	currentLineColor	= QColor( "#DCE4F9" );
	bookmarkLineColor	= QColor( "#0000FF" );
	breakpointLineColor	= QColor( "#FF0000" );
	matchBracesColor	= QColor( "#FF0000" );
	searchFoundColor	= QColor( "#DDDDFF" ); //QColor::fromRgb( 220, 220, 255)
	searchNotFoundColor	= QColor( "#FFAAAA" ); //QColor::fromRgb( 255, 102, 102) "#FF6666"
	whiteSpaceColor		= QColor( "#E0E0E0" );
	highlightCurrentLine	= true;
	showWhiteSpaces		= true;
	showMatchingBraces	= true;
	showPrintingMargins	= false;
	printMarginWidth	= 80;
	matchStart		= -1;
	matchEnd		= -1;
	matchingString		= "(){}[]\"\"''``";

	actionFind		= NULL;
	actionFindNext		= NULL;
	actionFindPrev		= NULL;
	actionCapitalize	= NULL;
	actionLowerCase		= NULL;
	actionChangeCase	= NULL;
	actionToggleBookmark	= NULL;
	actionTogglebreakpoint	= NULL;
	
	panel = new SamplePanel( this );
	panel->panelColor = QColor( "#FFFFD0" );
	panel->setVisible( true );

	setFrameStyle( QFrame::NoFrame );
	setLineWrapMode( QTextEdit::NoWrap );
	setAcceptRichText( false );
	QTimer::singleShot( 0, this, SLOT(adjustMarginWidgets()));
	syntaxHighlighter = NULL;

#ifdef WIN32
	QFont f("Courier New", 10);
#else
	QFont f("Monospace", 9);
#endif
	setFont( f );
	panel->setFont( f );
	
	findWidget = new TransparentWidget( this, 0.8 );
	ui_findWidget.setupUi( findWidget );
	ui_findWidget.searchText->setIcon( QPixmap(":/images/clear_left.png") );
	findWidget->hide();
	
	fileMessage = new TransparentWidget( this, 0.8 );
	ui_fileMessage.setupUi( fileMessage );
	connect( ui_fileMessage.label, SIGNAL(linkActivated(const QString&)), this, SLOT(on_fileMessage_clicked(QString)));
	fileMessage->hide();
	
	fileSystemWatcher = new QFileSystemWatcher(this);

	//connect( horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustMarginWidgets()));
	//connect( verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(adjustMarginWidgets()));
	connect( this, SIGNAL(cursorPositionChanged()), this, SLOT(on_cursorPositionChanged()));
	connect( document(), SIGNAL(contentsChanged()), this, SLOT(on_textDocument_contentsChanged()));
	connect( ui_findWidget.searchText, SIGNAL(textChanged(const QString)), this, SLOT(on_searchText_textChanged(const QString)) );
	connect( ui_findWidget.closeButton, SIGNAL(clicked()), this, SLOT(showFindWidget()));
	connect( ui_fileMessage.closeButton, SIGNAL(clicked()), fileMessage, SLOT(hide()));
	connect( fileSystemWatcher, SIGNAL(fileChanged(const QString&)), this, SLOT(on_fileChanged(const QString&)));
}

void LinesEditor::setupActions()
{
	actionFind = new QAction( "Find (inline)", this );
	actionFind->setObjectName("actionFind");
	actionFind->setShortcut( QKeySequence("Ctrl+F") );
	connect( actionFind, SIGNAL(triggered()), this, SLOT(showFindWidget()) );

	actionFindNext = new QAction( "Find next", this );
	actionFindNext->setObjectName("actionFindNext");
	actionFindNext->setShortcut( QKeySequence("F3") );
	connect( actionFindNext, SIGNAL(triggered()), this, SLOT(findNext()) );
	
	actionFindPrev = new QAction( "Find previous", this );
	actionFindPrev->setObjectName("actionFindPrev");
	actionFindPrev->setShortcut( QKeySequence("Shift+F3") );
	connect( actionFindPrev, SIGNAL(triggered()), this, SLOT(findPrev()) );
	
	actionCapitalize = new QAction( "Change to capital letters", this );
	actionCapitalize->setObjectName( "actionCapitalize" );
	actionCapitalize->setShortcut( QKeySequence( Qt::CTRL | Qt::Key_U ) );
	connect( actionCapitalize, SIGNAL(triggered()), this, SLOT(transformBlockToUpper()) );

	actionLowerCase = new QAction( "Change to lower letters", this );
	actionLowerCase->setObjectName( "actionLowerCase" );
	actionLowerCase->setShortcut( QKeySequence( Qt::CTRL | Qt::SHIFT | Qt::Key_U  ) );
	connect( actionLowerCase, SIGNAL(triggered()), this, SLOT(transformBlockToLower()) );

	actionChangeCase = new QAction( "Change case", this );
	actionChangeCase->setObjectName( "actionChangeCase" );
	connect( actionChangeCase, SIGNAL(triggered()), this, SLOT(transformBlockCase()) );

	actionToggleBookmark = new QAction( "Toggle line bookmark", this );
	actionToggleBookmark->setObjectName( "actionToggleBookmark" );
	actionToggleBookmark->setShortcut( QKeySequence( Qt::CTRL | Qt::Key_B  ) );
	connect( actionToggleBookmark, SIGNAL(triggered()), this, SLOT(toggleBookmark()) );

	actionTogglebreakpoint = new QAction( "Toggle breakpoint", this );
	actionTogglebreakpoint->setObjectName( "actionTogglebreakpoint" );
	actionTogglebreakpoint->setShortcut( QKeySequence("F9") );
	connect( actionTogglebreakpoint, SIGNAL(triggered()), this, SLOT(toggleBreakpoint()) );
}

QColor LinesEditor::getItemColor( ItemColors role )
{
	switch (role)
	{
		case LinesPanel:	return panel->panelColor; 
		case CurrentLine:	return currentLineColor;
		case MatchBrackets:	return matchBracesColor;
		case NoText:		return searchNoText;
		case TextFound:		return searchFoundColor;
		case TextNoFound:	return searchNotFoundColor;
		case WhiteSpaceColor:	return whiteSpaceColor;
		case BookmarkLineColor:	return bookmarkLineColor;
		case BreakpointLineColor: return breakpointLineColor;
	}
	
	// just to keep gcc happy, will not get executed
	return QColor();
}

void LinesEditor::setItemColor( ItemColors role, QColor c )
{
	switch (role)
	{
		case LinesPanel:	
			panel->panelColor = c;
			panel->update();
			break;
		case CurrentLine:	
			currentLineColor = c;
			break;
		case MatchBrackets:
			matchBracesColor = c;
			break;
		case NoText:	
			searchNoText = c;
			break;
		case TextFound:
			searchFoundColor = c;
			break;
		case TextNoFound:
			searchNotFoundColor = c;
			break;
		case WhiteSpaceColor:
			whiteSpaceColor = c;
			updateMarkIcons();
			break;
		case BookmarkLineColor:
			bookmarkLineColor = c;
		case BreakpointLineColor: 
			breakpointLineColor = c;
	}
}

void	LinesEditor::setMargin( int width )
{
	printMarginWidth = width;
	showPrintingMargins = (width>0);
}

void	LinesEditor::setTabSize( int size )
{
	const QFontMetrics fm = QFontMetrics( document()->defaultFont() );
	int j = fm.width( " " ) * size;
	setTabStopWidth( j );
}

void LinesEditor::findMatching( QChar c1, QChar c2, bool forward, QTextBlock &block )
{
	int i = matchStart;
	int n = 1;
	QChar c;
	QString blockString = block.text();
	
	do
	{
		if (forward)
		{
			i ++;
			if ((i - block.position()) == block.length())
			{
				block = block.next();
				blockString = block.text();
			}
		}
		else
		{
			i --;
			if ((i - block.position()) == -1)
			{
				block = block.previous();
				blockString = block.text();
			}
		}
		if (block.isValid())
			c = blockString[i - block.position()];
		else
			break;

		if (c == c1)
			n++;
		else if (c == c2)
			n--;
	} while (n!=0);
	
	if (n == 0)
		matchEnd = i;
	else
		matchEnd = -1;
}

void	LinesEditor::findMatching( QChar c, QTextBlock &block )
{
	int n = 0;
	QString blockString = block.text();
	int blockLen = block.length();
	
	// try forward
	while (n < blockLen) 
	{
		if (n != matchStart - block.position())
			if (blockString[n] == c)
			{
				matchEnd = block.position() + n;
				return;
			}
		n++;
	}
}

PrivateBlockData* LinesEditor::getPrivateBlockData( QTextBlock block )
{
	QTextBlockUserData *d1 = block.userData();
	PrivateBlockData *data = dynamic_cast<PrivateBlockData*>( d1 );
	
	// a user data has been defined, and it's not our structure
	if (d1 && !data)
		return NULL;
	
	if (!data)
	{
		data = new PrivateBlockData;
		block.setUserData( data );
	}
	return data;
}

QsvSyntaxHighlighter* LinesEditor::getSyntaxHighlighter()
{
	return syntaxHighlighter;
}

void LinesEditor::setSyntaxHighlighter( QsvSyntaxHighlighter *newSyntaxHighlighter )
{
	syntaxHighlighter = newSyntaxHighlighter;
	syntaxHighlighter->rehighlight();
}

QTextCursor	LinesEditor::getCurrentTextCursor()
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection())
		cursor.select(QTextCursor::WordUnderCursor);
	return cursor;
}

void	LinesEditor::showFindWidget()
{	
	if (findWidget->isVisible())
	{
		findWidget->hide();
		this->setFocus();
		return;
	}

	searchCursor = textCursor();
	widgetToBottom( findWidget );
	ui_findWidget.searchText->clear();
	ui_findWidget.searchText->setFocus();
}

void	LinesEditor::findNext()
{
	issue_search( ui_findWidget.searchText->text(), textCursor(), 0 );
}

void LinesEditor::findPrev()
{
	issue_search( ui_findWidget.searchText->text(), textCursor(), QTextDocument::FindBackward );
}

bool LinesEditor::issue_search( const QString &text, QTextCursor newCursor, QFlags<QTextDocument::FindFlag> findOptions  )
{
	QTextCursor c = document()->find( text, newCursor, findOptions );
	bool found = ! c.isNull();
	
	if (!found)
	{
		//lets try again, from the start
		c.movePosition(findOptions.testFlag(QTextDocument::FindBackward)? QTextCursor::End : QTextCursor::Start);
		c = document()->find( ui_findWidget.searchText->text(), c, findOptions );
		found = ! c.isNull();
	}
	
	if (found)
	{
		QPalette ok = this->palette();
		ok.setColor( QPalette::Base, searchFoundColor );
		ui_findWidget.searchText->setPalette( ok );
		setTextCursor( c );
	}
	else
	{
		QPalette bad = this->palette();
		bad.setColor( QPalette::Base, searchNotFoundColor );
		ui_findWidget.searchText->setPalette( bad );	
		setTextCursor( searchCursor );
	}
	
	return found;
}

int LinesEditor::loadFile( QString s )
{
	QFile file(s);
	QFileInfo fileInfo(file);
	
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return -1;
	
	QTextStream in(&file);
	setPlainText( in.readAll() );
	
	// clear older watches, and add a new one
	QStringList sl = fileSystemWatcher->directories();
	if (!sl.isEmpty())
		fileSystemWatcher->removePaths( sl );
	
	fileName = fileInfo.absoluteFilePath();
	fileSystemWatcher->addPath( fileName );
	
	return 0;
}

void LinesEditor::setDisplayCurrentLine( bool b )
{
	highlightCurrentLine = b;
}

void LinesEditor::setDisplayWhiteSpaces( bool b )
{
	showWhiteSpaces = b;
}

void LinesEditor::setDisplatMatchingBrackets( bool b )
{
	
	showMatchingBraces = b;
}

void LinesEditor::setMatchingString( QString s )
{
	matchingString = s;	
}

void	LinesEditor::setBookmark( BookmarkAction action, QTextBlock block  )
{
	PrivateBlockData *data = getPrivateBlockData( block );
	if (!data)
		return;

	switch (action)
	{
		case Toggle:
			data->m_isBookmark = ! data->m_isBookmark;
			break;
		case Enable: 
			data->m_isBookmark = true;
			break;
		case Disable:
			data->m_isBookmark = false;
			break;
	}

	updateCurrentLine();
	panel->update();
}

void	LinesEditor::toggleBookmark()
{
	setBookmark( Toggle, textCursor().block() );
}

void	LinesEditor::setBreakpoint( BookmarkAction action, QTextBlock block )
{
	PrivateBlockData *data = getPrivateBlockData( block );
	if (!data)
		return;

	switch (action)
	{
		case Toggle:
			data->m_isBreakpoint = ! data->m_isBreakpoint;
			break;
		case Enable: 
			data->m_isBreakpoint = true;
			break;
		case Disable:
			data->m_isBreakpoint = false;
			break;
	}

	updateCurrentLine();
	panel->update();
}

void	LinesEditor::toggleBreakpoint()
{
	setBreakpoint( Toggle, textCursor().block() );
}

void	LinesEditor::transformBlockToUpper()
{
	//QTextCursor cursor = textCursor();
	//if (!cursor.hasSelection())
		//cursor.select(QTextCursor::WordUnderCursor);
	//return cursor;

	QTextCursor cursor = getCurrentTextCursor();
	QString s_before = cursor.selectedText();
	QString s_after  = s_before.toUpper();
	
	if (s_before != s_after)
	{
		cursor.beginEditBlock();
		cursor.deleteChar();
		cursor.insertText( s_after );
		cursor.endEditBlock();
		setTextCursor( cursor );
	}
}

void	LinesEditor::transformBlockToLower()
{
	QTextCursor cursor = getCurrentTextCursor();
	QString s_before = cursor.selectedText();
	QString s_after  = s_before.toLower();
	
	if (s_before != s_after)
	{
		cursor.beginEditBlock();
		cursor.deleteChar();
		cursor.insertText( s_after );
		cursor.endEditBlock();
		setTextCursor( cursor );
	}
}

void	LinesEditor::transformBlockCase()
{
	QTextCursor cursor = getCurrentTextCursor();
	QString s_before = cursor.selectedText();
	QString s_after = s_before;
	uint s_len = s_before.length();
	
	for( uint i=0; i< s_len; i++ )
		if (s_after[i].isLower())
			s_after[i] = s_after[i].toUpper();
		else if (s_after[i].isUpper())
			s_after[i] = s_after[i].toLower();
		
		
	if (s_before != s_after)
	{
		cursor.beginEditBlock();
		cursor.deleteChar();
		cursor.insertText( s_after );
		cursor.endEditBlock();
		setTextCursor( cursor );
	}
}

void LinesEditor::keyPressEvent( QKeyEvent *event )
{
	switch (event->key())
	{
		case Qt::Key_Escape:
			if (findWidget->isVisible())
				// hide  it
				showFindWidget();
			else
			{
				// clear selection
				QTextCursor c = textCursor();
				if (c.hasSelection())
				{
					c.clearSelection();
					setTextCursor(c);
				}
			}
			break;
			
		case Qt::Key_Enter:
		case Qt::Key_Return:
			// TODO, BUG: ignore "enter" keypresses, if looking for text
			if (findWidget->isVisible())
				return;
			break;
			
		case Qt::Key_Tab:
			//if (tabIndents)
			{
				if (handleIndentEvent( !(event->modifiers() & Qt::ShiftModifier) ))
					// do not call original hanlder, if this was handled by that function
					return; 
			}
		default:
			if (handleKeyPressEvent(event))
				return;
	} // end case
	
	QTextEdit::keyPressEvent( event );
}

void LinesEditor::resizeEvent ( QResizeEvent *event )
{
	QTextEdit::resizeEvent( event );
	adjustMarginWidgets();

	if (findWidget->isVisible())
	{
		findWidget->hide();
		showFindWidget();
	}

	if (fileMessage->isVisible())
	{
		fileMessage->hide();
		widgetToTop( fileMessage );
	}
}

void LinesEditor::paintEvent(QPaintEvent *e)
{
	// if no special painting, no need to create the QPainter
	if (highlightCurrentLine || showWhiteSpaces || showMatchingBraces)
	{
		QPainter p( viewport() );
		
		printBackgrounds(p);
		QTextEdit::paintEvent(e);
		
		if (showMatchingBraces)
			printMatchingBraces( p );
	}
	else
		QTextEdit::paintEvent(e);
}

void	LinesEditor::printBackgrounds( QPainter &p )
{
	const int contentsY = verticalScrollBar()->value();
	const qreal pageBottom = contentsY + viewport()->height();
	
	for ( QTextBlock block = document()->begin(); block.isValid(); block = block.next() )
	{
		QTextLayout* layout = block.layout();
		const QRectF boundingRect = layout->boundingRect();
		QPointF position = layout->position();
		
		if ( position.y() +boundingRect.height() < contentsY )
			continue;
		if ( position.y() > pageBottom )
			break;
			
		if (highlightCurrentLine)
			printCurrentLines( p, block );
		if (showWhiteSpaces)
			printWhiteSpaces( p, block );
	}

	if (showPrintingMargins)
		printMargins( p );
}

void	LinesEditor::printWhiteSpaces( QPainter &p, QTextBlock &block )
{
	const QFontMetrics fm = QFontMetrics( document()->defaultFont() );
	const QString txt = block.text();
	const int len = txt.length();
	
	for ( int i=0; i<len; i++ )
	{
		QPixmap *p1 = 0;
		
		if (txt[i] == ' ' )
			p1 = &spacePixmap;
		else if (txt[i] == '\t' )
			p1 = &tabPixmap;
		else 
			continue;
		
		// pixmaps are of size 8x8 pixels
		QTextCursor cursor = textCursor();
		cursor.setPosition( block.position() + i, QTextCursor::MoveAnchor);
		
		QRect r = cursorRect( cursor );
		int x = r.x() + 4;
		int y = r.y() + fm.height() / 2 - 5;
		p.drawPixmap( x, y, *p1 );
	}
}

void	LinesEditor::printCurrentLines( QPainter &p, QTextBlock &block )
{
	PrivateBlockData *data = dynamic_cast<PrivateBlockData*>( block.userData() );
	QTextCursor cursor = textCursor();
	cursor.setPosition( block.position(), QTextCursor::MoveAnchor);
	QRect r = cursorRect( cursor );
	r.setX( 0 );
	r.setWidth( viewport()->width() );

	p.save();
	p.setOpacity( 0.8 );
	if (r.top() == cursorRect().top() )
		p.fillRect( r, currentLineColor );

	p.setOpacity( 0.2 );
	if (data)
	{
		if (data->m_isBookmark)
			p.fillRect( r, bookmarkLineColor );
			
		if (data->m_isBreakpoint)
			p.fillRect( r, breakpointLineColor );
			
		// print all other states
	}

	p.restore();
}

void	LinesEditor::timerEvent( QTimerEvent *event )
{
	// TODO
	Q_UNUSED( event );
}

QWidget* LinesEditor::getPanel()
{
	return panel;
}

void	LinesEditor::updateCurrentLine()
{
	if (highlightCurrentLine)
		viewport()->update();
}

void LinesEditor::on_searchText_textChanged( const QString & text )
{
	if (text.isEmpty())
	{
		QPalette p = palette();
		p.setColor( QPalette::Base, QColor("#FFFFFF") ); // white
		ui_findWidget.searchText->setPalette( p );
		return;
	}
	
	issue_search( text, searchCursor, !QTextDocument::FindCaseSensitively | !QTextDocument::FindBackward ); 
}

void	LinesEditor::on_cursorPositionChanged()
{
	QTextCursor cursor = textCursor();
	int pos = cursor.position();
	if (pos == -1)
	{
		matchStart = matchEnd = -1;
		currentChar = matchChar = 0;
		if (highlightCurrentLine)
			updateCurrentLine();
		return;
	}
		
	QTextBlock  block = cursor.block();
		int i = cursor.position();
	currentChar = block.text()[i - block.position() ];
	matchStart = i;

	int j = matchingString.indexOf( currentChar );

	if (j == -1)
	{
		matchStart = matchEnd = -1;
		currentChar = matchChar = 0;
		if (highlightCurrentLine)
			updateCurrentLine();
		return;
	}

	if ( matchingString[j] != matchingString[j+1] )
		if (j %2 == 0)
			findMatching( matchingString[j], matchChar = matchingString[j+1], true, block );
		else
			findMatching( matchingString[j], matchChar = matchingString[j-1], false, block );
	else
		findMatching( matchChar = matchingString[j], block );
		
	updateCurrentLine();
}

void	LinesEditor::on_textDocument_contentsChanged()
{
	PrivateBlockData* data = getPrivateBlockData( textCursor().block() );
	if (!data)
		return;
	
	data->m_isModified = true;
	panel->update();
}

void	LinesEditor::on_fileChanged( const QString &fName )
{
	if (fName != fileName)
		return;
		
	QFileInfo f (fileName);
	
	if (f.exists())
		ui_fileMessage.label->setText( tr("File has been modified outside the editor. <a href=':reload' title='Clicking this links will revert all changes to this editor'>Click here to reload.</a>") );
	else
		ui_fileMessage.label->setText( tr("File has been deleted outside the editor.") );
	
	widgetToTop( fileMessage );
}

void	LinesEditor::on_fileMessage_clicked( QString s )
{
	if (s == ":reload")
	{
		loadFile( fileName );
		ui_fileMessage.label->setText( "" );
		fileMessage->hide();
	}
}

void	LinesEditor::adjustMarginWidgets()
{
	if (panel->isVisible())
	{
		setViewportMargins( panel->width()-1, 0, 0, 0);
		QRect viewportRect = viewport()->geometry();
		QRect lrect = QRect(viewportRect.topLeft(), viewportRect.bottomLeft());
		lrect.adjust( -panel->width(), 0, 0, 0 );
		panel->setGeometry(lrect);		
	}
	else{
		setViewportMargins( 0, 0, 0, 0);
	}
}

void	LinesEditor::printMatchingBraces( QPainter &p )
{
	if (matchStart == -1)
		return;
		
	QFont f = document()->defaultFont();
	QTextCursor cursor = textCursor();

	f.setBold( true );
	p.setFont( f );
	p.setPen( matchBracesColor );
	cursor.setPosition(matchStart+1, QTextCursor::MoveAnchor);
	QRect r = cursorRect( cursor );
	p.drawText(r.x()-1, r.y(), r.width(), r.height(), Qt::AlignLeft | Qt::AlignVCenter, currentChar );

	if (matchEnd == -1)
		return;
		
	cursor.setPosition(matchEnd+1, QTextCursor::MoveAnchor);
	r = cursorRect( cursor );
		p.drawText(r.x()-1, r.y(), r.width(), r.height(), Qt::AlignLeft | Qt::AlignVCenter, matchChar );
}

void	LinesEditor::printMargins( QPainter &p )
{
	int lineLocation;
	
	p.setFont( document()->defaultFont() );
	p.setPen( whiteSpaceColor );
	lineLocation = p.fontMetrics().width( " " ) * printMarginWidth + 0;
	p.drawLine( lineLocation, 0, lineLocation, height() );
}

void LinesEditor::widgetToBottom( QWidget *w )
{
	QRect r1 = viewport()->geometry();
	QRect r2 = w->geometry();

	int i = r2.height();
	r2.setX( r1.left() + 5 );
	r2.setY( r1.height() - i - 5 );
	r2.setWidth( r1.width() - 10 );
	r2.setHeight( i );
	
	w->setGeometry(r2);
	w->show();	
}

void LinesEditor::widgetToTop( QWidget *w )
{
	QRect r1 = viewport()->geometry();
	QRect r2 = w->geometry();

	int i = r2.height();
	r2.setX( r1.left() + 5 );
	r2.setY( 5 );
	r2.setWidth( r1.width() - 10 );
	r2.setHeight( i );
	
	w->setGeometry(r2);
	w->show();	
}

bool	LinesEditor::handleKeyPressEvent( QKeyEvent *event )
{
	QTextCursor cursor = textCursor();
	int i,j;
	QString s;
	
	if (event->key() == Qt::Key_Delete)
	{
		if (cursor.hasSelection())
			return false;
		
		i = cursor.position() - cursor.block().position();
		QChar c1 = cursor.block().text()[ i ];
		j = matchingString.indexOf( c1 );
		if (j == -1)
			return false;
		
		if (j%2 == 0)
		{
			qDebug("Deleting forward");
			QChar c2 = cursor.block().text()[ i+1 ];
			if (c2 != matchingString[j+1])
				return false;
			cursor.deletePreviousChar();
			cursor.deleteChar();
		}
		else
		{
			qDebug("Deleting backward");
			QChar c2 = cursor.block().text()[ i-1 ];
			if (c2 != matchingString[j-1])
				return false;
			cursor.deletePreviousChar();
			cursor.deleteChar();
		}
		
		goto FUNCTION_END;
	}
	
	s = event->text();
	// handle only normal key presses
	if (s.isEmpty())
		return false;
	
	// don't handle if not in the matching list
	j = matchingString.indexOf( s[0] );
	if ((j == -1) || (j%2 == 1))
		return false;
	
	i = cursor.position();
	
	if (!cursor.hasSelection())
	{
		cursor.insertText( QString(matchingString[j]) );
		cursor.insertText( QString(matchingString[j+1]) );
	}
	else
	{
		QString s = cursor.selectedText();
		cursor.beginEditBlock();
		cursor.deleteChar();
		s = matchingString[j] + s + matchingString[j+1];
		cursor.insertText(s);
		cursor.endEditBlock();
	}
	cursor.setPosition( i + 1 );
	setTextCursor(cursor);
	
FUNCTION_END:
	event->accept();
	return true;
}

bool	LinesEditor::handleIndentEvent( bool forward )
{
	QTextCursor cursor = textCursor();
	if (!cursor.hasSelection())
	{
		qDebug("no selection, not handeling");
		return false;
	}
	
	return true;
}


void LinesEditor::updateMarkIcons()
{
	int x, y;
	QImage img;
	
	img = tabPixmap.toImage();
	for( x=0; x< tabPixmap.width(); x++ )
		for( y=0; y< tabPixmap.height(); y++ )
		{
			uint rgb = qRgb(  whiteSpaceColor.red(), whiteSpaceColor.green(), whiteSpaceColor.blue() );
			if (img.pixel(x,y) != 0)
				img.setPixel( x, y, rgb );
		}
	tabPixmap = QPixmap::fromImage( img );

	img = spacePixmap.toImage();
	for( x=0; x< tabPixmap.width(); x++ )
		for( y=0; y< tabPixmap.height(); y++ )
		{
			uint rgb = qRgb(  whiteSpaceColor.red(), whiteSpaceColor.green(), whiteSpaceColor.blue() );
			if (img.pixel(x,y) != 0)
				img.setPixel( x, y, rgb );
		}
	spacePixmap = QPixmap::fromImage( img );
}