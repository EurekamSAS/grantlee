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

#ifndef IFNODE_H
#define IFNODE_H

#include "node.h"

#include <QtCore/QSharedPointer>

using namespace Grantlee;

class IfNodeFactory : public AbstractNodeFactory
{
  Q_OBJECT
public:
  IfNodeFactory();

  Node *getNode(const Grantlee::Token &tag, Parser *p) const override;
};

class IfToken;

class GRANTLEE_TEMPLATES_EXPORT IfNode : public Node
{
  Q_OBJECT
public:
  IfNode(const Grantlee::Token &token, QObject *parent = {});

  void
  setNodelistConditions(const QVector<QPair<QSharedPointer<IfToken>, NodeList>>
                            &conditionNodelists);

  void render(OutputStream *stream, Context *c) const override;
  const QVector<QPair<QSharedPointer<IfToken>, NodeList>>  conditionNodeLists() const;
private:
  QVector<QPair<QSharedPointer<IfToken>, NodeList>> mConditionNodelists;
};

#endif
