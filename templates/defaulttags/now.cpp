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

#include "now.h"

#include "../lib/exception.h"
#include "parser.h"

#include <QtCore/QDateTime>

NowNodeFactory::NowNodeFactory() = default;

Node *NowNodeFactory::getNode(const Grantlee::Token &tag, Parser *p) const
{
  auto expr = tag.content.split(QLatin1Char('"'),
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
                               QString::KeepEmptyParts
#else
                               Qt::KeepEmptyParts
#endif
  );

  if (expr.size() != 3) {
    throw Grantlee::Exception(TagSyntaxError,
                              QStringLiteral("now tag takes one argument"),
                              tag.linenumber,
                              tag.columnnumber,
                              tag.content);
  }

  auto formatString = expr.at(1);

  return new NowNode(tag, formatString, p);
}

NowNode::NowNode(const Grantlee::Token &token, const QString &formatString, QObject *parent)
    : Node(token, parent), m_formatString(formatString)
{
}

void NowNode::render(OutputStream *stream, Context *c) const
{
  Q_UNUSED(c)
  (*stream) << QDateTime::currentDateTime().toString(m_formatString);
}
