/*
  This file is part of the Grantlee template system.

  Copyright (c) 2016 Stephen Kelly <steveire@gmail.com>

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

#ifndef IF_P_H
#define IF_P_H

#include <QtCore/QPair>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include "../lib/exception.h"
#include "filterexpression.h"
#include "node.h"
#include "util.h"

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)) && \
    (QT_VERSION  < QT_VERSION_CHECK(6, 0, 0))
namespace {
class ComparableVariant : public QVariant
{
  public:
    ComparableVariant(const QVariant &other)
      : QVariant(other) { }

    // Make QVariant::compare() public.
    int compare(const QVariant &other) const
    {
        return QVariant::compare(other);
    }
};
}
#endif

namespace Grantlee
{
class Parser;
}

class IfToken;

class IfParser
{
public:
  IfParser(Grantlee::Parser *parser, const QStringList &args);

  QSharedPointer<IfToken> parse();

  QSharedPointer<IfToken> expression(int rbp = {});

private:
  QSharedPointer<IfToken> createNode(const QString &content) const;

  QSharedPointer<IfToken> consumeToken();

private:
  Grantlee::Parser *mParser;
  QVector<QSharedPointer<IfToken>> mParseNodes;
  int mPos = 0;
  QSharedPointer<IfToken> mCurrentToken;
};

static bool contains(const QVariant &needle, const QVariant &var)
{
  if (Grantlee::isSafeString(var)) {
    return Grantlee::getSafeString(var).get().contains(
        Grantlee::getSafeString(needle));
  } else if (var.canConvert<QVariantList>()) {
    auto container = var.value<QVariantList>();
    if (Grantlee::isSafeString(needle)) {
      return container.contains(Grantlee::getSafeString(needle).get());
    }
    return container.contains(needle);
  }
  if (var.canConvert<QVariantHash>()) {
    auto container = var.value<QVariantHash>();
    if (Grantlee::isSafeString(needle)) {
      return container.contains(Grantlee::getSafeString(needle).get());
    }
    return container.contains(needle.toString());
  }
  return false;
}

class IfToken
{
public:
  enum OpCode {
    Invalid,
    Literal,
    OrCode,
    AndCode,
    NotCode,
    InCode,
    NotInCode,
    EqCode,
    NeqCode,
    GtCode,
    GteCode,
    LtCode,
    LteCode,
    Sentinal
  };

  static QSharedPointer<IfToken> makeSentinal()
  {
    return QSharedPointer<IfToken>::create(0, QString(), Sentinal);
  }

  using ArgsType = std::pair<QSharedPointer<IfToken>, QSharedPointer<IfToken>>;

  IfToken(int lbp, const QString &tokenName, OpCode opCode) : mArgs()
  {
    mLbp = lbp;
    mTokenName = tokenName;
    mOpCode = opCode;
  }

  IfToken(const Grantlee::FilterExpression &fe) : mFe(fe)
  {
    mLbp = 0;
    mTokenName = QStringLiteral("literal");
    mOpCode = Literal;
  }

  void nud(IfParser *parser);
  void led(QSharedPointer<IfToken> left, IfParser *parser);

  QVariant evaluate(Grantlee::Context *c) const;

  int lbp() const { return mLbp; }

  int mLbp;
  QString mTokenName;

  Grantlee::FilterExpression mFe;
  ArgsType mArgs;

  OpCode mOpCode;

  QString tokenName() const { return mTokenName; }
};

void IfToken::nud(IfParser *parser)
{
  switch (mOpCode) {
  default:
    break;
  case IfToken::Literal:
    return;
  case IfToken::NotCode:
    mArgs.first = parser->expression(mLbp);
    mArgs.second.clear();
    return;
  }

  throw Grantlee::Exception(
      Grantlee::TagSyntaxError,
      QStringLiteral("Not expecting '%1' in this position in if tag.")
          .arg(mTokenName),-1, -1,mTokenName);
}

void IfToken::led(QSharedPointer<IfToken> left, IfParser *parser)
{
  switch (mOpCode) {
  default:
    break;
  case IfToken::OrCode:
  case IfToken::AndCode:
  case IfToken::InCode:
  case IfToken::NotInCode:
  case IfToken::EqCode:
  case IfToken::NeqCode:
  case IfToken::GtCode:
  case IfToken::GteCode:
  case IfToken::LtCode:
  case IfToken::LteCode:
    mArgs.first = left;
    mArgs.second = parser->expression(mLbp);
    return;
  }

  throw Grantlee::Exception(
      Grantlee::TagSyntaxError,
      QStringLiteral("Not expecting '%1' as infix operator in if tag.")
          .arg(mTokenName),-1,-1,mTokenName);
}

IfParser::IfParser(Grantlee::Parser *parser, const QStringList &args)
    : mParser(parser)
{
  mParseNodes.reserve(args.size());
  if (args.size() > 1) {
    auto it = args.begin() + 1, end = args.end() - 1;
    for (; it != end; ++it) {
      if (*it == QLatin1String("not")
          && *std::next(it) == QLatin1String("in")) {
        mParseNodes.push_back(createNode(QStringLiteral("not in")));
        ++it;
        if (it == end) {
          break;
        }
      } else {
        mParseNodes.push_back(createNode(*it));
      }
    }
    mParseNodes.push_back(createNode(*it));
  }
  mPos = 0;
  mCurrentToken = consumeToken();
}

QSharedPointer<IfToken> IfParser::consumeToken()
{
  if (mPos >= mParseNodes.size()) {
    return IfToken::makeSentinal();
  }
  auto t = mParseNodes[mPos];
  ++mPos;
  return t;
}

QSharedPointer<IfToken> IfParser::expression(int rbp)
{
  auto t = mCurrentToken;
  mCurrentToken = consumeToken();
  t->nud(this);
  auto left = t;
  while (rbp < mCurrentToken->lbp()) {
    t = mCurrentToken;
    mCurrentToken = consumeToken();
    t->led(left, this);
    left = t;
  }
  return left;
}

QSharedPointer<IfToken> IfParser::parse()
{
  auto r = expression();

  if (mCurrentToken->mOpCode != IfToken::Sentinal) {
    throw Grantlee::Exception(
        Grantlee::TagSyntaxError,
        QStringLiteral("Unused '%1' at end of if expression.")
            .arg(mCurrentToken->tokenName()),-1,-1,mCurrentToken->tokenName());
  }

  return r;
}

QSharedPointer<IfToken> IfParser::createNode(const QString &content) const
{
  if (content == QLatin1String("or")) {
    return QSharedPointer<IfToken>::create(6, content, IfToken::OrCode);
  }
  if (content == QLatin1String("and")) {
    return QSharedPointer<IfToken>::create(7, content, IfToken::AndCode);
  }
  if (content == QLatin1String("in")) {
    return QSharedPointer<IfToken>::create(9, content, IfToken::InCode);
  }
  if (content == QLatin1String("not in")) {
    return QSharedPointer<IfToken>::create(9, content, IfToken::NotInCode);
  }
  if (content == QLatin1String("==")) {
    return QSharedPointer<IfToken>::create(10, content, IfToken::EqCode);
  }
  if (content == QLatin1String("!=")) {
    return QSharedPointer<IfToken>::create(10, content, IfToken::NeqCode);
  }
  if (content == QLatin1String(">")) {
    return QSharedPointer<IfToken>::create(10, content, IfToken::GtCode);
  }
  if (content == QLatin1String(">=")) {
    return QSharedPointer<IfToken>::create(10, content, IfToken::GteCode);
  }
  if (content == QLatin1String("<")) {
    return QSharedPointer<IfToken>::create(10, content, IfToken::LtCode);
  }
  if (content == QLatin1String("<=")) {
    return QSharedPointer<IfToken>::create(10, content, IfToken::LteCode);
  }
  if (content == QStringLiteral("not")) {
    return QSharedPointer<IfToken>::create(8, content, IfToken::NotCode);
  }
  return QSharedPointer<IfToken>::create(
      Grantlee::FilterExpression(content, mParser));
}

QVariant IfToken::evaluate(Grantlee::Context *c) const
{
  try {
    switch (mOpCode) {
    case Literal:
      return mFe.resolve(c);
    case OrCode:
      return Grantlee::variantIsTrue(mArgs.first->evaluate(c))
             || Grantlee::variantIsTrue(mArgs.second->evaluate(c));
    case AndCode:
      return Grantlee::variantIsTrue(mArgs.first->evaluate(c))
             && Grantlee::variantIsTrue(mArgs.second->evaluate(c));
    case NotCode:
      return !Grantlee::variantIsTrue(mArgs.first->evaluate(c));
    case InCode:
      return contains(mArgs.first->evaluate(c), mArgs.second->evaluate(c));
    case NotInCode:
      return !contains(mArgs.first->evaluate(c), mArgs.second->evaluate(c));
    case EqCode:
      return Grantlee::equals(mArgs.first->evaluate(c),
                              mArgs.second->evaluate(c));
    case NeqCode:
      return !Grantlee::equals(mArgs.first->evaluate(c),
                               mArgs.second->evaluate(c));
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    // Pre Qt-5.15, use the operators (first deprecated in Qt 5.15).
    case GtCode:
      return mArgs.first->evaluate(c) > mArgs.second->evaluate(c);
    case GteCode:
      return mArgs.first->evaluate(c) >= mArgs.second->evaluate(c);
    case LtCode:
      return mArgs.first->evaluate(c) < mArgs.second->evaluate(c);
    case LteCode:
      return mArgs.first->evaluate(c) <= mArgs.second->evaluate(c);
#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // From Qt 5.15, but before Qt 6, use a QVariant wrapper to access
    // the protected QVariant::compare() function instead.
    case GtCode:
      return ComparableVariant(mArgs.first->evaluate(c))
        .compare(mArgs.second->evaluate(c)) > 0;
    case GteCode:
      return ComparableVariant(mArgs.first->evaluate(c))
        .compare(mArgs.second->evaluate(c)) >= 0;
    case LtCode:
      return ComparableVariant(mArgs.first->evaluate(c))
        .compare(mArgs.second->evaluate(c)) < 0;
    case LteCode:
      return ComparableVariant(mArgs.first->evaluate(c))
        .compare(mArgs.second->evaluate(c)) <= 0;
#else // From Qt6, use QPartialOrdering (added in Qt 6.0).
    case GtCode:
    case GteCode:
    case LtCode:
    case LteCode: {
      auto f = mArgs.first->evaluate(c);
      auto s = mArgs.second->evaluate(c);

      auto comp = QVariant::compare(f, s);
      if (comp != QPartialOrdering::Unordered) {
        if (mOpCode == GtCode && comp == QPartialOrdering::Greater)
          return true;
        if (mOpCode == GteCode && comp != QPartialOrdering::Less)
          return true;
        if (mOpCode == LtCode && comp == QPartialOrdering::Less)
          return true;
        if (mOpCode == LteCode && comp != QPartialOrdering::Greater)
          return true;
      }
      return false;
    }
#endif
    default:
      Q_ASSERT(!"Invalid OpCode");
      return QVariant();
    }
  } catch (const Grantlee::Exception &) {
    return false;
  }
}

#endif
