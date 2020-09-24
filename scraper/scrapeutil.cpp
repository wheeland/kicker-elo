#include "scrapeutil.hpp"


namespace ScrapeUtil
{

static void collectElements(GumboNode *node, const NodeFilter &filter, QVector<GumboElement*> &into,  bool recursive)
{
    if (node->type != GUMBO_NODE_ELEMENT)
        return;

    const bool matches = filter(&node->v.element);
    if (matches) {
        into << &node->v.element;
    }
    if (!matches || recursive) {
        for (uint i = 0; i < node->v.element.children.length; ++i) {
            collectElements((GumboNode*) node->v.element.children.data[i], filter, into, recursive);
        }
    }
}

QVector<GumboElement*> collectElements(GumboNode *node, const NodeFilter &filter, bool recursive)
{
    QVector<GumboElement*> ret;
    collectElements(node, filter, ret, recursive);
    return ret;
}

QVector<GumboElement*> collectElements(GumboElement *elem, const NodeFilter &filter, bool recursive)
{
    QVector<GumboElement*> ret;
    for (uint i = 0; i < elem->children.length; ++i) {
        collectElements((GumboNode*) elem->children.data[i], filter, ret, recursive);
    }
    return ret;
}

QVector<GumboElement*> collectElements(GumboNode *node, GumboTag tag, bool recursive)
{
    return collectElements(node, [=](GumboElement *e) { return e->tag == tag; }, recursive);
}

QVector<GumboElement*> collectElements(GumboElement *elem, GumboTag tag, bool recursive)
{
    return collectElements(elem, [=](GumboElement *e) { return e->tag == tag; }, recursive);
}

QByteArray attributeValue(GumboElement *elem, const QByteArray &name)
{
    for (uint i =0; i < elem->attributes.length; ++i)
    {
        GumboAttribute *attr = (GumboAttribute*) elem->attributes.data[i];
        if (name == attr->name)
            return QByteArray(attr->value);
    }
    return nullptr;
}

static const char *getText_helper(GumboElement *elem);

static const char *getText_helper(GumboNode *node)
{
    if (node->type == GUMBO_NODE_TEXT)
        return node->v.text.text;
    else if (node->type == GUMBO_NODE_ELEMENT)
        return getText_helper(&node->v.element);
    else
        return nullptr;
}

static const char *getText_helper(GumboElement *elem)
{
    for (uint i = 0; i < elem->children.length; ++i) {
        const char *txt = getText_helper((GumboNode*) elem->children.data[i]);
        if (txt)
            return txt;
    }
    return nullptr;
}

static void collectTexts(GumboNode *node, QStringList &into)
{
    if (node->type == GUMBO_NODE_TEXT) {
        into << QString(node->v.text.text);
    }
    else if (node->type == GUMBO_NODE_ELEMENT) {
        for (uint i = 0; i < node->v.element.children.length; ++i) {
            collectTexts((GumboNode*) node->v.element.children.data[i], into);
        }
    }
}

QStringList collectTexts(GumboElement *elem)
{
    QStringList ret;
    for (uint i = 0; i < elem->children.length; ++i) {
        collectTexts((GumboNode*) elem->children.data[i], ret);
    }
    return ret;
}

QString getFirstText(GumboElement *elem)
{
    const char *txt = getText_helper(elem);
    return txt ? QString(txt) : QString();
}

} // namespace ScrapeUtil
