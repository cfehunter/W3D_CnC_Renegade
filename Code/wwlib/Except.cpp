/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwlib/Except.cpp                             $*
 *                                                                                             *
 *                      $Author:: Steve_t                                                     $*
 *                                                                                             *
 *                     $Modtime:: 2/07/02 12:28p                                              $*
 *                                                                                             *
 *                    $Revision:: 14                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *                                                                                             *
 * Exception_Proc -- Windows dialog callback for the exception dialog                          *
 * Exception_Dialog -- Brings up the exception options dialog.                                 *
 * Add_Txt -- Add the given text to the machine state dump buffer.                             *
 * Dump_Exception_Info -- Dump machine state information into a buffer                         *
 * Exception_Handler -- Exception handler filter function                                      *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 //#CFE_TODO: This entire file is windows specific. if we want to go cross platform this will need reworking and abstracting
 // It also has a metric-shit-tonne of unsafe string manipulation that should be replaced.

#ifdef _MSC_VER

#include <array>
#include	"always.h"
#include <windows.h>
#include	"assert.h"
#include "cpudetect.h"
#include	"except.h"
//#include "debug.h"
#include "mpu.h"
//#include "commando\nat.h"
#include "thread.h"
#include "wwdebug.h"
#include "wwmemlog.h"

#include	<conio.h>
#include	<imagehlp.h>
#include <crtdbg.h>
#include	<stdio.h>

#ifdef WWDEBUG
#define DebugString 	WWDebug_Printf
#else
void DebugString(char const *, ...){};
#endif //WWDEBUG

#ifdef _WIN64
// Print pointers as 16 hex digits
#define PRIPTRFMT "%016zX"
#else
// Print pointers as 8 hex digits
#define PRIPTRFMT "%08zX"
#endif

/*
** Enable this define to get the 'demo timed out' message on a crash or assert failure.
*/
//#define DEMO_TIME_OUT

/*
** Buffer to dump machine state information to. We don't want to allocate this at run-time
** in case the exception was caused by a malfunction in the memory system.
*/
static char ExceptionText [65536];

bool SymbolsAvailable = false;
HMODULE DbgHelp = nullptr;

void (*AppCallback)(void) = NULL;
char *(*AppVersionCallback)(void) = NULL;

/*
** Flag to indicate we should exit when an exception occurs.
*/
bool ExitOnException = false;
bool TryingToExit = false;

/*
** Register dump variables. These are used to allow the game to restart from an arbitrary
** position after an exception occurs.
*/
unsigned long ExceptionReturnStack = 0;
unsigned long ExceptionReturnAddress = 0;
unsigned long ExceptionReturnFrame = 0;

/*
** Number of times the exception handler has recursed. Recursions are bad.
*/
int ExceptionRecursions = -1;

/*
** List of threads that the exception handler knows about.
*/
DynamicVectorClass<ThreadInfoType*> ThreadList;

/*
** Definitions to allow run-time linking to the Imagehlp.dll functions.
**
*/
#ifdef _WIN64
typedef BOOL(WINAPI* SymCleanupType) (HANDLE hProcess);
typedef BOOL(WINAPI* SymGetSymFromAddrType) (HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PIMAGEHLP_SYMBOL Symbol);
typedef BOOL(WINAPI* SymInitializeType) (HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess);
typedef BOOL(WINAPI* SymLoadModuleType) (HANDLE hProcess, HANDLE hFile, PCSTR ImageName, PCSTR ModuleName, DWORD64 BaseOfDll, DWORD64 SizeOfDll);
typedef DWORD(WINAPI* SymSetOptionsType) (DWORD SymOptions);
typedef BOOL(WINAPI* SymUnloadModuleType) (HANDLE hProcess, DWORD64 BaseOfDll);
typedef BOOL(WINAPI* StackWalkType) (DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);
typedef PVOID(WINAPI* SymFunctionTableAccessType) (HANDLE hProcess, DWORD64 AddrBase);
typedef DWORD64(WINAPI* SymGetModuleBaseType) (HANDLE hProcess, DWORD64 dwAddr);
#else
typedef BOOL  (WINAPI *SymCleanupType) (HANDLE hProcess);
typedef BOOL  (WINAPI *SymGetSymFromAddrType) (HANDLE hProcess, DWORD Address, LPDWORD Displacement, PIMAGEHLP_SYMBOL Symbol);
typedef BOOL  (WINAPI *SymInitializeType) (HANDLE hProcess, LPSTR UserSearchPath, BOOL fInvadeProcess);
typedef BOOL  (WINAPI *SymLoadModuleType) (HANDLE hProcess, HANDLE hFile, LPSTR ImageName, LPSTR ModuleName, DWORD BaseOfDll, DWORD SizeOfDll);
typedef DWORD (WINAPI *SymSetOptionsType) (DWORD SymOptions);
typedef BOOL  (WINAPI *SymUnloadModuleType) (HANDLE hProcess, DWORD BaseOfDll);
typedef BOOL  (WINAPI *StackWalkType) (DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME StackFrame, LPVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);
typedef LPVOID (WINAPI *SymFunctionTableAccessType) (HANDLE hProcess, DWORD AddrBase);
typedef DWORD (WINAPI *SymGetModuleBaseType) (HANDLE hProcess, DWORD dwAddr);
#endif // #endif


static SymCleanupType               _SymCleanup = NULL;
static SymGetSymFromAddrType        _SymGetSymFromAddr = NULL;
static SymInitializeType            _SymInitialize = NULL;
static SymLoadModuleType            _SymLoadModule = NULL;
static SymSetOptionsType            _SymSetOptions = NULL;
static SymUnloadModuleType          _SymUnloadModule = NULL;
static StackWalkType                _StackWalk = NULL;
static SymFunctionTableAccessType   _SymFunctionTableAccess = NULL;
static SymGetModuleBaseType         _SymGetModuleBase = NULL;

static std::array<FARPROC*, 9> Get_DbgHelp_FunctionPointers()
{
	return std::array<FARPROC*, 9>{
		(FARPROC*)&_SymCleanup,
		(FARPROC*)&_SymGetSymFromAddr,
		(FARPROC*)&_SymInitialize,
		(FARPROC*)&_SymLoadModule,
		(FARPROC*)&_SymSetOptions,
		(FARPROC*)&_SymUnloadModule,
		(FARPROC*)&_StackWalk,
		(FARPROC*)&_SymFunctionTableAccess,
		(FARPROC*)&_SymGetModuleBase,
	};
}

/***********************************************************************************************
 * Init_DbgHelp -- Load the dbghelp dynamic library and fetch symbol function pointers.        *
 * Internally checks the DbgHelp handle so redundant calls are ignored.                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/08/2025 CFE     : Created                                                              *
 *=============================================================================================*/
static void Init_DbgHelp()
{
	// Don't attempt to load if already loaded
	if (DbgHelp)
		return;

	std::array<FARPROC*, 9> ImageHelpFunctions = Get_DbgHelp_FunctionPointers();

	// Symbol names are different between x64 and x86
	static constexpr std::array<const char*, 9> ImagehelpFunctionNames =
	{
	#ifdef _WIN64
		"SymCleanup",
		"SymGetSymFromAddr64",
		"SymInitialize",
		"SymLoadModule64",
		"SymSetOptions",
		"SymUnloadModule64",
		"StackWalk64",
		"SymFunctionTableAccess64",
		"SymGetModuleBase64",
	#else
		"SymCleanup",
		"SymGetSymFromAddr",
		"SymInitialize",
		"SymLoadModule",
		"SymSetOptions",
		"SymUnloadModule",
		"StackWalk",
		"SymFunctionTableAccess",
		"SymGetModuleBase",
	#endif // _WIN64
	};

	// Load the dll. Early out with a logged error if we can't find it
	HMODULE dll_handle = LoadLibraryA("dbghelp.dll");
	if (!dll_handle)
	{
		DebugString("Exception Handler: Unable to load dbghelp.dll\n");
		return;
	}

	static_assert(ImageHelpFunctions.size() == ImagehelpFunctionNames.size());
	for (size_t i = 0; i < ImageHelpFunctions.size(); ++i)
	{
		const FARPROC loaded_func = GetProcAddress(dll_handle, ImagehelpFunctionNames[i]);
		*ImageHelpFunctions[i] = loaded_func;

		// Log an error if this failed. It's not fatal but we'll want to know
		if (!loaded_func)
		{
			DebugString("Exception Handler: Unable to load %s dbghelp.dll\n", ImagehelpFunctionNames[i]);
		}
	}
}

/***********************************************************************************************
 * Load_Symbols -- Load the debug symbols. Will load the dbghelp dynamic library,              *
 * if it has not already been loaded. SymbolsAvailable is checked before loading symbols.      *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   true = symbols available                                                          *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/08/2025 CFE     : Created                                                              *
 *=============================================================================================*/
static bool Load_Symbols()
{
	// Symbols are already loaded
	if (SymbolsAvailable)
		return SymbolsAvailable;

	// Init dbghelp function pointers if they're not already loaded
	Init_DbgHelp();

	// Retrieve the programs symbols if they are available. This can be a .pdb or a .dbg file.
	if (_SymSetOptions)
		_SymSetOptions(SYMOPT_DEFERRED_LOADS);

	if (_SymInitialize && _SymInitialize(GetCurrentProcess(), NULL, FALSE))
	{

		if (_SymSetOptions)
			_SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);

		char module_name[_MAX_PATH];
		GetModuleFileName(NULL, module_name, sizeof(module_name));

		SymbolsAvailable  = _SymLoadModule && _SymLoadModule(GetCurrentProcess(), NULL, module_name, NULL, 0, 0);
	}

	return SymbolsAvailable;
}

/***********************************************************************************************
 * Unload_DbgHelp_AndSymbols -- Cleanup loaded symbols and unload the dbghelp dynamic lib.     *
 * Internally guards against invalid access. Can be called regardless of state.                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/08/2025 CFE     : Created                                                              *
 *=============================================================================================*/
static void Unload_DbgHelp_And_Symbols()
{
	// Cleanup symbols
	if (SymbolsAvailable)
	{
		HANDLE current_proc = GetCurrentProcess();

		if (_SymCleanup)
			_SymCleanup(current_proc);

		if (_SymUnloadModule)
			_SymUnloadModule(current_proc, NULL);

		SymbolsAvailable = false;
	}

	// Null the functions
	auto function_pointers = Get_DbgHelp_FunctionPointers();
	for (auto* func : function_pointers)
	{
		*func = nullptr;
	}

	// Free the debug library
	if (DbgHelp)
	{
		FreeLibrary(DbgHelp);
		DbgHelp = nullptr;
	}
}

/***********************************************************************************************
 * _purecall -- This function overrides the C library Pure Virtual Function Call error         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   0 = no error                                                                      *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   8/22/00 11:42AM ST : Created                                                              *
 *   03/04/2025 CFE     : Switch out ASM exception for debug trap and abort                    *
 *=============================================================================================*/
int __cdecl _purecall(void)
{
	int return_code = 0;

#ifdef WWDEBUG
	// Break in the debugger and then abort
	WWDEBUG_SAY(("Pure Virtual Function call. Oh No!\n"));
#ifdef _WIN32
	DebugBreak();
#else
	raise(SIGTRAP);
#endif //_WIN32
	abort();
#endif	//_DEBUG_ASSERT

	return(return_code);
}



/***********************************************************************************************
 * Last_Error_Text -- Get the system error text for GetLastError                                *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Ptr to error string                                                               *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   8/14/98 11:11AM ST : Created                                                              *
 *=============================================================================================*/
char const * Last_Error_Text(void)
{
	static char message_buffer[256];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &message_buffer[0], 256, NULL);
	return (message_buffer);
}



/***********************************************************************************************
 * Add_Txt -- Add the given text to the machine state dump buffer.                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Text                                                                              *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    7/22/97 12:21PM ST : Created                                                             *
 *=============================================================================================*/
static void Add_Txt (char const *txt)
{
	if (strlen(ExceptionText) + strlen(txt) < 65535) {
		strcat(ExceptionText, txt);
	}
#if (0)
	/*
	** Log to debug output too.
	*/
	static char _debug_output_txt[512];
	const char *in = txt;
	char *out = _debug_output_txt;
	bool done = false;

	if (strlen(txt) < sizeof(_debug_output_txt)) {
		for (int i=0 ; i<sizeof(_debug_output_txt) ; i++) {

			switch (*in) {
				case '\r':
					in++;
					continue;

				case 0:
					done = true;
					// fall through

				default:
					*out++ = *in++;
					break;
			}

			if (done) {
				break;
			}
		}

		DebugString(_debug_output_txt);
	}
#endif //(0)
}



/***********************************************************************************************
 * Dump_Exception_Info -- Dump machine state information into a buffer                         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to exception information                                                      *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    7/22/97 12:21PM ST : Created                                                             *
 *    03/04/2025 CFE     : Converted for win64. Switched to safe print functions.              *
 *=============================================================================================*/
void Dump_Exception_Info(EXCEPTION_POINTERS *e_info)
{
	/*
	** List of possible exceptions
	*/
	static constexpr unsigned int _codes[] = {
		EXCEPTION_ACCESS_VIOLATION,
		EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
		EXCEPTION_BREAKPOINT,
		EXCEPTION_DATATYPE_MISALIGNMENT,
		EXCEPTION_FLT_DENORMAL_OPERAND,
		EXCEPTION_FLT_DIVIDE_BY_ZERO,
		EXCEPTION_FLT_INEXACT_RESULT,
		EXCEPTION_FLT_INVALID_OPERATION,
		EXCEPTION_FLT_OVERFLOW,
		EXCEPTION_FLT_STACK_CHECK,
		EXCEPTION_FLT_UNDERFLOW,
		EXCEPTION_ILLEGAL_INSTRUCTION,
		EXCEPTION_IN_PAGE_ERROR,
		EXCEPTION_INT_DIVIDE_BY_ZERO,
		EXCEPTION_INT_OVERFLOW,
		EXCEPTION_INVALID_DISPOSITION,
		EXCEPTION_NONCONTINUABLE_EXCEPTION,
		EXCEPTION_PRIV_INSTRUCTION,
		EXCEPTION_SINGLE_STEP,
		EXCEPTION_STACK_OVERFLOW,
		0xffffffff
	};

	/*
	** Information about each exception type.
	*/
	static constexpr const char* _code_txt[] = {
		"Error code: EXCEPTION_ACCESS_VIOLATION\r\r\nDescription: The thread tried to read from or write to a virtual address for which it does not have the appropriate access.",
		"Error code: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\r\r\nDescription: The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.",
		"Error code: EXCEPTION_BREAKPOINT\r\r\nDescription: A breakpoint was encountered.",
		"Error code: EXCEPTION_DATATYPE_MISALIGNMENT\r\r\nDescription: The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.",
		"Error code: EXCEPTION_FLT_DENORMAL_OPERAND\r\r\nDescription: One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.",
		"Error code: EXCEPTION_FLT_DIVIDE_BY_ZERO\r\r\nDescription: The thread tried to divide a floating-point value by a floating-point divisor of zero.",
		"Error code: EXCEPTION_FLT_INEXACT_RESULT\r\r\nDescription: The result of a floating-point operation cannot be represented exactly as a decimal fraction.",
		"Error code: EXCEPTION_FLT_INVALID_OPERATION\r\r\nDescription: Some strange unknown floating point operation was attempted.",
		"Error code: EXCEPTION_FLT_OVERFLOW\r\r\nDescription: The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.",
		"Error code: EXCEPTION_FLT_STACK_CHECK\r\r\nDescription: The stack overflowed or underflowed as the result of a floating-point operation.",
		"Error code: EXCEPTION_FLT_UNDERFLOW\r\r\nDescription:	The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.",
		"Error code: EXCEPTION_ILLEGAL_INSTRUCTION\r\r\nDescription:	The thread tried to execute an invalid instruction.",
		"Error code: EXCEPTION_IN_PAGE_ERROR\r\r\nDescription:	The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.",
		"Error code: EXCEPTION_INT_DIVIDE_BY_ZERO\r\r\nDescription: The thread tried to divide an integer value by an integer divisor of zero.",
		"Error code: EXCEPTION_INT_OVERFLOW\r\r\nDescription: The result of an integer operation caused a carry out of the most significant bit of the result.",
		"Error code: EXCEPTION_INVALID_DISPOSITION\r\r\nDescription: An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.",
		"Error code: EXCEPTION_NONCONTINUABLE_EXCEPTION\r\r\nDescription: The thread tried to continue execution after a noncontinuable exception occurred.",
		"Error code: EXCEPTION_PRIV_INSTRUCTION\r\r\nDescription: The thread tried to execute an instruction whose operation is not allowed in the current machine mode.",
		"Error code: EXCEPTION_SINGLE_STEP\r\r\nDescription: A trace trap or other single-instruction mechanism signaled that one instruction has been executed.",
		"Error code: EXCEPTION_STACK_OVERFLOW\r\r\nDescription: The thread used up its stack.",
		"Error code: ?????\r\r\nDescription: Unknown exception."
	};

	DebugString("Dump exception info\n");

	// Used a lot in this function
	const HANDLE current_proc = GetCurrentProcess();

	/*
	** Scrap buffer for constructing dump strings
	*/
	char scrap [256];

	/*
	** Clear out the dump buffer
	*/
	memset(ExceptionText, 0, sizeof (ExceptionText));

	/*
	** Retrieve the programs symbols if they are available
	*/
	Load_Symbols();

	unsigned char symbol [256];
	uintptr_t displacement;
	IMAGEHLP_SYMBOL *symptr = (IMAGEHLP_SYMBOL*)&symbol;

	/*
	** Get the exception address and the machine context at the time of the exception
	*/
	const CONTEXT* const context = e_info->ContextRecord;

	/*
	** The following are set for access violation only
	*/
	int access_read_write=-1;
	uintptr_t access_address = 0;

	if (e_info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		DebugString("Exception Handler: Exception is access violation\n");
		access_read_write = e_info->ExceptionRecord->ExceptionInformation[0];  // 0=read, 1=write
		access_address = e_info->ExceptionRecord->ExceptionInformation[1];
	} else {
		DebugString ("Exception Handler: Exception code is %d\n", e_info->ExceptionRecord->ExceptionCode);
	}

	/*
	** Match the exception type with the error string and print it out
	*/
	int i;

	for (i=0 ; _codes[i] != 0xffffffff ; i++) {
		if (_codes[i] == e_info->ExceptionRecord->ExceptionCode) {
			DebugString("Exception Handler: Found exception description\n");
			break;
		}
	}
	Add_Txt(_code_txt[i]);
	Add_Txt("\r\n");

	/*
	** For access violations, print out the violation address and if it was read or write.
	*/
	if (e_info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
		snprintf(scrap, sizeof(scrap), "Access address:" PRIPTRFMT " ", access_address);
		Add_Txt(scrap);
		if (access_read_write) {
			Add_Txt("was written to.\r\n");
		} else {
			Add_Txt("was read from.\r\n");
		}
	}


	/*
	** If symbols are available, print out the exception eip address and the name of the
	** function it represents.
	*/
#ifdef _WIN64
	const auto executionAddress = context->Rip;
#else
	const auto executionAddress = context->Eip;
#endif //_WIN64
	memset(symptr, 0, sizeof (IMAGEHLP_SYMBOL));
	symptr->SizeOfStruct = sizeof (IMAGEHLP_SYMBOL);
	symptr->MaxNameLength = 256-sizeof (IMAGEHLP_SYMBOL);
	symptr->Size = 0;
	symptr->Address = executionAddress;

	if (!IsBadCodePtr((FARPROC)executionAddress)) {
		if (_SymGetSymFromAddr != NULL && _SymGetSymFromAddr (GetCurrentProcess(), executionAddress, &displacement, symptr)) {
			snprintf(scrap, sizeof(scrap), "Exception occurred at " PRIPTRFMT " - %s + " PRIPTRFMT "\r\n", executionAddress, symptr->Name, displacement);
		} else {
			DebugString ("Exception Handler: Failed to get symbol for EIP\r\n");
			if (_SymGetSymFromAddr != NULL) {
				DebugString ("Exception Handler: SymGetSymFromAddr failed with code %d - %s\n", GetLastError(), Last_Error_Text());
			}
			snprintf(scrap, sizeof(scrap), "Exception occurred at " PRIPTRFMT "\r\n", executionAddress);
		}
	} else {
		DebugString ("Exception Handler: executionAddress is bad code pointer\n");
	}

	Add_Txt (scrap);

	/*
	** Try to walk the stack. It might work....
	*/
	DebugString("Stack walk...\n");
	Add_Txt("\r\n  Stack walk...\r\n");

	std::array<uintptr_t, 256> return_addresses;
#ifdef _WIN64
	const size_t num_addresses = Stack_Walk(context->Rip, context->Rsp, context->Rbp, 0, return_addresses);
#else
	const size_t num_addresses = Stack_Walk(context->Eip, context->Esp, context->Ebp, 0, return_addresses);
#endif //_WIN64

	if (num_addresses)
	{
		char symbuf[256];
		for (size_t s = 0; s < num_addresses; ++s)
		{
			uintptr_t temp_addr = return_addresses[s];
			displacement = 0;

			for (int space = 0 ; space <= s ; space++)
			{
				Add_Txt("  ");
			}

			if (SymbolsAvailable)
			{
				symptr->SizeOfStruct = sizeof(symbol);
				symptr->MaxNameLength = 128;
				symptr->Size = 0;
				symptr->Address = temp_addr;

				if (_SymGetSymFromAddr && _SymGetSymFromAddr(current_proc, temp_addr, &displacement, symptr))
				{
					snprintf(symbuf, sizeof(symbuf), "%s + " PRIPTRFMT "\r\n", symptr->Name, displacement);
					Add_Txt(symbuf);
				}
			}
			else
			{
				snprintf(symbuf, sizeof(symbuf), "" PRIPTRFMT "\r\n", temp_addr);
				Add_Txt(symbuf);
			}
		}

		Add_Txt("\r\n\r\n");
	}
	else
	{
		DebugString("Stack walk failed!\n");
		Add_Txt("Stack walk failed!\r\n");
	}


#if (0)	//Don't have this info yet for Renegade.
	/*
	** Add in the version info.
	*/
	snprintf(scrap, sizeof(scrap), "\r\nVersion %s\r\n", Version_Name());
	Add_Txt(scrap);

	snprintf(scrap, sizeof(scrap), "Internal Version %s\r\n", VerNum.Version_Name());
	Add_Txt(scrap);

	char buildinfo[128];
	buildinfo[0] = 0;
	Build_Date_String(buildinfo, 128);

	char build_number[128];
	char build_name[128];

#endif	//(0)

	if (AppVersionCallback) {
		snprintf(scrap, sizeof(scrap), "%s\r\n\r\n", AppVersionCallback());
		Add_Txt(scrap);
	}

	/*
	** Thread list.
	*/
	Add_Txt("Thread list\r\n");

	/*
	** Get the thread info from ThreadClass.
	*/
	for (int thread = 0; thread < ThreadList.Count(); ++thread)
	{
		snprintf(scrap, sizeof(scrap), "  ID: %08X - %s", ThreadList[thread]->ThreadID, ThreadList[thread]->ThreadName);
		Add_Txt(scrap);
		if (GetCurrentThreadId() == ThreadList[thread]->ThreadID)
			Add_Txt("   ***CURRENT THREAD***");
		Add_Txt("\r\n");
	}

	/*
	** CPU type
	*/
	snprintf(scrap, sizeof(scrap), "\r\nCPU %s, %d Mhz, Vendor: %s\r\n", (char*)CPUDetectClass::Get_Processor_String(), Get_RDTSC_CPU_Speed(), (char*)CPUDetectClass::Get_Processor_Manufacturer_Name());
	Add_Txt(scrap);


	Add_Txt("\r\nDetails:\r\n");

	DebugString("Register dump...\n");

	/*
	** Dump the registers.
	*/
#ifdef _WIN64
	snprintf(scrap, sizeof(scrap), "Rip:" PRIPTRFMT "\tRsp:" PRIPTRFMT "\tRbp:" PRIPTRFMT "\r\n", context->Rip, context->Rsp, context->Rbp);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "Rax:" PRIPTRFMT "\tRbx:" PRIPTRFMT "\tRcx:" PRIPTRFMT "\r\n", context->Rax, context->Rbx, context->Rcx);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "Rdx:" PRIPTRFMT "\tRsi:" PRIPTRFMT "\tRdi:" PRIPTRFMT "\r\n", context->Rdx, context->Rsi, context->Rdi);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "EFlags: %08X\r\n", context->EFlags);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "CS:%04x  SS:%04x  DS:%04x  ES:%04x  FS:%04x  GS:%04x\r\n", context->SegCs, context->SegSs, context->SegDs, context->SegEs, context->SegFs, context->SegGs);
	Add_Txt(scrap);
#else
	snprintf(scrap, sizeof(scrap), "Eip:" PRIPTRFMT "\tEsp:" PRIPTRFMT "\tEbp:" PRIPTRFMT "\r\n", executionAddress, context->Esp, context->Ebp);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "Eax:" PRIPTRFMT "\tEbx:" PRIPTRFMT "\tEcx:" PRIPTRFMT "\r\n", context->Eax, context->Ebx, context->Ecx);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "Edx:" PRIPTRFMT "\tEsi:" PRIPTRFMT "\tEdi:" PRIPTRFMT "\r\n", context->Edx, context->Esi, context->Edi);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "EFlags:" PRIPTRFMT " \r\n", context->EFlags);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "CS:%04x  SS:%04x  DS:%04x  ES:%04x  FS:%04x  GS:%04x\r\n", context->SegCs, context->SegSs, context->SegDs, context->SegEs, context->SegFs, context->SegGs);
	Add_Txt(scrap);
#endif //_WIN64

	
#ifdef _WIN64
	const auto& floatSave = context->FltSave;
#else
	const auto& floatSave = context->FloatSave;
#endif //_WIN64

	/*
	** Now the FP registers.
	*/
	Add_Txt("\r\nFloating point status\r\n");
	snprintf(scrap, sizeof(scrap), "     Control word: %08X\r\n", floatSave.ControlWord);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "      Status word: %08X\r\n", floatSave.StatusWord);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "         Tag word: %08X\r\n", floatSave.TagWord);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "     Error Offset: %08X\r\n", floatSave.ErrorOffset);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "   Error Selector: %08X\r\n", floatSave.ErrorSelector);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "      Data Offset: %08X\r\n", floatSave.DataOffset);
	Add_Txt(scrap);
	snprintf(scrap, sizeof(scrap), "    Data Selector: %08X\r\n", floatSave.DataSelector);
	Add_Txt(scrap);
	//snprintf(scrap, sizeof(scrap), "      Cr0NpxState: " PRIPTRFMT "\r\n", floatSave.Cr0NpxState);
	//Add_Txt(scrap);

	// x87 registers are only present in the 32-bit version of the context structure
	// This entire file needs replacing for non-windows platforms
#if !defined(_WIN64)
	for (int fp=0 ; fp < 8; fp++) {
		snprintf(scrap, sizeof(scrap), "ST%d : ", fp);
		Add_Txt(scrap);
		for (int b=0 ; b<10 ; b++) {
			snprintf(scrap, sizeof(scrap), "%02X", floatSave.RegisterArea[(fp*10) + b]);
			Add_Txt(scrap);
		}

		void *fp_data_ptr = (void*)(&floatSave.RegisterArea[fp*10]);

		snprintf(scrap, sizeof(scrap), "   %+#.17e\r\n", reinterpret_cast<double*>(floatSave.RegisterArea[fp*10]));
		Add_Txt(scrap);
	}
#endif //!defined(_WIN64)

	/*
	** Dump the bytes at the execution address. This will make it easier to match the crash address with later versions of the game.
	*/
	DebugString("EIP bytes dump...\n");
	snprintf(scrap, sizeof(scrap), "\r\nBytes at CS:EIP (" PRIPTRFMT ")  : ", executionAddress);

	unsigned char *execution_ptr = (unsigned char *) (executionAddress);
	char bytestr[32];

	for (int c = 0 ; c < 32 ; c++) {
		if (IsBadReadPtr(execution_ptr, 1)) {
			strcat(scrap, "?? ");
		} else {
			snprintf(bytestr, sizeof(bytestr), "%02X ", *execution_ptr);
			strcat(scrap, bytestr);
		}
		execution_ptr++;
	}

	strcat(scrap, "\r\n\r\n");
	Add_Txt(scrap);

	/*
	** Dump out the values on the stack.
	*/
	DebugString("Stack dump...\n");
	Add_Txt("Stack dump (* indicates possible code address) :\r\n");
#ifdef _WIN64
	uintptr_t* stackptr = (uintptr_t*)context->Rsp;
#else
	uintptr_t* stackptr = (uintptr_t*) context->Esp;
#endif // _WIN64

	for (int j = 0; j < 2048 ; ++j)
	{
		if (IsBadReadPtr(stackptr, 4))
		{
			/*
			** The stack contents cannot be read so just print up question marks.
			*/
			snprintf(scrap, sizeof(scrap), "%08p: ", stackptr);
			strcat(scrap, "????????\r\n");
		}
		else
		{
			/*
			** If this stack address is in our memory space then try to match it with a code symbol.
			*/
			if (IsBadCodePtr((FARPROC)*stackptr))
			{
				snprintf(scrap, sizeof(scrap), "%08p: " PRIPTRFMT " ", stackptr, *stackptr);
				strcat(scrap, "DATA_PTR\r\n");
			}
			else
			{
				snprintf(scrap, sizeof(scrap), "%08p: " PRIPTRFMT "", stackptr, *stackptr);

				if (SymbolsAvailable)
				{
					symptr->SizeOfStruct = sizeof(symbol);
					symptr->MaxNameLength = 128;
					symptr->Size = 0;
					symptr->Address = *stackptr;

					if (_SymGetSymFromAddr && _SymGetSymFromAddr (current_proc, *stackptr, &displacement, symptr))
					{
						char symbuf[256];
						snprintf(symbuf, sizeof(symbuf), " - %s + " PRIPTRFMT "", symptr->Name, displacement);
						strcat(scrap, symbuf);
					}
				} else
				{
					strcat (scrap, " *");
				}
				strcat (scrap, "\r\n");
			}
		}
		Add_Txt(scrap);
		stackptr++;
	}

	Add_Txt ("\r\n\r\n");

	// Cleanup
	Unload_DbgHelp_And_Symbols();
}





/***********************************************************************************************
 * Exception_Handler -- Exception handler filter function                                      *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    exception code                                                                    *
 *           pointer to exception information pointers                                         *
 *                                                                                             *
 * OUTPUT:   EXCEPTION_EXECUTE_HANDLER -- Excecute the body of the __except construct          *
 *        or EXCEPTION_CONTINUE_SEARCH -- Pass this exception down to the debugger             *
 *        or EXCEPTION_CONTINUE_EXECUTION -- Continue to execute at the fault address          *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    7/22/97 12:29PM ST : Created                                                              *
 *=============================================================================================*/
int Exception_Handler(int exception_code, EXCEPTION_POINTERS *e_info)
{
	DebugString("Exception!\n");

#ifdef DEMO_TIME_OUT
	if ( !WindowedMode ) {
		Load_Title_Page("TITLE.PCX", true);
		MouseCursor->Release_Mouse();
		MessageBox(MainWindow, "This demo has timed out. Thank you for playing Red Alert 2.","Byeee!", MB_ICONEXCLAMATION|MB_OK);
		return (EXCEPTION_EXECUTE_HANDLER);
	}
#endif	//DEMO_TIME_OUT

	/*
	** If we were trying to quit and we got another exception then just shut down the whole shooting match right here.
	*/
	if (TryingToExit) {
		ExitProcess(0);
	}

	/*
	** Track recursions because we need to know if something here is failing.
	*/
	ExceptionRecursions++;

	if (ExceptionRecursions > 1) {
		return (EXCEPTION_CONTINUE_SEARCH);
	}

	/*
	** If there was a breakpoint then chances are it was set by a debugger. In _DEBUG mode
	** we probably should ignore breakpoints. Breakpoints become more significant in release
	** mode since there probably isn't a debugger present.
	*/
#ifdef _DEBUG
	if (exception_code == EXCEPTION_BREAKPOINT) {
		return (EXCEPTION_CONTINUE_SEARCH);
	}
#else
	exception_code = exception_code;
#endif	//_DEBUG


	if (ExceptionRecursions == 0) {

		/*
		** Create a dump of the exception info.
		*/
		Dump_Exception_Info(e_info);

		/*
		** Log the machine state to disk
		*/
		HANDLE debug_file;
		DWORD	actual;
		debug_file = CreateFile("_except.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (debug_file != INVALID_HANDLE_VALUE){
			WriteFile(debug_file, ExceptionText, strlen(ExceptionText), &actual, NULL);
			CloseHandle (debug_file);

#if (0)
#ifdef _DEBUG_PRINT
#ifndef _DEBUG
			/*
			** Copy the exception debug file to the network. No point in doing this for the debug version
			** since symbols are not normally available.
			*/
			DebugString ("About to copy debug file\n");
			char filename[512];
			if (Get_Global_Output_File_Name ("EXCEPT", filename, 512)) {
				DebugString ("Copying DEBUG.TXT to %s\n", filename);
				int result = CopyFile("debug.txt", filename, false);
				if (result == 0) {
					DebugString ("CopyFile failed with error code %d - %s\n", GetLastError(), Last_Error_Text());
				}
			}
			DebugString ("Debug file copied\n");
#endif	//_DEBUG
#endif	//_DEBUG_PRINT
#endif	//(0)

		}
	}

	/*
	** Call the apps callback function.
	*/
	if (AppCallback) {
		AppCallback();
	}

	/*
	** If an exit is required then turn of memory leak reporting (there will be lots of them) and use
	** EXCEPTION_EXECUTE_HANDLER to let us fall out of winmain.
	*/
	if (ExitOnException) {
#ifdef _DEBUG
		_CrtSetDbgFlag(0);
#endif //_DEBUG
		TryingToExit = true;

		unsigned long id = Get_Main_Thread_ID();
		if (id != GetCurrentThreadId()) {
			DebugString("Exiting due to exception in sub thread\n");
			ExitProcess(EXIT_SUCCESS);
		}

		return(EXCEPTION_EXECUTE_HANDLER);

	}
	return (EXCEPTION_CONTINUE_SEARCH);
}




/***********************************************************************************************
 * Register_Thread_ID -- Let the exception handler know about a thread                         *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Thread ID                                                                         *
 *           Thread name                                                                       *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   8/30/2001 3:04PM ST : Created                                                             *
 *=============================================================================================*/
void Register_Thread_ID(unsigned long thread_id, char *thread_name, bool main_thread)
{
	WWMEMLOG(MEM_GAMEDATA);
	if (thread_name) {

		/*
		** See if we already know about this thread. Maybe just the thread_id changed.
		*/
		for (int i=0 ; i<ThreadList.Count() ; i++) {
			if (strcmp(thread_name, ThreadList[i]->ThreadName) == 0) {
				ThreadList[i]->ThreadID = thread_id;
				return;
			}
		}

		ThreadInfoType *thread = new ThreadInfoType;
		thread->ThreadID = thread_id;
		strcpy(thread->ThreadName, thread_name);
		thread->Main = main_thread;
		thread->ThreadHandle = INVALID_HANDLE_VALUE;
		ThreadList.Add(thread);
	}
}


#if (0)
/***********************************************************************************************
 * Register_Thread_Handle -- Keep a copy of the thread handle that matches this thread ID      *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Thread ID                                                                         *
 *           Thread handle                                                                     *
 *                                                                                             *
 * OUTPUT:   True if thread ID was matched                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/6/2002 9:40PM ST : Created                                                              *
 *=============================================================================================*/
bool Register_Thread_Handle(unsigned long thread_id, HANDLE thread_handle)
{
	for (int i=0 ; i<ThreadList.Count() ; i++) {
		if (ThreadList[i]->ThreadID == thread_id) {
			ThreadList[i]->ThreadHandle = thread_handle;
			return(true);
			break;
		}
	}
	return(false);
}



/***********************************************************************************************
 * Get_Num_Threads -- Get the number of threads being tracked.                                 *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Number of threads we know about                                                   *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/6/2002 9:43PM ST : Created                                                              *
 *=============================================================================================*/
int Get_Num_Threads(void)
{
	return(ThreadList.Count());
}


/***********************************************************************************************
 * Get_Thread_Handle -- Get the handle for the given thread index                              *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Thread index                                                                      *
 *                                                                                             *
 * OUTPUT:   Thread handle                                                                     *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   2/6/2002 9:46PM ST : Created                                                              *
 *=============================================================================================*/
HANDLE Get_Thread_Handle(int thread_index)
{
	if (thread_index < ThreadList.Count()) {
		return(ThreadList[thread_index]->ThreadHandle);
	}
}
#endif //(0)

/***********************************************************************************************
 * Unregister_Thread_ID -- Remove a thread entry from the thread list                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Thread ID                                                                         *
 *           Thread name                                                                       *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   8/30/2001 3:10PM ST : Created                                                             *
 *=============================================================================================*/
void Unregister_Thread_ID(unsigned long thread_id, char *thread_name)
{
	for (int i=0 ; i<ThreadList.Count() ; i++) {
		if (strcmp(thread_name, ThreadList[i]->ThreadName) == 0) {
			assert(ThreadList[i]->ThreadID == thread_id);
			delete ThreadList[i];
			ThreadList.Delete(i);
			return;
		}
	}
}



/***********************************************************************************************
 * Get_Main_Thread_ID -- Get the ID of the processes main thread                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Thread ID                                                                         *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/6/2001 12:20PM ST : Created                                                            *
 *=============================================================================================*/
unsigned long Get_Main_Thread_ID(void)
{
	for (int i=0 ; i<ThreadList.Count() ; i++) {
		if (ThreadList[i]->Main) {
			return(ThreadList[i]->ThreadID);
		}
	}
	return(0);
}






/***********************************************************************************************
 * Load_Image_Helper -- Load imagehlp.dll and retrieve the programs symbols                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   6/12/2001 4:27PM ST : Created                                                             *
 *   3/08/2025       CFE : Replaced internally with a load symbols call.                       *
 *=============================================================================================*/
void Load_Image_Helper(void)
{
	Load_Symbols();
}

/***********************************************************************************************
 * Lookup_Symbol -- Get the symbol for a given code address                                    *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Address of code to get symbol for                                                 *
 *           Ptr to buffer to return symbol in                                                 *
 *           Reference to int to return displacement                                           *
 *                                                                                             *
 * OUTPUT:   True if symbol found                                                              *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   6/12/2001 4:47PM ST : Created                                                             *
 *=============================================================================================*/
bool Lookup_Symbol(void *code_ptr, char *symbol, int &displacement)
{
	/*
	** Locals.
	*/
	char symbol_struct_buf[1024];
	IMAGEHLP_SYMBOL *symbol_struct_ptr = (IMAGEHLP_SYMBOL *)symbol_struct_buf;

	/*
	** Set default values in case of early exit.
	*/
	displacement = 0;
	*symbol = '\0';

	/*
	** Make sure symbols are available.
	*/
	if (!SymbolsAvailable || _SymGetSymFromAddr == NULL) {
		return(false);
	}

	/*
	** If it's a bad code pointer then there is no point in trying to match it with a symbol.
	*/
	if (IsBadCodePtr((FARPROC)code_ptr)) {
		strcpy(symbol, "Bad code pointer");
		return(false);
	}

	/*
	** Set up the parameters for the call to SymGetSymFromAddr
	*/
	memset (symbol_struct_ptr, 0, sizeof (symbol_struct_buf));
	symbol_struct_ptr->SizeOfStruct = sizeof (symbol_struct_buf);
	symbol_struct_ptr->MaxNameLength = sizeof(symbol_struct_buf)-sizeof (IMAGEHLP_SYMBOL);
	symbol_struct_ptr->Size = 0;
	symbol_struct_ptr->Address = (uintptr_t)code_ptr;

	/*
	** See if we have the symbol for that address.
	*/
	if (_SymGetSymFromAddr(GetCurrentProcess(), (uintptr_t)code_ptr, (uintptr_t*)&displacement, symbol_struct_ptr)) {

		/*
		** Copy it back into the buffer provided.
		*/
		strcpy(symbol, symbol_struct_ptr->Name);
		return(true);
	}
	return(false);
}




/***********************************************************************************************
 * Stack_Walk -- Walk the stack and get the last n return addresses                            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Ptr to return address list                                                        *
 *           Number of return addresses to fetch                                               *
 *           Ptr to optional context. NULL means use current                                   *
 *                                                                                             *
 * OUTPUT:   Number of return addresses found                                                  *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   6/12/2001 11:57AM ST : Created                                                            *
 *   8/3/2025 CFE         : Made more compatible with x64. Removed asm.                        *
 *=============================================================================================*/
size_t Stack_Walk(const uintptr_t ip_in, const uintptr_t sp_in, const uintptr_t bp_in, const size_t skip_frames, std::span<uintptr_t> return_addresses)
{
	// Immediately early out if the return span is empty
	if (return_addresses.empty())
		return 0;

	/*
	** If this is the first time through then fix up the imagehelp function pointers since imagehlp.dll
	** can't be statically linked.
	*/
	Load_Image_Helper();

	/*
	** If there is no debug support .dll available then we can't walk the stack.
	*/
	if (!DbgHelp || !_StackWalk)
		return 0;

	/*
	** Set up the stack frame structure for the start point of the stack walk (i.e. here).
	*/
	STACKFRAME stack_frame{};

	stack_frame.AddrPC.Mode = AddrModeFlat;
	stack_frame.AddrPC.Offset = ip_in;
	stack_frame.AddrStack.Mode = AddrModeFlat;
	stack_frame.AddrStack.Offset = sp_in;
	stack_frame.AddrFrame.Mode = AddrModeFlat;
	stack_frame.AddrFrame.Offset = bp_in;

	/*
	** Walk the stack by the requested number of return address iterations.
	*/
	const size_t max_addresses = return_addresses.size();
	const HANDLE current_proc = GetCurrentProcess();
	const HANDLE current_thread = GetCurrentThread();

#ifdef _WIN64
	const DWORD machine_type = IMAGE_FILE_MACHINE_AMD64;
#else
	const DWORD machine_type = IMAGE_FILE_MACHINE_I386;
#endif

	// Skip frames
	for (size_t i = 0; i < skip_frames; ++i)
	{
		if (!_StackWalk(machine_type, current_proc, current_thread, &stack_frame, NULL, NULL, _SymFunctionTableAccess, _SymGetModuleBase, NULL))
			break;
	}

	// Read addresses
	size_t pointer_index = 0;
	while (pointer_index < max_addresses)
	{
		if (!_StackWalk(machine_type, current_proc, current_thread, &stack_frame, NULL, NULL, _SymFunctionTableAccess, _SymGetModuleBase, NULL))
			break;
		
		uintptr_t return_address = stack_frame.AddrReturn.Offset;
		return_addresses[pointer_index++] = return_address;
	}

	return pointer_index;
}



void Register_Application_Exception_Callback(void (*app_callback)(void))
{
	AppCallback = app_callback;
}

void Register_Application_Version_Callback(char *(*app_ver_callback)(void))
{
	AppVersionCallback = app_ver_callback;
}



void Set_Exit_On_Exception(bool set)
{
	ExitOnException = true;
}

bool Is_Trying_To_Exit(void)
{
	return(TryingToExit);
}




#endif	//_MSC_VER




