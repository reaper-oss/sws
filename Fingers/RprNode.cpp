#include "stdafx.h"
#include <memory>

#include "RprNode.h"

void RprPropertyNode::toReaper(std::ostringstream &oss, int indent)
{
    std::string strIndent(indent, ' ');
    oss << strIndent.c_str() << getValue().c_str();
    oss << std::endl;
}

const std::string& RprNode::getValue() const
{
    return mValue;
}

void RprNode::setValue(const std::string& value)
{
    mValue = value;
}

RprNode *RprNode::getParent()
{
    return mParent;
}

void RprNode::setParent(RprNode *parent)
{
    mParent = parent;
}

RprParentNode::~RprParentNode()
{
    for(std::vector<RprNode *>::iterator i = mChildren.begin();
        i != mChildren.end();
        i++) {
            delete *i;
    }
}

RprParentNode::RprParentNode(const char *value)
{
    setValue(value);
}

RprNode *RprParentNode::getChild(int index) const
{
    return mChildren.at(index);
}

RprNode *RprParentNode::findChildByToken(const std::string &token) const
{
    for(RprNode *child : mChildren) {
        const std::string &value = child->getValue();
        if(value.rfind(token, 0) == 0 && value[token.size()] == '\x20')
            return child;
    }

    return nullptr;
}

void RprParentNode::addChild(RprNode *node)
{
    node->setParent(this);
    mChildren.push_back(node);
}

void RprParentNode::addChild(RprNode *node, int index)
{
    node->setParent(this);
    mChildren.insert(mChildren.begin() + index, node);
}

int RprParentNode::childCount() const
{
    return (int)mChildren.size();
}

void RprParentNode::removeChild(int index)
{
    RprNode *child = mChildren.at(index);
    mChildren.erase(mChildren.begin() + index);
    delete child;
}

static std::string getTrimmedLine(std::istringstream &iss)
{
    while(iss.peek() == '\x20') iss.get();

    std::string line;
    std::getline(iss, line);
    return line;
}

void RprParentNode::toReaper(std::ostringstream &oss, int indent)
{
    std::string strIndent(indent, ' ');
    oss << strIndent.c_str() << "<";
    oss << getValue().c_str() << std::endl;
    for(std::vector<RprNode *>::iterator i = mChildren.begin();
        i != mChildren.end();
        i++) {
            (*i)->toReaper(oss, 0);
    }
    oss << strIndent.c_str() << ">" << std::endl;
}

std::string RprNode::toReaper()
{
    std::ostringstream oss;
    toReaper(oss, 0);
    return oss.str();
}

static RprNode *addNewChildNode(RprNode *node, const std::string &value)
{
    RprNode *newNode = new RprParentNode(value.c_str());
    node->addChild(newNode);
    return newNode;
}

static void addNewPropertyNode(RprNode *node, const std::string &property)
{
    node->addChild(new RprPropertyNode(property));
}

RprPropertyNode::RprPropertyNode(const std::string &value)
{
    setValue(value);
}

RprNode *RprParentNode::createItemStateTree(const char *itemState)
{
    if(itemState == NULL)
        return NULL;

    /* check if it is an item node */
    if(strncmp(itemState, "<ITEM", 5))
        return NULL;

    std::istringstream iss(itemState);
    std::string line = getTrimmedLine(iss);
    std::auto_ptr<RprParentNode> parentNode(new RprParentNode(line.substr(1).c_str()));

    RprNode *currentNode = parentNode.get();

    while(!iss.eof()) {

        line = getTrimmedLine(iss);
        if(line.empty())
            continue;

        if(line[0] == '<')
            currentNode = addNewChildNode(currentNode, line.substr(1));
        else if(line[0] == '>')
            currentNode = currentNode->getParent();
        else
            addNewPropertyNode(currentNode, line);
    }

    return parentNode.release();
}
