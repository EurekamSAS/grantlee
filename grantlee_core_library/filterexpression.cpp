/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>
*/

#include "filterexpression.h"

#include "parser.h"
#include "grantlee.h"
#include "filter.h"
#include "util_p.h"

using namespace Grantlee;

namespace Grantlee
{
class FilterExpressionPrivate
{
  FilterExpressionPrivate(FilterExpression *fe, int error)
    : q_ptr(fe), m_error(error)
  {
  }

  Variable m_variable;
  QList<ArgFilter> m_filters;
  int m_error;

  Q_DECLARE_PUBLIC(FilterExpression)
  FilterExpression *q_ptr;
};

}

static const char * FILTER_SEPARATOR = "|";
static const char * FILTER_ARGUMENT_SEPARATOR = ":";
static const char * VARIABLE_ATTRIBUTE_SEPARATOR = ".";
static const char * ALLOWED_VARIABLE_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_\\.";

static const char * varChars = "\\w\\." ;
static const char * numChars = "[-+\\.]?\\d[\\d\\.e]*";
static const QString filterSep( QRegExp::escape( FILTER_SEPARATOR ) );
static const QString argSep( QRegExp::escape( FILTER_ARGUMENT_SEPARATOR ) );
static const QString i18nOpen( QRegExp::escape( "_(" ) );
static const QString i18nClose( QRegExp::escape( ")" ) );

static const QString constantString = QString(
    "(?:%3%1%4|"
    "%3%2%4|"
    "%1|"
    "%2)"
    )
    .arg("\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\"")
    .arg("\'[^\'\\\\]*(?:\\\\.[^\'\\\\]*)*\'")
    .arg(i18nOpen)
    .arg(i18nClose)
    ;

static const QString filterRawString = QString(
          "^%1|"                      /* Match "strings" and 'strings' incl i18n */
          "^[%2]+|"                   /* Match variables */
          "%3|"                       /* Match numbers */
          "%4\\w+|"                   /* Match filters */
          "%5(?:%1|[%2]+|%3|%4\\w+)"  /* Match arguments to filters, which may be strings, */
          )                           /*   variables, numbers or another filter in the chain */
 /* 1 */  .arg(constantString)
 /* 2 */  .arg(varChars)
 /* 3 */  .arg(numChars)
 /* 4 */  .arg(filterSep)
 /* 5 */  .arg(argSep);

static const QRegExp sFilterRe(filterRawString);

FilterExpression::FilterExpression(const QString &varString, Parser *parser)
  : d_ptr(new FilterExpressionPrivate(this, NoError))
{
  Q_D(FilterExpression);

  int pos = 0;
  int lastPos = 0;
  int len;
  QString subString;

  if (varString.contains("\n"))
  {
    // error.
  }
  
  
  QString vs = varString.trimmed();
  
  while ((pos = sFilterRe.indexIn(vs, pos)) != -1) {
    len = sFilterRe.matchedLength();
    subString = vs.mid(pos, len);
    int ssSize = subString.size();
    if (subString.startsWith(FILTER_SEPARATOR))
    {
      subString = subString.right(ssSize - 1);
      Filter *f = parser->getFilter(subString);
      if (f)
        d->m_filters << qMakePair<Filter*, Variable>(f, Variable());
      else
      {
        d->m_error = TagSyntaxError;
        return;
      }
    }
    else if (subString.startsWith(FILTER_ARGUMENT_SEPARATOR))
    {
      subString = subString.right(ssSize - 1);
      int lastFilter = d->m_filters.size();
      d->m_filters[lastFilter -1].second = Variable(subString);
    } else if (subString.startsWith("_(") && subString.endsWith(")"))
    {
      // Token is _("translated"): subString.mid(1, ssSize - 1 -2);
    } else if (subString.startsWith("\"") && subString.endsWith("\""))
    {
      // Token is a "constant"
      d->m_variable = Variable(subString);
    } else
    {
      // Arg is a variable
      d->m_variable = Variable(subString);
    }
    
    pos += len;
    lastPos = pos;
  }
  QString remainder = vs.right( vs.size() - lastPos);
  if (remainder.size() > 0)
  {
    d->m_error = TagSyntaxError;
  }
}

int FilterExpression::error() const
{
  Q_D(const FilterExpression);
  return d->m_error;
}


FilterExpression::FilterExpression(const FilterExpression &other)
  : d_ptr(new FilterExpressionPrivate(this, other.d_ptr->m_error))
{
  d_ptr->m_variable = other.d_ptr->m_variable;
  d_ptr->m_filters = other.d_ptr->m_filters;
}

FilterExpression::FilterExpression()
  : d_ptr(new FilterExpressionPrivate(this, NoError))
{
  Q_D(FilterExpression);
}

FilterExpression::~FilterExpression()
{
  delete d_ptr;
}

Variable FilterExpression::variable() const
{
  Q_D(const FilterExpression);
  return d->m_variable;
}

FilterExpression &FilterExpression::operator=(const FilterExpression &other)
{
  d_ptr->m_variable = other.d_ptr->m_variable;
  d_ptr->m_filters = other.d_ptr->m_filters;
  d_ptr->m_error = other.d_ptr->m_error;
  return *this;
}

QVariant FilterExpression::resolve(Context *c) const
{
  Q_D(const FilterExpression);
  QVariant var = d->m_variable.resolve(c);
  foreach(ArgFilter argfilter, d->m_filters)
  {
    var = argfilter.first->doFilter(var, argfilter.second.resolve(c).toString());
  }
  return var;
}

// void FilterExpression::begin(Context *c, int reversed)
// {
//   m_position = 0;
//   QVariant var = resolve(c);
//   if (!var.isValid())
//     return;
//
//   if (var.type() == QVariant::String)
//   {
//
//   } else if (var.userType() > 0)
//   {
//     // A registered user type. Signal error.
// //     void *obj = QMetaType::construct(var.userType(), (void *)var );
//
//     // Instead of all this messing, I can simply require that an object can't be iterated over.
//     // Iterate over a property instead.
//     // Or I can allow iterating over an objects children(), which is a QList<QObject>
//
// //     QMetaType::destroy(var.userType(), obj);
//     //   TODO: see if I can use QMetaType::construct for user defined types.
//   }
//   else
//   {
//     m_variantList.append(var);
//   }
// }

QVariantList FilterExpression::toList(Context *c) const
{
  QVariant var = resolve(c);
  if (!var.isValid())
    return QVariantList();

  if (var.type() == QVariant::List)
  {
    return var.toList();
  }
  if (var.type() == QVariant::String)
  {
    QString s = var.toString();

    QString::iterator i;
    QVariantList list;
    for (i = s.begin(); i != s.end(); ++i)
    {
      list << *i;
    }
    return list;
  } else if (var.userType() == QMetaType::QObjectStar)
  {
    // A registered user type. Signal error.
//     void *obj = QMetaType::construct(var.userType(), (void *)var );

    // Instead of all this messing, I can simply require that an object can't be iterated over.
    // Iterate over a property instead.
    // Or I can allow iterating over an objects children(), which is a QList<QObject *>

//     QMetaType::destroy(var.userType(), obj);
    //   TODO: see if I can use QMetaType::construct for user defined types.
  }
  else
  {
//     QVariantList list;
//     list << var;
//     return list;
    return QVariantList() << var;
//     m_variantList.append(var);
  }

}

bool FilterExpression::isTrue(Context *c) const
{
  return Util::variantIsTrue(resolve(c));
}

// QVariant FilterExpression::next()
// {
//   return m_variantList[m_position++];
//
// //   if (!var.isValid())
// //     return QVariant();
// //
// //   m_iterVariant = var;
// }

