import sys

if sys.maxsize > 2**32:
  from sws_python64 import *
else:
  from sws_python32 import *
