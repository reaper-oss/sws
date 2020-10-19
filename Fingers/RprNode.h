#ifndef RPRNODE_HXX
#define RPRNODE_HXX

class RprNode {
public:
    virtual ~RprNode() {}

    RprNode *getParent();
    void setParent(RprNode *parent);

    std::string toReaper();
    virtual void toReaper(std::ostringstream &oss, int indent) = 0;

    virtual int childCount() const = 0;
    virtual RprNode *getChild(int index) const = 0;
    virtual RprNode *findChildByToken(const std::string &) const = 0;
    virtual void addChild(RprNode *node) = 0;
    virtual void addChild(RprNode *node, int index) {}
    virtual void removeChild(int index) = 0;

    void setValue(const std::string &value);
    const std::string &getValue() const;

private:
    std::string mValue;
    RprNode *mParent;
};

class RprPropertyNode : public RprNode {
public:
    RprPropertyNode(const std::string &value);
    ~RprPropertyNode() {}

    int childCount() const override { return 0; }
    RprNode *getChild(int index) const override { return nullptr; }
    RprNode *findChildByToken(const std::string &) const override { return nullptr; }
    void addChild(RprNode *node) override {}
    void removeChild(int index) override {}

private:
    void toReaper(std::ostringstream &oss, int indent) override;
};

class RprParentNode : public RprNode {
public:
    static RprNode *createItemStateTree(const char *itemState);

    RprParentNode(const char *value);
    RprParentNode(const RprNode&) = delete;
    ~RprParentNode();

    int childCount() const override;
    RprNode *getChild(int index) const override;
    RprNode *findChildByToken(const std::string &) const override;
    void addChild(RprNode *node) override;
    void addChild(RprNode *node, int index) override;
    void removeChild(int index) override;

private:
    RprParentNode& operator=(const RprNode&);
    void toReaper(std::ostringstream &oss, int indent) override;

    std::vector<RprNode *> mChildren;
};

#endif /* RPRNODE_HXX */
