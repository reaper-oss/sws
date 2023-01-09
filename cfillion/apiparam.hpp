/******************************************************************************
/ apiparam.hpp
/
/ Copyright (c) 2022 Christian Fillion
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

#define APIPARAM_C(type, member)                                    \
  APIParam(const char *name, type(T::*g)(), void(T::*s)(type))      \
    : m_name { name } { m_getter.member = g; m_setter.member = s; }
#define APIPARAM_GET(key, member)          \
  case key:                                \
    if(!m_getter.member) return false;     \
    *valueOut = (obj->*m_getter.member)(); \
    return true;
#define APIPARAM_SET(key, type, member)                   \
  case key:                                               \
    if(!m_setter.member) return false;                    \
    (obj->*m_setter.member)(static_cast<type>(newValue)); \
    return true;

template<typename T>
class APIParam {
public:
  APIPARAM_C(bool,   b)
  APIPARAM_C(double, d)
  APIPARAM_C(int,    i)

  bool match(const char *name) const { return !strcmp(m_name, name); }

  bool get(T *obj, double *valueOut) const
  {
    switch(m_name[0]) {
      APIPARAM_GET('B', b)
      APIPARAM_GET('D', d)
      APIPARAM_GET('I', i)
    default:
      return false;
    }
  }

  bool set(T *obj, double newValue) const
  {
    switch(m_name[0]) {
      APIPARAM_SET('B', bool,   b)
      APIPARAM_SET('D', double, d)
      APIPARAM_SET('I', int,    i)
    default:
      return false;
    }
  }

private:
  const char *m_name;
  union {
    bool(T::*b)();
    double(T::*d)();
    int(T::*i)();
  } m_getter;
  union {
    void(T::*b)(bool);
    void(T::*d)(double);
    void(T::*i)(int);
  } m_setter;
};

#undef APIPARAM_C
#undef APIPARAM_GET
#undef APIPARAM_SET
