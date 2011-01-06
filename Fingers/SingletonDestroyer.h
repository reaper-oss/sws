#ifndef _SINGLETON_DESTROYER_H_
#define _SINGLETON_DESTROYER_H_
template <typename DOOMED>
class SingletonDestroyer
{
public:
	SingletonDestroyer(DOOMED* = 0);
    ~SingletonDestroyer();

	void SetDoomed(DOOMED*);
private:
	SingletonDestroyer(const SingletonDestroyer<DOOMED>&);
	void operator=(const SingletonDestroyer<DOOMED>&);
	DOOMED* _doomed;
};

template <class DOOMED>
SingletonDestroyer<DOOMED>::SingletonDestroyer (DOOMED* d)
{
	_doomed = d;
}

template <class DOOMED>
SingletonDestroyer<DOOMED>::~SingletonDestroyer ()
{
	delete _doomed;
}

template <class DOOMED>
void SingletonDestroyer<DOOMED>::SetDoomed (DOOMED* d)
{
	_doomed = d;
}
#endif /*_SINGLETON_DESTROYER_H_ */