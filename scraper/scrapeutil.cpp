#include "scrapeutil.hpp"

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <QDebug>

namespace ScrapeUtil
{
    
static GumboNode *node(GumboElement *elem)
{
    const intptr_t ofs = offsetof(GumboNode, v.element);
    return (GumboNode*) ((intptr_t) elem - ofs);
}

GumboElement *parentElement(GumboElement *elem)
{
    for (GumboNode *n = node(elem)->parent; n; n = n->parent) {
        if (n->type == GUMBO_NODE_ELEMENT)
            return &n->v.element;
    }
    return nullptr;
}

GumboElement *getFirstElement(GumboNode *node, const NodeFilter &filter)
{
    if (node->type != GUMBO_NODE_ELEMENT)
        return nullptr;

    return getFirstElement(&node->v.element, filter);
}

GumboElement *getFirstElement(GumboElement *elem, const NodeFilter &filter)
{
    if (filter(elem))
        return elem;

    for (uint i = 0; i < elem->children.length; ++i) {
        GumboElement *found = getFirstElement((GumboNode*) elem->children.data[i], filter);
        if (found)
            return found;
    }

    return nullptr;
}

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


static std::string nonbreaking_inline  = "|a|abbr|acronym|b|bdo|big|cite|code|dfn|em|font|i|img|kbd|nobr|s|small|span|strike|strong|sub|sup|tt|";
static std::string empty_tags          = "|area|base|basefont|bgsound|br|command|col|embed|event-source|frame|hr|image|img|input|keygen|link|menuitem|meta|param|source|spacer|track|wbr|";
static std::string preserve_whitespace = "|pre|textarea|script|style|";
static std::string special_handling    = "|html|body|";
static std::string no_entity_sub       = "|script|style|";
static std::string treat_like_inline   = "|p|";

static std::string prettyprint(GumboNode* node, int lvl, const std::string indent_chars);

static inline void rtrim(std::string &s)
{
    s.erase(s.find_last_not_of(" \n\r\t")+1);
}

static inline void ltrim(std::string &s)
{
    s.erase(0,s.find_first_not_of(" \n\r\t"));
}

static void replace_all(std::string &s, const char * s1, const char * s2)
{
    std::string t1(s1);
    size_t len = t1.length();
    size_t pos = s.find(t1);
    while (pos != std::string::npos) {
        s.replace(pos, len, s2);
        pos = s.find(t1, pos + len);
    }
}

static std::string substitute_xml_entities_into_text(const std::string &text)
{
    std::string result = text;
    // replacing & must come first
    replace_all(result, "&", "&amp;");
    replace_all(result, "<", "&lt;");
    replace_all(result, ">", "&gt;");
    return result;
}

static std::string substitute_xml_entities_into_attributes(char quote, const std::string &text)
{
    std::string result = substitute_xml_entities_into_text(text);
    if (quote == '"') {
        replace_all(result,"\"","&quot;");
    }
    else if (quote == '\'') {
        replace_all(result,"'","&apos;");
    }
 return result;
}

static std::string handle_unknown_tag(GumboStringPiece *text)
{
    std::string tagname = "";
    if (text->data == NULL) {
        return tagname;
    }
    // work with copy GumboStringPiece to prevent asserts
    // if try to read same unknown tag name more than once
    GumboStringPiece gsp = *text;
    gumbo_tag_from_original_text(&gsp);
    tagname = std::string(gsp.data, gsp.length);
    return tagname;
}

static std::string get_tag_name(GumboNode *node)
{
    std::string tagname;
    // work around lack of proper name for document node
    if (node->type == GUMBO_NODE_DOCUMENT) {
        tagname = "document";
    } else {
        tagname = gumbo_normalized_tagname(node->v.element.tag);
    }
    if (tagname.empty()) {
        tagname = handle_unknown_tag(&node->v.element.original_tag);
    }
    return tagname;
}

static std::string build_doctype(GumboNode *node)
{
    std::string results = "";
    if (node->v.document.has_doctype) {
        results.append("<!DOCTYPE ");
        results.append(node->v.document.name);
        std::string pi(node->v.document.public_identifier);
        if ((node->v.document.public_identifier != NULL) && !pi.empty() ) {
                results.append(" PUBLIC \"");
                results.append(node->v.document.public_identifier);
                results.append("\" \"");
                results.append(node->v.document.system_identifier);
                results.append("\"");
        }
        results.append(">\n");
    }
    return results;
}

static std::string build_attributes(GumboAttribute * at, bool no_entities)
{
    std::string atts = "";
    atts.append(" ");
    atts.append(at->name);

    // how do we want to handle attributes with empty values
    // <input type="checkbox" checked />    or <input type="checkbox" checked="" />

    if ( (!std::string(at->value).empty())     ||
             (at->original_value.data[0] == '"') ||
             (at->original_value.data[0] == '\'') ) {

        // determine original quote character used if it exists
        char quote = at->original_value.data[0];
        std::string qs = "";
        if (quote == '\'') qs = std::string("'");
        if (quote == '"') qs = std::string("\"");

        atts.append("=");

        atts.append(qs);

        if (no_entities) {
            atts.append(at->value);
        } else {
            atts.append(substitute_xml_entities_into_attributes(quote, std::string(at->value)));
        }

        atts.append(qs);
    }
    return atts;
}

// prettyprint children of a node
// may be invoked recursively

static std::string prettyprint_contents(GumboNode* node, int lvl, const std::string indent_chars) {

    std::string contents                = "";
    std::string tagname                 = get_tag_name(node);
    std::string key                         = "|" + tagname + "|";
    bool no_entity_substitution = no_entity_sub.find(key) != std::string::npos;
    bool keep_whitespace                = preserve_whitespace.find(key) != std::string::npos;
    bool is_inline                            = nonbreaking_inline.find(key) != std::string::npos;
    bool pp_okay                                = !is_inline && !keep_whitespace;

    GumboVector* children = &node->v.element.children;

    for (unsigned int i = 0; i < children->length; ++i) {
        GumboNode* child = static_cast<GumboNode*> (children->data[i]);

        if (child->type == GUMBO_NODE_TEXT) {

            std::string val;

            if (no_entity_substitution) {
                val = std::string(child->v.text.text);
            } else {
                val = substitute_xml_entities_into_text(std::string(child->v.text.text));
            }

            if (pp_okay) rtrim(val);

            if (pp_okay && (contents.length() == 0)) {
                // add required indentation
                char c = indent_chars.at(0);
                int n    = indent_chars.length();
                contents.append(std::string((lvl-1)*n,c));
            }

            contents.append(val);


        } else if ((child->type == GUMBO_NODE_ELEMENT) || (child->type == GUMBO_NODE_TEMPLATE)) {

            std::string val = prettyprint(child, lvl, indent_chars);

            // remove any indentation if this child is inline and not first child
            std::string childname = get_tag_name(child);
            std::string childkey = "|" + childname + "|";
            if ((nonbreaking_inline.find(childkey) != std::string::npos) && (contents.length() > 0)) {
                ltrim(val);
            }

            contents.append(val);

        } else if (child->type == GUMBO_NODE_WHITESPACE) {

            if (keep_whitespace || is_inline) {
                contents.append(std::string(child->v.text.text));
            }

        } else if (child->type != GUMBO_NODE_COMMENT) {

            // Does this actually exist: (child->type == GUMBO_NODE_CDATA)
            fprintf(stderr, "unknown element of type: %d\n", child->type);

        }

    }

    return contents;
}


// prettyprint a GumboNode back to html/xhtml
// may be invoked recursively

static std::string prettyprint(GumboNode* node, int lvl, const std::string indent_chars) {

    // special case the document node
    if (node->type == GUMBO_NODE_DOCUMENT) {
        std::string results = build_doctype(node);
        results.append(prettyprint_contents(node,lvl+1,indent_chars));
        return results;
    }

    std::string close                            = "";
    std::string closeTag                     = "";
    std::string atts                             = "";
    std::string tagname                        = get_tag_name(node);
    std::string key                                = "|" + tagname + "|";
    bool need_special_handling         =    special_handling.find(key) != std::string::npos;
    bool is_empty_tag                            = empty_tags.find(key) != std::string::npos;
    bool no_entity_substitution        = no_entity_sub.find(key) != std::string::npos;
    bool keep_whitespace                     = preserve_whitespace.find(key) != std::string::npos;
    bool is_inline                                 = nonbreaking_inline.find(key) != std::string::npos;
    bool inline_like                             = treat_like_inline.find(key) != std::string::npos;
    bool pp_okay                                     = !is_inline && !keep_whitespace;
    char c                                                 = indent_chars.at(0);
    int    n                                                 = indent_chars.length();

    // build attr string
    const GumboVector * attribs = &node->v.element.attributes;
    for (int i=0; i< attribs->length; ++i) {
        GumboAttribute* at = static_cast<GumboAttribute*>(attribs->data[i]);
        atts.append(build_attributes(at, no_entity_substitution));
    }

    // determine closing tag type
    if (is_empty_tag) {
            close = "/";
    } else {
            closeTag = "</" + tagname + ">";
    }

    std::string indent_space = std::string((lvl-1)*n,c);

    // prettyprint your contents
    std::string contents = prettyprint_contents(node, lvl+1, indent_chars);

    if (need_special_handling) {
        rtrim(contents);
        contents.append("\n");
    }

    char last_char = ' ';
    if (!contents.empty()) {
        last_char = contents.at(contents.length()-1);
    }

    // build results
    std::string results;
    if (pp_okay) {
        results.append(indent_space);
    }
    results.append("<"+tagname+atts+close+">");
    if (pp_okay && !inline_like) {
        results.append("\n");
    }
    if (inline_like) {
        ltrim(contents);
    }
    results.append(contents);
    if (pp_okay && !contents.empty() && (last_char != '\n') && (!inline_like)) {
        results.append("\n");
    }
    if (pp_okay && !inline_like && !closeTag.empty()) {
        results.append(indent_space);
    }
    results.append(closeTag);
    if (pp_okay && !closeTag.empty()) {
        results.append("\n");
    }

    return results;
}

void prettyPrint(GumboOutput *output)
{
    const std::string pretty = prettyprint(output->root, 1, "    ");
    qDebug().noquote() << pretty.data();
}

} // namespace ScrapeUtil
