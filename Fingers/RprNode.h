#ifndef RPRNODE_HXX
#define RPRNODE_HXX

class RprNode {
public:
    const std::string &getValue();

    RprNode *getParent();
    void setParent(RprNode *parent);

    std::string toReaper();
    virtual void toReaper(std::ostringstream &oss, int indent) = 0;

    virtual int childCount() = 0;
    virtual RprNode *getChild(int index) = 0;
    virtual void addChild(RprNode *node) = 0;
    virtual void addChild(RprNode *node, int index) {}
    virtual void removeChild(int index) = 0;
    virtual ~RprNode() {}

    void setValue(const std::string &value);

private:
    std::string mValue;
    RprNode *mParent;
};

class RprPropertyNode : public RprNode {
public:
    RprPropertyNode(const std::string &value);
    int childCount();
    RprNode *getChild(int index);
    void addChild(RprNode *node);
    void removeChild(int index);
    ~RprPropertyNode() {}
private:
    void toReaper(std::ostringstream &oss, int indent);
};

class RprParentNode : public RprNode {
public:
    static RprNode *createItemStateTree(const char *itemState);

    RprParentNode(const char *value);
    int childCount();
    RprNode *getChild(int index);
    void addChild(RprNode *node);
    void addChild(RprNode *node, int index);
    void removeChild(int index);
    ~RprParentNode();
private:
    RprParentNode();
    RprParentNode(const RprNode&);
    RprParentNode& operator=(const RprNode&);

    void toReaper(std::ostringstream &oss, int indent);

    std::vector<RprNode *> mChildren;
};

#endif /* RPRNODE_HXX */
