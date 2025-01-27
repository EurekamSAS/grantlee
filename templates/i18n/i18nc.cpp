/*
  This file is part of the Grantlee template system.

  Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>

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

#include "i18nc.h"

#include <QtCore/QStringList>

#include "abstractlocalizer.h"
#include "engine.h"
#include "exception.h"
#include "parser.h"
#include "template.h"

#include <QtCore/QDebug>
#include <complex>
#include <util.h>

I18ncNodeFactory::I18ncNodeFactory() = default;

Node *I18ncNodeFactory::getNode(const Grantlee::Token &tag, Parser *p) const
{
  auto expr = smartSplit(tag.content);

  if (expr.size() < 3)
    throw Grantlee::Exception(
        TagSyntaxError,
        QStringLiteral("Error: i18nc tag takes at least two arguments"),
              tag.linenumber,
              tag.columnnumber,
              tag.content);

  auto contextText = expr.at(1);

  if (!(contextText.startsWith(QLatin1Char('"'))
        && contextText.endsWith(QLatin1Char('"')))
      && !(contextText.startsWith(QLatin1Char('\''))
           && contextText.endsWith(QLatin1Char('\'')))) {
    throw Grantlee::Exception(
        TagSyntaxError,
        QStringLiteral(
            "Error: i18nc tag first argument must be a static string."),
                  tag.linenumber,
                  tag.columnnumber,
                  tag.content);
  }
  contextText = contextText.mid(1, contextText.size() - 2);

  auto sourceText = expr.at(2);

  if (!(sourceText.startsWith(QLatin1Char('"'))
        && sourceText.endsWith(QLatin1Char('"')))
      && !(sourceText.startsWith(QLatin1Char('\''))
           && sourceText.endsWith(QLatin1Char('\'')))) {
    throw Grantlee::Exception(
        TagSyntaxError,
        QStringLiteral(
            "Error: i18nc tag second argument must be a static string."),
                  tag.linenumber,
                  tag.columnnumber,
                  tag.content);
  }
  sourceText = sourceText.mid(1, sourceText.size() - 2);

  QList<FilterExpression> feList;
  for (auto i = 3; i < expr.size(); ++i) {
    feList.append(FilterExpression(expr.at(i), p));
  }

  return new I18ncNode(tag, sourceText, contextText, feList);
}

I18ncVarNodeFactory::I18ncVarNodeFactory() = default;

Grantlee::Node *I18ncVarNodeFactory::getNode(const Grantlee::Token &tag,
                                             Parser *p) const
{
  auto expr = smartSplit(tag.content);

  if (expr.size() < 5)
    throw Grantlee::Exception(
        TagSyntaxError,
        QStringLiteral("Error: i18nc_var tag takes at least four arguments"),
              tag.linenumber,
              tag.columnnumber,
              tag.content);

  auto contextText = expr.at(1);

  if (!(contextText.startsWith(QLatin1Char('"'))
        && contextText.endsWith(QLatin1Char('"')))
      && !(contextText.startsWith(QLatin1Char('\''))
           && contextText.endsWith(QLatin1Char('\'')))) {
    throw Grantlee::Exception(
        TagSyntaxError,
        QStringLiteral(
            "Error: i18nc_var tag first argument must be a static string."),
                  tag.linenumber,
                  tag.columnnumber,
                  tag.content);
  }
  contextText = contextText.mid(1, contextText.size() - 2);

  auto sourceText = expr.at(2);

  if (!(sourceText.startsWith(QLatin1Char('"'))
        && sourceText.endsWith(QLatin1Char('"')))
      && !(sourceText.startsWith(QLatin1Char('\''))
           && sourceText.endsWith(QLatin1Char('\'')))) {
    throw Grantlee::Exception(
        TagSyntaxError,
        QStringLiteral(
            "Error: i18nc_var tag second argument must be a static string."),
                  tag.linenumber,
                  tag.columnnumber,
                  tag.content);
  }
  sourceText = sourceText.mid(1, sourceText.size() - 2);

  QList<FilterExpression> feList;
  for (auto i = 3; i < expr.size() - 2; ++i) {
    feList.append(FilterExpression(expr.at(i), p));
  }

  auto resultName = expr.last();

  return new I18ncVarNode(tag, sourceText, contextText, feList, resultName);
}

I18ncNode::I18ncNode(const Grantlee::Token &token, const QString &sourceText, const QString &context,
                     const QList<Grantlee::FilterExpression> &feList,
                     QObject *parent)
    : Node(token, parent), m_sourceText(sourceText), m_context(context),
      m_filterExpressionList(feList)
{
}

void I18ncNode::render(OutputStream *stream, Context *c) const
{
  QVariantList args;
  for (const FilterExpression &fe : m_filterExpressionList)
    args.append(fe.resolve(c));
  auto resultString
      = c->localizer()->localizeContextString(m_sourceText, m_context, args);

  streamValueInContext(stream, resultString, c);
}

I18ncVarNode::I18ncVarNode(const Grantlee::Token &token, const QString &sourceText, const QString &context,
                           const QList<Grantlee::FilterExpression> &feList,
                           const QString &resultName, QObject *parent)
    : Node(token, parent), m_sourceText(sourceText), m_context(context),
      m_filterExpressionList(feList), m_resultName(resultName)
{
}

void I18ncVarNode::render(OutputStream *stream, Context *c) const
{
  Q_UNUSED(stream)
  QVariantList args;
  for (const FilterExpression &fe : m_filterExpressionList)
    args.append(fe.resolve(c));
  auto resultString
      = c->localizer()->localizeContextString(m_sourceText, m_context, args);

  c->insert(m_resultName, resultString);
}
