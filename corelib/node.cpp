/*
  This file is part of the Grantlee template system.

  Copyright (c) 2009,2010 Stephen Kelly <steveire@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either version
  2 of the Licence, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "node.h"

#include "util_p.h"
#include "template.h"

using namespace Grantlee;

namespace Grantlee
{

class NodePrivate
{
  NodePrivate( Node *node )
      : q_ptr( node ) {

  }
  Q_DECLARE_PUBLIC( Node )
  Node *q_ptr;
};

class AbstractNodeFactoryPrivate
{
  AbstractNodeFactoryPrivate( AbstractNodeFactory *factory )
      : q_ptr( factory ) {

  }

  Q_DECLARE_PUBLIC( AbstractNodeFactory )
  AbstractNodeFactory *q_ptr;
};

}

Node::Node( QObject *parent ) : QObject( parent ),
    d_ptr( new NodePrivate( this ) )
{

}

Node::~Node()
{
  delete d_ptr;
}

void Node::streamValueInContext( OutputStream *stream, const QVariant& input, Context* c )
{
  Grantlee::SafeString inputString;
  if ( input.type() == QVariant::List ) {
    inputString = Util::toString( input.toList() );
  } else {
    inputString = Util::getSafeString( input );
  }
  if ( c->autoEscape() && !inputString.isSafe() )
    inputString.setNeedsEscape( true );

  ( *stream ) << inputString;
}

TemplateImpl* Node::containerTemplate() const
{
  QObject *_parent = parent();
  TemplateImpl *ti = qobject_cast<TemplateImpl *>( _parent );
  while ( _parent && !ti ) {
    _parent = _parent->parent();
    ti = qobject_cast<TemplateImpl *>( _parent );
  }
  Q_ASSERT( ti );
  return ti;
}


NodeList::NodeList()
    : QList<Grantlee::Node*>(), m_containsNonText( false )
{

}

NodeList::NodeList( const NodeList &list )
    : QList<Grantlee::Node*>( list )
{
  m_containsNonText = list.m_containsNonText;
}

NodeList::NodeList( const QList<Grantlee::Node *> &list )
    : QList<Grantlee::Node*>( list )
{
  Q_FOREACH( Grantlee::Node *node, list ) {
    TextNode *textNode = qobject_cast<TextNode *>( node );
    if ( !textNode ) {
      m_containsNonText = true;
      return;
    }
  }
  m_containsNonText = false;
}

NodeList::~NodeList()
{
}

void NodeList::append( Grantlee::Node *node )
{
  if ( !m_containsNonText ) {
    TextNode *textNode = qobject_cast<TextNode *>( node );
    if ( !textNode )
      m_containsNonText = true;
  }

  QList<Grantlee::Node *>::append( node );
}

void NodeList::append( QList<Grantlee::Node*> nodeList )
{
  if ( !m_containsNonText ) {
    Q_FOREACH( Grantlee::Node *node, nodeList ) {
      TextNode *textNode = qobject_cast<TextNode *>( node );
      if ( !textNode ) {
        m_containsNonText = true;
        break;
      }
    }
  }

  QList<Grantlee::Node *>::append( nodeList );
}

bool NodeList::containsNonText() const
{
  return m_containsNonText;
}

void NodeList::render( OutputStream *stream, Context *c )
{
  if ( c->isMutating() )
    return mutableRender( stream, c );

  for ( int i = 0; i < this->size(); ++i ) {
    this->at( i )->render( stream, c );
  }

  return;
}

void NodeList::mutableRender( OutputStream *stream, Context *c )
{
  QString renderedTemplate;
  QString renderedNode;

  QTextStream textStream( &renderedNode );
  QSharedPointer<OutputStream> nodeStream = stream->clone( &textStream );

  QList<Grantlee::Node*>::iterator it;
  QList<Grantlee::Node*>::iterator first = begin();
  QList<Grantlee::Node*>::iterator last = end();

  for ( it = first; it != last; ++it ) {
    renderedNode.clear();
    Grantlee::Node *node = *it;
    node->render( nodeStream.data(), c );
    renderedTemplate += renderedNode;
    bool isPersistent = node->isPersistent();
    if ( it != first ) {
      Grantlee::Node *lastNode = *( it - 1 );
      TextNode *textNode = qobject_cast<TextNode*>( lastNode );
      if ( textNode && ( !isPersistent || node->isRepeatable() ) ) {
        textNode->appendContent( renderedNode );
      }
      if ( *it == *( last - 1 ) ) {
        break;
      }
      if ( !isPersistent && !lastNode->isPersistent() ) {
        it = erase( it );
        // TODO: This --it is broken. Fix this iteration.
        --it;
      }
    }
  }
  ( *stream ) << renderedTemplate;
}

AbstractNodeFactory::AbstractNodeFactory( QObject *parent )
    : QObject( parent ), d_ptr( new AbstractNodeFactoryPrivate( this ) )
{

}

AbstractNodeFactory::~AbstractNodeFactory()
{
  delete d_ptr;
}

QList< FilterExpression > AbstractNodeFactory::getFilterExpressionList( const QStringList &list, Parser *p ) const
{
  QList<FilterExpression> fes;
  QListIterator<QString> it( list );
  while ( it.hasNext() ) {
    QString varString = it.next();
    fes << FilterExpression( varString, p );
  }
  return fes;
}

QStringList AbstractNodeFactory::smartSplit( const QString &str ) const
{
  QRegExp r( "(\"(?:[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*)\"|\\\'(?:[^\\\'\\\\]*(?:\\\\.[^\\\'\\\\]*)*)\\\'|[^\\s]+)" );

  QStringList l;
  int count = 0;
  int pos = 0;
  while (( pos = r.indexIn( str, pos ) ) != -1 ) {
    ++count;
    pos += r.matchedLength();
    l << r.capturedTexts().at( 0 );
  }

  return l;
}

TextNode::TextNode( const QString &content, QObject *parent )
    : Node( parent ), m_content( content )
{
}

VariableNode::VariableNode( const FilterExpression &fe, QObject *parent )
    : Node( parent ), m_filterExpression( fe )
{
}

void VariableNode::render( OutputStream *stream, Context *c )
{
  QVariant v = m_filterExpression.resolve( c );
  if ( !v.isValid() )
    return;
  streamValueInContext( stream, v, c );
}

#include "node.moc"
