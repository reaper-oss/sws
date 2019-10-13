target_sources(sws
PRIVATE
  padreActions.cpp
  padreEnvelopeProcessor.cpp
  padreMidiItemFilters.cpp
  padreMidiItemProcBase.cpp
  padreUtils.cpp
)

if(WIN32)
  target_sources(sws PRIVATE padreRmeTotalmix.cpp)
endif()
