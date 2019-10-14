#ifndef __RPR_STATECHUNK_H
#define __RPR_STATECHUNK_H

class RprStateChunk 
{
public:
    RprStateChunk(const char *stateChunk);
    
    const char *get();
    
    ~RprStateChunk();
    
private:
    const char* mChunk;
};


#endif /* __RPR_STATECHUNK_H */
