/******************************************************************************
/ win32-import.h
/
/ Copyright (c) 2020 Christian Fillion
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

#ifndef _WIN32
#  error This file should not be included on non-Windows systems.
#endif

namespace win32 {
  template<typename Proc, typename = std::enable_if_t<std::is_function_v<Proc>>>
  class import {
  public:
    import(const char *dll, const char *func)
    {
      if (m_lib = LoadLibrary(dll))
        m_proc = reinterpret_cast<Proc *>(GetProcAddress(m_lib, func));
      else
        m_proc = nullptr;
    }

    ~import()
    {
      if (m_lib)
        FreeLibrary(m_lib);
    }

    operator bool() const { return m_proc != nullptr; }

    template<typename... Args>
    auto operator()(Args&&... args) const { return m_proc(std::forward<Args>(args)...); }

  private:
    HINSTANCE m_lib;
    Proc *m_proc;
  };
}
