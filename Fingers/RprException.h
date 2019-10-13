#ifndef __RPR_EXCEPTION_HXX
#define __RPR_EXCEPTION_HXX

class RprLibException : public std::exception {
public:
	RprLibException(const std::string &message, bool notifyUser = false);
	const char *what() const throw() override;
	bool notify() const;
private:
	std::string mMessage;
	bool mNotify;
};

#endif /* __RPR_EXCEPTION_HXX */
