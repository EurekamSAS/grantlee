/*
  This file is part of the Grantlee template system.

  Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 3 only, as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License version 3 for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "parser.h"

#include <QFile>

#include "taglibraryinterface.h"
#include "template.h"
#include "engine.h"
#include "filter.h"
#include "exception.h"

#include "grantlee_version.h"

using namespace Grantlee;

namespace Grantlee
{

class ParserPrivate
{
public:
  ParserPrivate( Parser *parser, const QList<Token> &tokenList )
      : q_ptr( parser ),
      m_tokenList( tokenList ) {

  }

  NodeList extendNodeList( NodeList list, Node *node );

  /**
    Parses the template to create a Nodelist.
    The given @p parent is the parent of each node in the returned list.
  */
  NodeList parse( QObject *parent, const QStringList &stopAt = QStringList() );

  void openLibrary( TagLibraryInterface * library );
  Q_DECLARE_PUBLIC( Parser )
  Parser *q_ptr;

  QList<Token> m_tokenList;

  QHash<QString, AbstractNodeFactory*> m_nodeFactories;
  QHash<QString, Filter::Ptr> m_filters;

  NodeList m_nodeList;
};

}

void ParserPrivate::openLibrary( TagLibraryInterface *library )
{
  QHashIterator<QString, AbstractNodeFactory*> nodeIt( library->nodeFactories() );
  while ( nodeIt.hasNext() ) {
    nodeIt.next();
    m_nodeFactories.insert( nodeIt.key(), nodeIt.value() );
  }
  QHashIterator<QString, Filter*> filterIt( library->filters() );
  while ( filterIt.hasNext() ) {
    filterIt.next();
    Filter::Ptr f = Filter::Ptr( filterIt.value() );
    m_filters.insert( filterIt.key(), f );
  }
}

Parser::Parser( const QList<Token> &tokenList, QObject *parent )
    : QObject( parent ), d_ptr( new ParserPrivate( this, tokenList ) )
{
  Q_D( Parser );

  Engine *engine = Engine::instance();

  TemplateImpl *ti = qobject_cast<TemplateImpl *>( parent );

  engine->loadDefaultLibraries( ti->state() );
  foreach( const QString &libraryName, engine->defaultLibraries()) {
    TagLibraryInterface *library = engine->loadLibrary( libraryName, ti->state() );
    if(!library)
      continue;
    d->openLibrary( library );
  }
}

Parser::~Parser()
{
  // Don't delete filters here because filters must out-live the parser in the
  // filter expressions.
  qDeleteAll( d_ptr->m_nodeFactories );
  delete d_ptr;
}


void Parser::setTokens( const QList< Token >& tokenList )
{
  Q_D( Parser );
  d->m_tokenList = tokenList;
}


void Parser::loadLib( const QString &name )
{
  Q_D( Parser );
  TemplateImpl *ti = qobject_cast<TemplateImpl *>( parent() );
  TagLibraryInterface *library = Engine::instance()->loadLibrary( name, ti->state() );
  if ( !library )
    return;
  d->openLibrary( library );
}

NodeList ParserPrivate::extendNodeList( NodeList list, Node *node )
{
  if ( node->mustBeFirst() && list.containsNonText() ) {
    throw Grantlee::Exception( TagSyntaxError, QString(
        "Node appeared twice in template: %1" ).arg( node->metaObject()->className() ) );
  }

  list.append( node );
  return list;
}

void Parser::skipPast( const QString &tag )
{
  while ( hasNextToken() ) {
    Token token = nextToken();
    if ( token.tokenType == BlockToken && token.content.trimmed() == tag )
      return;
  }
  throw Grantlee::Exception( UnclosedBlockTagError, QString( "No closing tag found for %1" ).arg( tag ) );
}

Filter::Ptr Parser::getFilter( const QString &name ) const
{
  Q_D( const Parser );
  if ( !d->m_filters.contains( name ) )
    throw Grantlee::Exception( UnknownFilterError, QString( "Unknown filter: %1" ).arg( name ) );
  return d->m_filters.value( name );
}

NodeList Parser::parse( Node *parent, const QString &stopAt )
{
  Q_D( Parser );
  return d->parse( parent, QStringList() << stopAt );
}

NodeList Parser::parse( TemplateImpl *parent, const QStringList &stopAt )
{
  Q_D( Parser );
  return d->parse( parent, stopAt );
}

NodeList Parser::parse( Node *parent, const QStringList &stopAt )
{
  Q_D( Parser );
  return d->parse( parent, stopAt );
}

NodeList ParserPrivate::parse( QObject *parent, const QStringList &stopAt )
{
  Q_Q( Parser );
  NodeList nodeList;

  while ( q->hasNextToken() ) {
    Token token = q->nextToken();
    if ( token.tokenType == TextToken ) {
      nodeList = extendNodeList( nodeList, new TextNode( token.content, parent ) );
    } else if ( token.tokenType == VariableToken ) {
      if ( token.content.isEmpty() ) {
        // Error. Empty variable
        QString message;
        if ( q->hasNextToken() )
          message = QString( "Empty variable before \"%1\"" ).arg( q->nextToken().content );
        else
          message = QString( "Empty variable at end of input." );

        throw Grantlee::Exception( EmptyVariableError, message );
      }
      FilterExpression filterExpression( token.content, q );

      nodeList = extendNodeList( nodeList, new VariableNode( filterExpression, parent ) );
    } else if ( token.tokenType == BlockToken ) {
      if ( stopAt.contains( token.content ) ) {
        // put the token back.
        q->prependToken( token );
        return nodeList;
      }

      QStringList tagContents = token.content.split( ' ' );
      if ( tagContents.size() == 0 ) {
        QString message;
        if ( q->hasNextToken() )
          message = QString( "Empty block tag before \"%1\"" ).arg( q->nextToken().content );
        else
          message = QString( "Empty block tag at end of input." );

        throw Grantlee::Exception( EmptyBlockTagError, message );
      }
      QString command = tagContents.at( 0 );
      AbstractNodeFactory *nodeFactory = m_nodeFactories[command];

      // unknown tag.
      if ( !nodeFactory ) {
        throw Grantlee::Exception( InvalidBlockTagError, QString( "Unknown tag: \"%1\"" ).arg( command ) );
      }

      // TODO: Make getNode take a Token instead?
      Node *n = nodeFactory->getNode( token.content, q );

      if ( !n ) {
        throw Grantlee::Exception( EmptyBlockTagError, QString( "Failed to get node from %1" ).arg( command ) );
      }

      n->setParent( parent );

      nodeList = extendNodeList( nodeList, n );
    }
  }

  if ( !stopAt.isEmpty() ) {
    QString message = QString( "Unclosed tag in template. Expected one of: (%1)" ).arg( stopAt.join( " " ) );
    throw Grantlee::Exception( UnclosedBlockTagError, message );
  }

  return nodeList;

}

bool Parser::hasNextToken() const
{
  Q_D( const Parser );
  return d->m_tokenList.size() > 0;
}

Token Parser::nextToken()
{
  Q_D( Parser );
  return d->m_tokenList.takeAt( 0 );
}

void Parser::deleteNextToken()
{
  Q_D( Parser );
  if ( !d->m_tokenList.isEmpty() )
    d->m_tokenList.removeAt( 0 );
}

void Parser::prependToken( const Token &token )
{
  Q_D( Parser );
  d->m_tokenList.prepend( token );
}

#include "parser.moc"
