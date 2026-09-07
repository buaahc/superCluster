#ifndef SCENECORE_EXPORT
#define SCENECORE_EXPORT

#ifndef SCENECORE_API
#if defined(_MSC_VER) || defined (_WIN32) || defined (_WIN64)

	#if !defined(SCENECORE_STATIC)
	#    if defined(SCENECORE_EXPORTS)
	#        define SCENECORE_API __declspec(dllexport)
	#    else
	#        define SCENECORE_API __declspec(dllimport)
	#    endif
	#else
	#    define SCENECORE_API
	#endif // SCENECORE_STATIC
#else
	#define SCENECORE_API
#endif
#endif

#endif //SCENECORE_EXPORT
