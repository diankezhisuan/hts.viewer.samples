#pragma once

#if defined(_WIN32) || defined(_WIN64)
#  if defined(HTS_VIEWER_DEMO_IMPORTER_BUILD)
#    define HTS_VIEWER_DEMO_IMPORTER_API __declspec(dllexport)
#  else
#    define HTS_VIEWER_DEMO_IMPORTER_API __declspec(dllimport)
#  endif
#else
#  define HTS_VIEWER_DEMO_IMPORTER_API
#endif
