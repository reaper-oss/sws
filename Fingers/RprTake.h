#ifndef __RPR_TAKE_HXX
#define __RPR_TAKE_HXX

class RprItem;
class MediaItem_Take;
class PCM_source;

class RprTake {
public:
    RprTake(MediaItem_Take *take);

    double getPlayRate() const;
    void setPlayRate(double playRate);

    double getStartOffset() const;
    void setStartOffset(double offset);

    GUID *getGUID() const;

    RprItem getParent() const;

    PCM_source *getSource();

    bool isFile();
    bool isMIDI();
    
    void setName(const char *name);
    const char *getName();

    static RprTake createFromMidiEditor();

    /* Conversion */
    MediaItem_Take *toReaper() const;

private:
    MediaItem_Take *mTake;
};

#endif /* __RPR_TAKE_HXX */
