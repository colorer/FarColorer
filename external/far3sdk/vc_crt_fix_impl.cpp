// validator: no-self-include
/*
vc_crt_fix_impl.cpp

Workaround for Visual C++ CRT incompatibility with old Windows versions
*/
/*
Copyright © 2011 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <memory>
#include <utility>

#include <windows.h>
#include <winnls.h>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif


template<typename T>
static T GetFunctionPointer(const wchar_t* ModuleName, const char* FunctionName, T Replacement)
{
	auto Module = GetModuleHandleW(ModuleName);
	if (!Module)
		Module = LoadLibraryW(ModuleName);
	const auto Address = Module? GetProcAddress(Module, FunctionName) : nullptr;
	return Address? reinterpret_cast<T>(reinterpret_cast<void*>(Address)) : Replacement;
}

#define WRAPPER(name) Wrapper_ ## name
#define CREATE_AND_RETURN(ModuleName, ...) \
	static const auto FunctionPointer = GetFunctionPointer(ModuleName, __func__ + sizeof("Wrapper_") - 1, &implementation::impl); \
	return FunctionPointer(__VA_ARGS__)

namespace modules
{
	static const wchar_t kernel32[] = L"kernel32";
	static const wchar_t bcrypt[] = L"bcrypt";
}

static void* XorPointer(void* Ptr)
{
	static const auto Cookie = []
	{
		static void* Ptr;
		auto Result = reinterpret_cast<uintptr_t>(&Ptr);

		FILETIME CreationTime, NotUsed;
		if (GetThreadTimes(GetCurrentThread(), &CreationTime, &NotUsed, &NotUsed, &NotUsed))
		{
			Result ^= CreationTime.dwLowDateTime;
			Result ^= CreationTime.dwHighDateTime;
		}
		return Result;
	}();
	return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Ptr) ^ Cookie);
}

namespace fallback
{
	// Adapted from YY-Thunks (MIT, Chuyu-Team): bcrypt.hpp / api-ms-win-core-synch.hpp
	constexpr LONG StatusSuccess = 0;
	constexpr LONG StatusUnsuccessful = static_cast<LONG>(0xC0000001L);
	constexpr LONG StatusNotImplemented = static_cast<LONG>(0xC0000002L);
	constexpr LONG StatusInvalidParameter = static_cast<LONG>(0xC000000DL);
	constexpr ULONG BcryptUseSystemPreferredRng = 0x00000002ul;

	static CRITICAL_SECTION* srw_cs(PSRWLOCK SRWLock)
	{
		auto* cs = static_cast<CRITICAL_SECTION*>(SRWLock->Ptr);
		if (cs)
			return cs;

		cs = static_cast<CRITICAL_SECTION*>(HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CRITICAL_SECTION)));
		if (!cs)
			return nullptr;

		InitializeCriticalSection(cs);
		const auto previous = InterlockedCompareExchangePointer(&SRWLock->Ptr, cs, nullptr);
		if (previous)
		{
			DeleteCriticalSection(cs);
			HeapFree(GetProcessHeap(), 0, cs);
			return static_cast<CRITICAL_SECTION*>(previous);
		}
		return cs;
	}

	static void acquire_srw(PSRWLOCK SRWLock)
	{
		if (const auto cs = srw_cs(SRWLock))
			EnterCriticalSection(cs);
	}

	static void release_srw(PSRWLOCK SRWLock)
	{
		if (auto* cs = static_cast<CRITICAL_SECTION*>(SRWLock->Ptr))
			LeaveCriticalSection(cs);
	}

	static BOOL init_once_execute_once(PINIT_ONCE InitOnce, PINIT_ONCE_FN InitFn, PVOID Parameter, LPVOID *Context)
	{
		auto* const slot = &InitOnce->Ptr;

		for (;;)
		{
			const auto current = *slot;
			const auto bits = reinterpret_cast<ULONG_PTR>(current) & 3;

			if (bits == 2 || bits == 3)
			{
				if (Context)
					*Context = reinterpret_cast<PVOID>(reinterpret_cast<ULONG_PTR>(current) & ~ULONG_PTR(3));
				return TRUE;
			}

			if (bits == 0)
			{
				if (InterlockedCompareExchangePointer(slot, reinterpret_cast<PVOID>(ULONG_PTR(1)), current) == current)
					break;
				continue;
			}

			SwitchToThread();
		}

		PVOID context = Context ? *Context : nullptr;
		if (InitFn && InitFn(InitOnce, Parameter, &context))
		{
			const auto completed = reinterpret_cast<PVOID>((reinterpret_cast<ULONG_PTR>(context) & ~ULONG_PTR(3)) | 3);
			InterlockedExchangePointer(slot, completed);
			if (Context)
				*Context = context;
			return TRUE;
		}

		InterlockedExchangePointer(slot, nullptr);
		return FALSE;
	}

	static LONG bcrypt_gen_random(PVOID hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer, ULONG dwFlags)
	{
		if (!pbBuffer)
			return StatusInvalidParameter;
		if (cbBuffer == 0)
			return StatusSuccess;
		if ((dwFlags & BcryptUseSystemPreferredRng) && hAlgorithm)
			return StatusInvalidParameter;

		using RtlGenRandomFn = BOOLEAN (WINAPI *)(PVOID, ULONG);
		static const auto RtlGenRandom = reinterpret_cast<RtlGenRandomFn>(reinterpret_cast<void*>(
			GetProcAddress(GetModuleHandleW(L"advapi32"), "SystemFunction036")));
		if (!RtlGenRandom)
			return StatusNotImplemented;

		return RtlGenRandom(pbBuffer, cbBuffer) ? StatusSuccess : StatusUnsuccessful;
	}
}

// EncodePointer (VC2010)
extern "C" PVOID WINAPI WRAPPER(EncodePointer)(PVOID Ptr)
{
	struct implementation
	{
		static PVOID WINAPI impl(PVOID Ptr)
		{
			return XorPointer(Ptr);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Ptr);
}

// DecodePointer(VC2010)
extern "C" PVOID WINAPI WRAPPER(DecodePointer)(PVOID Ptr)
{
	struct implementation
	{
		static PVOID WINAPI impl(PVOID Ptr)
		{
			return XorPointer(Ptr);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Ptr);
}

// GetModuleHandleExW (VC2012)
extern "C" BOOL WINAPI WRAPPER(GetModuleHandleExW)(DWORD Flags, LPCWSTR ModuleName, HMODULE *Module)
{
	struct implementation
	{
		static BOOL WINAPI impl(DWORD Flags, LPCWSTR ModuleName, HMODULE *Module)
		{
			if (Flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
			{
				MEMORY_BASIC_INFORMATION mbi;
				if (!VirtualQuery(ModuleName, &mbi, sizeof(mbi)))
					return FALSE;

				const auto ModuleValue = static_cast<HMODULE>(mbi.AllocationBase);

				if (!(Flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
				{
					wchar_t Buffer[MAX_PATH];
					if (!GetModuleFileNameW(ModuleValue, Buffer, ARRAYSIZE(Buffer)))
						return FALSE;
					// It's the same so not really necessary, but analysers report handle leak otherwise.
					*Module = LoadLibraryW(Buffer);
				}
				else
				{
					*Module = ModuleValue;
				}
				return TRUE;
			}

			// GET_MODULE_HANDLE_EX_FLAG_PIN not implemented

			if (const auto ModuleValue = (Flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT? GetModuleHandleW : LoadLibraryW)(ModuleName))
			{
				*Module = ModuleValue;
				return TRUE;
			}
			return FALSE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Flags, ModuleName, Module);
}

// VC2015
extern "C" void WINAPI WRAPPER(InitializeSListHead)(PSLIST_HEADER ListHead)
{
	struct implementation
	{
		static void WINAPI impl(PSLIST_HEADER ListHead)
		{
			*ListHead = {};
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ListHead);
}

#ifndef _WIN64
static bool atomic_assign(PSLIST_HEADER To, SLIST_HEADER const& New, SLIST_HEADER const& Old)
{
	return InterlockedCompareExchange64(
		static_cast<LONG64*>(static_cast<void*>(&To->Alignment)),
		New.Alignment,
		Old.Alignment
	) == static_cast<LONG64>(Old.Alignment);
}
#endif

extern "C" PSLIST_ENTRY WINAPI WRAPPER(InterlockedFlushSList)(PSLIST_HEADER ListHead)
{
	struct implementation
	{
		static PSLIST_ENTRY WINAPI impl(PSLIST_HEADER ListHead)
		{
#ifdef _WIN64
			// The oldest x64 OS (XP) already has SList, so this shall never be called.
			DebugBreak();
			return {};
#else
			if (!ListHead->Next.Next)
				return {};

			SLIST_HEADER OldHeader, NewHeader{};

			do
			{
				OldHeader = *ListHead;
				NewHeader.CpuId = OldHeader.CpuId;
			}
			while (!atomic_assign(ListHead, NewHeader, OldHeader));

			return OldHeader.Next.Next;
#endif
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ListHead);
}

extern "C" PSLIST_ENTRY WINAPI WRAPPER(InterlockedPushEntrySList)(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry)
{
	struct implementation
	{
		static PSLIST_ENTRY WINAPI impl(PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry)
		{
#ifdef _WIN64
			// The oldest x64 OS (XP) already has SList, so this shall never be called.
			DebugBreak();
			return {};
#else
			SLIST_HEADER OldHeader, NewHeader;
			NewHeader.Next.Next = ListEntry;

			do
			{
				OldHeader = *ListHead;
				ListEntry->Next = OldHeader.Next.Next;
				NewHeader.Depth = OldHeader.Depth + 1;
				NewHeader.CpuId = OldHeader.CpuId;
			}
			while (!atomic_assign(ListHead, NewHeader, OldHeader));

			return OldHeader.Next.Next;
#endif
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ListHead, ListEntry);
}

// VC2017
extern "C" BOOL WINAPI WRAPPER(GetNumaHighestNodeNumber)(PULONG HighestNodeNumber)
{
	struct implementation
	{
		static BOOL WINAPI impl(PULONG HighestNodeNumber)
		{
			*HighestNodeNumber = 0;
			return TRUE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, HighestNodeNumber);
}

// VC2017
extern "C" BOOL WINAPI WRAPPER(GetLogicalProcessorInformation)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnLength)
{
	struct implementation
	{
		static BOOL WINAPI impl(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Buffer, ReturnLength);
}

// VC2019
extern "C" BOOL WINAPI WRAPPER(SetThreadStackGuarantee)(PULONG StackSizeInBytes)
{
	struct implementation
	{
		static BOOL WINAPI impl(PULONG)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, StackSizeInBytes);
}

// VC2019
extern "C" BOOL WINAPI WRAPPER(InitializeCriticalSectionEx)(LPCRITICAL_SECTION CriticalSection, DWORD SpinCount, DWORD Flags)
{
	struct implementation
	{
		static int WINAPI impl(LPCRITICAL_SECTION CriticalSection, DWORD SpinCount, DWORD Flags)
		{
			InitializeCriticalSection(CriticalSection);
			CriticalSection->SpinCount = Flags | SpinCount;
			return TRUE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, CriticalSection, SpinCount, Flags);
}

static LCID locale_name_to_lcid(const wchar_t* LocaleName)
{
	if (!LocaleName)
		return LOCALE_USER_DEFAULT;

	if (!*LocaleName)
		return LOCALE_INVARIANT;

	if (!lstrcmp(LocaleName, LOCALE_NAME_SYSTEM_DEFAULT))
		return LOCALE_SYSTEM_DEFAULT;

	return LOCALE_USER_DEFAULT;
}

// VC2019
extern "C" int WINAPI WRAPPER(CompareStringEx)(LPCWSTR LocaleName, DWORD CmpFlags, LPCWCH String1, int Count1, LPCWCH String2, int Count2, LPNLSVERSIONINFO VersionInformation, LPVOID Reserved, LPARAM Param)
{
	struct implementation
	{
		static int WINAPI impl(LPCWSTR LocaleName, DWORD CmpFlags, LPCWCH String1, int Count1, LPCWCH String2, int Count2, LPNLSVERSIONINFO, LPVOID, LPARAM)
		{
			return CompareStringW(locale_name_to_lcid(LocaleName), CmpFlags, String1, Count1, String2, Count2);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, LocaleName, CmpFlags, String1, Count1, String2, Count2, VersionInformation, Reserved, Param);
}

// VC2019
extern "C" int WINAPI WRAPPER(LCMapStringEx)(LPCWSTR LocaleName, DWORD MapFlags, LPCWSTR SrcStr, int SrcCount, LPWSTR DestStr, int DestCount, LPNLSVERSIONINFO VersionInformation, LPVOID Reserved, LPARAM SortHandle)
{
	struct implementation
	{
		static int WINAPI impl(LPCWSTR LocaleName, DWORD MapFlags, LPCWSTR SrcStr, int SrcCount, LPWSTR DestStr, int DestCount, LPNLSVERSIONINFO, LPVOID, LPARAM)
		{
			return LCMapStringW(locale_name_to_lcid(LocaleName), MapFlags, SrcStr, SrcCount, DestStr, DestCount);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, LocaleName, MapFlags, SrcStr, SrcCount, DestStr, DestCount, VersionInformation, Reserved, SortHandle);
}

// VC2022
extern "C" BOOL WINAPI WRAPPER(SleepConditionVariableSRW)(PCONDITION_VARIABLE ConditionVariable, PSRWLOCK SRWLock, DWORD Milliseconds, ULONG Flags)
{
	struct implementation
	{
		static BOOL WINAPI impl(PCONDITION_VARIABLE, PSRWLOCK, DWORD, ULONG)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
			return FALSE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ConditionVariable, SRWLock, Milliseconds, Flags);
}

// VC2022
extern "C" void WINAPI WRAPPER(WakeAllConditionVariable)(PCONDITION_VARIABLE ConditionVariable)
{
	struct implementation
	{
		static void WINAPI impl(PCONDITION_VARIABLE)
		{
			SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, ConditionVariable);
}

// VC2022
extern "C" void WINAPI WRAPPER(AcquireSRWLockExclusive)(PSRWLOCK SRWLock)
{
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			fallback::acquire_srw(SRWLock);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// VC2022
extern "C" void WINAPI WRAPPER(ReleaseSRWLockExclusive)(PSRWLOCK SRWLock)
{
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			fallback::release_srw(SRWLock);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// VC2022
extern "C" BOOLEAN WINAPI WRAPPER(TryAcquireSRWLockExclusive)(PSRWLOCK SRWLock)
{
	struct implementation
	{
		static BOOLEAN WINAPI impl(PSRWLOCK SRWLock)
		{
			if (const auto cs = fallback::srw_cs(SRWLock))
				return TryEnterCriticalSection(cs) ? TRUE : FALSE;
			return FALSE;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// VC2022
extern "C" void WINAPI WRAPPER(AcquireSRWLockShared)(PSRWLOCK SRWLock)
{
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			fallback::acquire_srw(SRWLock);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// VC2022
extern "C" void WINAPI WRAPPER(ReleaseSRWLockShared)(PSRWLOCK SRWLock)
{
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			fallback::release_srw(SRWLock);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

// VC2022
extern "C" BOOL WINAPI WRAPPER(InitOnceExecuteOnce)(PINIT_ONCE InitOnce, PINIT_ONCE_FN InitFn, PVOID Parameter, LPVOID *Context)
{
	struct implementation
	{
		static BOOL WINAPI impl(PINIT_ONCE InitOnce, PINIT_ONCE_FN InitFn, PVOID Parameter, LPVOID *Context)
		{
			return fallback::init_once_execute_once(InitOnce, InitFn, Parameter, Context);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, InitOnce, InitFn, Parameter, Context);
}

// VC2022
extern "C" LONG WINAPI WRAPPER(BCryptGenRandom)(PVOID hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer, ULONG dwFlags)
{
	struct implementation
	{
		static LONG WINAPI impl(PVOID hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer, ULONG dwFlags)
		{
			return fallback::bcrypt_gen_random(hAlgorithm, pbBuffer, cbBuffer, dwFlags);
		}
	};

	CREATE_AND_RETURN(modules::bcrypt, hAlgorithm, pbBuffer, cbBuffer, dwFlags);
}

// libxml2 calls BCryptGenRandom without dllimport. Providing the stdcall
// symbol keeps the linker from pulling bcrypt.lib, which would redefine
// __imp_BCryptGenRandom (LNK2005) and add a hard import of bcrypt.dll.
extern "C" LONG WINAPI BCryptGenRandom(PVOID hAlgorithm, PUCHAR pbBuffer, ULONG cbBuffer, ULONG dwFlags)
{
	return WRAPPER(BCryptGenRandom)(hAlgorithm, pbBuffer, cbBuffer, dwFlags);
}

// VC2019
extern "C" void WINAPI WRAPPER(InitializeSRWLock)(PSRWLOCK SRWLock)
{
	struct implementation
	{
		static void WINAPI impl(PSRWLOCK SRWLock)
		{
			*SRWLock = SRWLOCK_INIT;
		}
	};

	CREATE_AND_RETURN(modules::kernel32, SRWLock);
}

extern "C" DWORD WINAPI WRAPPER(FlsAlloc)(PFLS_CALLBACK_FUNCTION Callback)
{
	struct implementation
	{
		static DWORD WINAPI impl(PFLS_CALLBACK_FUNCTION)
		{
			return TlsAlloc();
		}
	};

	CREATE_AND_RETURN(modules::kernel32, Callback);
}

extern "C" PVOID WINAPI WRAPPER(FlsGetValue)(DWORD FlsIndex)
{
	struct implementation
	{
		static PVOID WINAPI impl(DWORD FlsIndex)
		{
			return TlsGetValue(FlsIndex);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, FlsIndex);
}

extern "C" BOOL WINAPI WRAPPER(FlsSetValue)(DWORD FlsIndex, PVOID FlsData)
{
	struct implementation
	{
		static BOOL WINAPI impl(DWORD FlsIndex, PVOID FlsData)
		{
			return TlsSetValue(FlsIndex, FlsData);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, FlsIndex, FlsData);
}

extern "C" BOOL WINAPI WRAPPER(FlsFree)(DWORD FlsIndex)
{
	struct implementation
	{
		static BOOL WINAPI impl(DWORD FlsIndex)
		{
			return TlsFree(FlsIndex);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, FlsIndex);
}

// added to original file for FarColorer
#include "yy_thunks_impl.h"

extern "C" int WINAPI WRAPPER(GetLocaleInfoEx)(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
	struct implementation
	{
		static int WINAPI impl(LPCWSTR lpLocaleName, LCTYPE LCType, LPWSTR lpLCData, int cchData)
		{
			auto Locale = LocaleNameToLCID(lpLocaleName, 0);

			if (Locale == 0)
			{
				SetLastError(ERROR_INVALID_PARAMETER);
				return 0;
			}
			return GetLocaleInfoW(Locale, LCType, lpLCData, cchData);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, lpLocaleName, LCType, lpLCData, cchData);
}

extern "C" LCID WINAPI WRAPPER(LocaleNameToLCID)(LPCWSTR lpName, DWORD dwFlags)
{
	struct implementation
	{
		static LCID WINAPI impl(LPCWSTR lpName, DWORD dwFlags)
		{
			return local_LocaleNameToLCID(lpName, dwFlags);
		}
	};

	CREATE_AND_RETURN(modules::kernel32, lpName, dwFlags);
}

// end FarColorer

#undef CREATE_AND_RETURN
#undef WRAPPER

// disable VS2015 telemetry
extern "C"
{
	void __vcrt_initialize_telemetry_provider() {}
	void __telemetry_main_invoke_trigger() {}
	void __telemetry_main_return_trigger() {}
	void __vcrt_uninitialize_telemetry_provider() {}
}
