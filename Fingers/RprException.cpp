#include "stdafx.h"
#include "RprException.h"

RprLibException::RprLibException(const std::string &message, bool notifyUser)
    : mMessage(message), mNotify(notifyUser)
{
}

bool RprLibException::notify() const
{
    return mNotify;
}

const char *RprLibException::what() const throw()
{
    return mMessage.c_str();
}
