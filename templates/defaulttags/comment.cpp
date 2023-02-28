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

#include "comment.h"

#include "parser.h"

CommentNodeFactory::CommentNodeFactory() = default;

Node *CommentNodeFactory::getNode(const Grantlee::Token &tag, Parser *p) const
{
  Q_UNUSED(tag.content);

  p->skipPast(QStringLiteral("endcomment"), tag);

  return new CommentNode(tag, p);
}

CommentNode::CommentNode(const Grantlee::Token &token, QObject *parent) : Node(token, parent) {}

void CommentNode::render(OutputStream *stream, Context *c) const
{
  Q_UNUSED(stream);
  Q_UNUSED(c);
}
