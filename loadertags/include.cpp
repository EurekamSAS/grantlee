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

#include "include.h"

#include <QtCore/QStringList>
#include <QtPlugin>

#include "parser.h"
#include "template.h"
#include "engine.h"
#include "exception.h"

#include <QtCore/QDebug>
#include <util_p.h>

IncludeNodeFactory::IncludeNodeFactory()
{

}

Node* IncludeNodeFactory::getNode( const QString &tagContent, Parser *p ) const
{
  QStringList expr = smartSplit( tagContent );

  if ( expr.size() != 2 )
    throw Grantlee::Exception( TagSyntaxError, "Error: Include tag takes only one argument" );

  QString includeName = expr.at( 1 );
  int size = includeName.size();

  if (( includeName.startsWith( QLatin1Char( '"' ) ) && includeName.endsWith( QLatin1Char( '"' ) ) )
      || ( includeName.startsWith( QLatin1Char( '\'' ) ) && includeName.endsWith( QLatin1Char( '\'' ) ) ) ) {
    return new ConstantIncludeNode( includeName.mid( 1, size - 2 ) );
  }
  return new IncludeNode( FilterExpression( includeName, p ), p );
}

IncludeNode::IncludeNode( const FilterExpression &fe, QObject *parent )
    : Node( parent ), m_filterExpression( fe )
{
}

void IncludeNode::render( OutputStream *stream, Context *c )
{
  QString filename = Util::getSafeString( m_filterExpression.resolve( c ) );

  TemplateImpl *ti = containerTemplate();

  try {
    Template t = ti->engine()->loadByName( filename );

    if ( !t )
      return;

    t->render( stream, c );
  } catch ( Grantlee::Exception e ) {
  }
}

ConstantIncludeNode::ConstantIncludeNode( const QString &name, QObject *parent )
    : Node( parent )
{
  m_name = name;
}

void ConstantIncludeNode::render( OutputStream *stream, Context *c )
{
  TemplateImpl *ti = containerTemplate();

  try {
    Template t = ti->engine()->loadByName( m_name );
    if ( !t )
      return;

    t->render( stream, c );
  } catch ( Grantlee::Exception e ) {
  }
}
