/*
  This file is part of the Grantlee template system.

  Copyright (c) 2009,2010 Stephen Kelly <steveire@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either version
  2.1 of the Licence, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "include.h"

#include "block.h"
#include "blockcontext.h"
#include "engine.h"
#include "exception.h"
#include "parser.h"
#include "rendercontext.h"
#include "template.h"
#include "util.h"

IncludeNodeFactory::IncludeNodeFactory() = default;

Node *IncludeNodeFactory::getNode(const Grantlee::Token &tag, Parser *p) const
{
  auto expr = smartSplit(tag.content);

  if (expr.size() != 2)
    throw Grantlee::Exception(
        TagSyntaxError,
        QStringLiteral("Error: Include tag takes only one argument"),tag.linenumber,tag.columnnumber,tag.content);

  auto includeName = expr.at(1);
  auto size = includeName.size();

  if ((includeName.startsWith(QLatin1Char('"'))
       && includeName.endsWith(QLatin1Char('"')))
      || (includeName.startsWith(QLatin1Char('\''))
          && includeName.endsWith(QLatin1Char('\'')))) {
    return new ConstantIncludeNode(tag, includeName.mid(1, size - 2));
  }
  return new IncludeNode(tag, FilterExpression(includeName, p), p);
}

IncludeNode::IncludeNode(const Grantlee::Token& token, const FilterExpression &fe, QObject *parent)
    : Node(token, parent), m_filterExpression(fe)
{
}

void IncludeNode::render(OutputStream *stream, Context *c) const
{
  QString filename = getSafeString(m_filterExpression.resolve(c));

  auto ti = containerTemplate();

  auto t = ti->engine()->loadByName(filename);

  if (!t)
    throw Grantlee::Exception(
        TagSyntaxError, QStringLiteral("Template not found %1").arg(filename),-1,-1,QString());

  if (t->error())
    throw Grantlee::Exception(t->error(), t->errorString(),-1,-1,QString());

  t->render(stream, c);

  if (t->error())
    throw Grantlee::Exception(t->error(), t->errorString(),-1,-1,QString());
}

ConstantIncludeNode::ConstantIncludeNode(const Grantlee::Token &token, const QString filename, QObject *parent)
    : Node(token, parent)
{
  m_name = filename;
}

void ConstantIncludeNode::render(OutputStream *stream, Context *c) const
{
  auto ti = containerTemplate();

  auto t = ti->engine()->loadByName(m_name);
  if (!t)
    throw Grantlee::Exception(
        TagSyntaxError, QStringLiteral("Template not found %1").arg(m_name),-1,-1,QString());

  if (t->error())
    throw Grantlee::Exception(t->error(), t->errorString(),-1,-1,QString());

  t->render(stream, c);

  if (t->error())
    throw Grantlee::Exception(t->error(), t->errorString(),-1,-1,QString());

  QVariant &variant = c->renderContext()->data(nullptr);
  auto blockContext = variant.value<BlockContext>();
  auto nodes = t->findChildren<BlockNode *>();
  blockContext.remove(nodes);
  variant.setValue(blockContext);
}
