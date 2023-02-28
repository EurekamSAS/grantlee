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

#include "regroup.h"

#include "../lib/exception.h"
#include "parser.h"
#include "util.h"

RegroupNodeFactory::RegroupNodeFactory() = default;

Node *RegroupNodeFactory::getNode(const Grantlee::Token &tag, Parser *p) const
{
  auto expr = tag.content.split(QLatin1Char(' '));

  if (expr.size() != 6) {
    throw Grantlee::Exception(
        TagSyntaxError, QStringLiteral("widthratio takes five arguments"),
                  tag.linenumber,
                  tag.columnnumber,
                  tag.content);
  }
  FilterExpression target(expr.at(1), p);
  if (expr.at(2) != QStringLiteral("by")) {
    throw Grantlee::Exception(TagSyntaxError,
                              QStringLiteral("second argument must be 'by'"),
                              tag.linenumber,
                              tag.columnnumber,
                              tag.content);
  }

  if (expr.at(4) != QStringLiteral("as")) {
    throw Grantlee::Exception(TagSyntaxError,
                              QStringLiteral("fourth argument must be 'as'"),
                              tag.linenumber,
                              tag.columnnumber,
                              tag.content);
  }

  FilterExpression expression(
      QStringLiteral("\"") + expr.at(3) + QStringLiteral("\""), p);

  auto name = expr.at(5);

  return new RegroupNode(tag, target, expression, name, p);
}

RegroupNode::RegroupNode(const Grantlee::Token &token,
                         const FilterExpression &target,
                         const FilterExpression &expression,
                         const QString &varName, QObject *parent)
    : Node(token, parent), m_target(target), m_expression(expression),
      m_varName(varName)
{
}

void RegroupNode::render(OutputStream *stream, Context *c) const
{
  Q_UNUSED(stream)
  auto objList = m_target.toList(c);
  if (objList.isEmpty()) {
    c->insert(m_varName, QVariantHash());
    return;
  }

  // What's going on?
  //
  // objList is a flat list of objects with a common parameter. For example,
  // Person objects with
  // a name parameter. The list is already sorted.
  // Say the objList contains ["David Beckham", "David Blain", "Keira
  // Nightly"]
  // etc.
  // We want to regroup the list into separate lists of people with the same
  // first name.
  // ie objHash should be: {"David": ["David Beckham", "David Blain"],
  // "Keira":
  // ["Keira Nightly"]}
  //
  // We then insert the objHash into the Context ready for rendering later in
  // a
  // for loop.

  QVariantList contextList;
  const QString keyName = getSafeString(m_expression.resolve(c));
  for (auto &var : objList) {
    c->push();
    c->insert(QStringLiteral("var"), var);
    const QString key = getSafeString(
        FilterExpression(QStringLiteral("var.") + keyName, nullptr).resolve(c));
    c->pop();
    QVariantHash hash;
    if (!contextList.isEmpty()) {
      auto hashVar = contextList.last();
      hash = hashVar.value<QVariantHash>();
    }

    const auto it = hash.constFind(QStringLiteral("grouper"));
    if (it == hash.constEnd() || it.value() != key) {
      contextList.append(QVariantHash{
          {QStringLiteral("grouper"), key},
          {QStringLiteral("list"), QVariantList{var}},
      });
    } else {
      QVariantList list;
      auto itList = hash.find(QStringLiteral("list"));
      if (itList != hash.end()) {
        list = itList.value().value<QVariantList>();
        list.append(var);
        *itList = list;
      } else {
        list.append(var);
        hash.insert(QStringLiteral("list"), list);
      }
      contextList[contextList.size() - 1] = hash;
    }
  }
  c->insert(m_varName, contextList);
}
