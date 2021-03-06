#pragma once

#pragma warning( disable : 4250 ) // inherit via dominance

#define NOMINMAX

#pragma region
//C RunTime Header Files
#include <wchar.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdint>
#pragma endregion stl

#include <rttr/registration>

#pragma region
#include <EtMath/MathInc.h>
#pragma endregion math

#pragma region
template<class T>
inline void SafeDelete(T &pObjectToDelete)
{
	if (pObjectToDelete != nullptr)
	{
		delete pObjectToDelete;
		pObjectToDelete = nullptr;
	}
}
#pragma endregion Templates

//platform independent utility to suppress unused parameter warnings from https://stackoverflow.com/questions/4851075
// use expression as sub-expression,
// then make type of full expression int, discard result
#define UNUSED(x) (void)(x)

#pragma warning(disable : 4201) //nameless struct union - used in math library

#pragma region
#include <EtCore/Util/AtomicTypes.h>
#include <EtCore/UpdateCycle/Context.h>
#include <EtCore/UpdateCycle/Time.h>
#include <EtCore/Util/Logger.h>
#include <EtCore/UpdateCycle/PerformanceInfo.h>
#include <EtCore/Util/StringUtil.h>
#include <EtCore/Hashing/HashString.h>
#include <EtCore/Reflection/ReflectionUtil.h>
//Working singleton Set
#define TIME et::core::ContextManager::GetInstance()->GetActiveContext()->time
#define LOG(fmt, ...) et::core::Logger::Log(fmt, __VA_ARGS__)
#define PERFORMANCE et::core::PerformanceInfo::GetInstance()

// assertions
#ifdef ET_SHIPPING
#	define ET_ASSERT(condition, ...)
#else
#ifdef ET_PLATFORM_WIN
#	define ET_ASSERT(condition, ...) et::core::Logger::ProcessAssert(condition, __FUNCSIG__, FS(__VA_ARGS__))
#else
#	define ET_ASSERT(condition, ...) et::core::Logger::ProcessAssert(condition, __PRETTY_FUNCTION__, FS(__VA_ARGS__))
#endif // ET_PLATFORM_WIN
#endif // ET_SHIPPING

// for performance critical code
#define ET_PARANOID_ASSERTS_ENABLED 0

#if ET_PARANOID_ASSERTS_ENABLED
#	define ET_ASSERT_PARANOID(condition, ...) ET_ASSERT(condition, __VA_ARGS__)
#else
#	define ET_ASSERT_PARANOID(condition, ...)
#endif

#pragma endregion Macros