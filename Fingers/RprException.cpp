#include "stdafx.h"
#include "RprException.h"

RprLibException::RprLibException(std::string message, bool notifyUser)
{
    mMessage = message;
    mNotify = notifyUser;
}

bool RprLibException::notify()
{
    return mNotify;
}

const char *RprLibException::what()
{
    return mMessage.c_str();
}

RprLibException::~RprLibException() throw()
{}
