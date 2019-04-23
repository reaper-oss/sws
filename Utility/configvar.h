/******************************************************************************
/ configvar.h
/
/ Copyright (c) 2019 Christian Fillion
/ https://cfillion.ca
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/

#pragma once

template <typename T>
T *GetConfigVar(const char *name)
{
	int size = 0;
	void *configVar = NULL;

	if (int offset = projectconfig_var_getoffs(name, &size))
		configVar = projectconfig_var_addr(EnumProjects(-1, NULL, 0), offset);
	else
		configVar = get_config_var(name, &size);

	if (size != sizeof(T))
		return NULL;

	return static_cast<T *>(configVar);
}

template <typename T>
T GetConfigVar(const char *name, const T fallback)
{
	if (T *configVar = GetConfigVar<T>(name))
		return *configVar;
	else
		return fallback;
}

template <typename T>
bool SetConfigVar(const char *name, const T value)
{
	if (T *configVar = GetConfigVar<T>(name)) {
		*configVar = value;
		return true;
	}

	return false;
}
