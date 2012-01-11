#ifndef _ENVELOPE_H_
#define _ENVELOPE_H_

class CEnvelope;

class CEnvelopePoint
{
public:
    enum EnvShape { Linear, Square, Slow, FastStart, FastEnd, Bezier };

    CEnvelopePoint(CEnvelope *parent = NULL) : 
    dTime(0.0), dParameterValue(0.0), nEnvShape(Linear),
        nSelUnknown(0), nSelected(0), nBezUnknown(0),
        dBezierTension(0.0), pParent(parent)
    {}

    void SetTime(double time) { dTime = time; }
    static bool ComparePointPos(const CEnvelopePoint &left, const CEnvelopePoint &right) { return left.dTime < right.dTime; }
    double GetTime() { return dTime; }

    void SetValue(double value);
    double GetValue() { return dParameterValue; }

    void SetShape(EnvShape shape) { nEnvShape = shape; }
    EnvShape GetShape() { return nEnvShape; }

    bool IsSelected() { return nSelected == 1;}

    double GetBezierTension() { return dBezierTension; }

    void SetParent(CEnvelope *parent) { pParent = parent;}

    void FromString(char *stateData);
    std::string ToString();

private:
    double dTime;
    double dParameterValue;
    EnvShape nEnvShape;
    int nSelUnknown;
    int nSelected;
    int nBezUnknown;
    double dBezierTension;
    CEnvelope *pParent;
};

class CEnvelopePointOp;
class CEnvelope
{
public:

    CEnvelope(TrackEnvelope* env);
    void Write();
    template <typename T>
    void ApplyToPoints(T &op)
    { std::for_each(vPoints.begin(), vPoints.end(), op); }
    std::vector<CEnvelopePoint>& GetPoints()
    { return vPoints; }

    void Add(CEnvelopePoint &point);

    double GetMax() { return dParamMax; }
    double GetMin() { return dParamMin; }


    ~CEnvelope();
private:

    TrackEnvelope* m_env;
    std::string m_szTitle;

    int nParamIndex; // 0-based parameter index, VST top-to-bottom
    double dParamMin; 
    double dParamMax; 
    double dNeutralVal; // neutral or zero value
    /* ACT */
    int nActive; /* 1 or 0 */
    /* VIS */
    int nVisible; /* 1 or 0 */
    int nAutomationInLane; /* 1 or 0 */
    double dVISUnknown;
    /* LANEHEIGHT */
    int nLaneHeight;
    int nLHUnknown;
    /* ARM */
    int nArm;  /* 1 or 0 */
    /* DEFSHAPE */
    int nDefaultShape; /* 0 -> 5 */
    std::vector<CEnvelopePoint> vPoints;
};

typedef std::vector<CEnvelopePoint> EnvPoints;
typedef std::vector<CEnvelopePoint>::iterator EnvPointsIter;

class CEnvelopePointOp {
public:
    void operator() (CEnvelopePoint &p)
    {Do(p);}
private:
    virtual void Do(CEnvelopePoint &p) = 0;
};

class AddToPointPosition : public CEnvelopePointOp
{
public:
    AddToPointPosition(double amt)
    { m_dAmt = amt;} 

private:
    void Do(CEnvelopePoint &p)
    {
        if (p.IsSelected())
            p.SetTime( p.GetTime() + m_dAmt);
    }

    double m_dAmt;
};

class AddToValue : public CEnvelopePointOp
{
public:
    AddToValue(double amt)
    { m_db = amt; m_dm = 0.0; } 

    AddToValue(double m, double b)
    { m_dm = m; m_db = b; } 

private:
    void Do(CEnvelopePoint &p)
    {
        if (p.IsSelected())
            p.SetValue( p.GetValue() + p.GetTime() * m_dm + m_db); 
    }

    double m_dm;
    double m_db;
};

class ScaleValueAroundDifference : public CEnvelopePointOp
{
public:
    ScaleValueAroundDifference(double amt, double diffArg)
    { m_dk = amt; m_dda = diffArg; }

private:
    virtual void Do(CEnvelopePoint &p)
    {
        if (p.IsSelected())
            p.SetValue( p.GetValue() + m_dk * (p.GetValue() - m_dda));
    }

    double m_dk;
    double m_dda;
};

#endif /* _ENVELOPE_H_ */

