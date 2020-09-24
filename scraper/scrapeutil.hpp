#pragma once

#include "gumbo.h"

#include <functional>
#include <QVector>
#include <QString>

namespace ScrapeUtil
{

using NodeFilter = std::function<bool(GumboElement*)>;
QVector<GumboElement*> collectElements(GumboNode *node, const NodeFilter &filter, bool recursive = false);
QVector<GumboElement*> collectElements(GumboElement *elem, const NodeFilter &filter, bool recursive = false);

QVector<GumboElement*> collectElements(GumboNode *node, GumboTag tag, bool recursive = false);
QVector<GumboElement*> collectElements(GumboElement *elem, GumboTag tag, bool recursive = false);

QStringList collectTexts(GumboElement *elem);
QString getFirstText(GumboElement *elem);

QByteArray attributeValue(GumboElement *elem, const QByteArray &name);

} // namespace ScrapeUtil
