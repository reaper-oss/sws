#ifndef __RPR_STATECHUNK_HXX
#define __RPR_STATECHUNK_HXX

class RprStateChunk {
public:
    RprStateChunk(const char *stateChunk);
    const char *get();
    ~RprStateChunk();
private:
    const char *mChunk;
};


#endif /* __RPR_STATECHUNK_HXX */