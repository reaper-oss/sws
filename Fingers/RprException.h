#ifndef __RPR_EXCEPTION_HXX
#define __RPR_EXCEPTION_HXX

class RprLibException : public std::exception {
	public:
		RprLibException(std::string message, bool notifyUser = false);
		const char *what();
		bool notify();
		virtual ~RprLibException() throw();
	private:
		std::string mMessage;
		bool mNotify;
	};

#endif /* __RPR_EXCEPTION_HXX */