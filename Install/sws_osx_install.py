#JFB: OSX "umbrella" import file for SWS/S&M functions
#     See the "note" at http://docs.python.org/library/platform.html
import sys
if sys.maxsize > 2**32:
   from sws_python64 import *
else:
   from sws_python32 import *
