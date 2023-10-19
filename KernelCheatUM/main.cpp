#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <memory>
#include <string_view>
#include <cstdint>
#include <vector>
#include <math.h>
#include <stdio.h>

typedef struct _NULL_MEMORY
{
	void* buffer_address;
	UINT_PTR address;
	ULONGLONG size;
	ULONG pid;
	BOOLEAN write;
	BOOLEAN read;
	BOOLEAN req_base;
	BOOLEAN draw_box;
	BOOLEAN draw_line;
	int		r, g, b, x, y, w, h, t;
	int     dx, dy;
	int     x_step, y_step;
	void* output;
	const char* module_name;
	ULONG64 base_address;
}NULL_MEMORY;

uintptr_t base_address = 0;
std::uint32_t process_id = 0;
HDC hdc;

template<typename ... Arg>
uint64_t call_hook(const Arg ... args)
{
	void* hooked_func = GetProcAddress(LoadLibrary("win32u.dll"), "NtDxgkGetTrackedWorkloadStatistics");

	auto func = static_cast<uint64_t(_stdcall*)(Arg...)>(hooked_func);

	return func(args ...);
}

struct HandleDisposer
{
	using pointer = HANDLE;
	void operator()(HANDLE handle) const
	{
		if (handle != NULL || handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
		}
	}
};

using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;

std::uint32_t get_process_id(std::string_view process_name)
{
	PROCESSENTRY32 processentry;
	const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL));

	if (snapshot_handle.get() == INVALID_HANDLE_VALUE)
		return NULL;

	processentry.dwSize = sizeof(MODULEENTRY32);

	while (Process32Next(snapshot_handle.get(), &processentry) == TRUE)
	{
		if (process_name.compare(processentry.szExeFile) == NULL)
		{
			return processentry.th32ProcessID;
		}
	}
	return NULL;
}

static ULONG64 get_module_base_address(const char* module_name)
{
	NULL_MEMORY instructions = { 0 };
	instructions.pid = process_id;
	instructions.req_base = TRUE;
	instructions.read = FALSE;
	instructions.write = FALSE;
	instructions.draw_box = FALSE;
	instructions.module_name = module_name;
	call_hook(&instructions);
	
	ULONG64 base = NULL;
	base = instructions.base_address;
	return base;
}
template <class T>
T Read(UINT_PTR read_address)
{
	T response{};
	NULL_MEMORY instructions;
	instructions.pid = process_id;
	instructions.size = sizeof(T);
	instructions.address = read_address;
	instructions.read = TRUE;
	instructions.write = FALSE;
	instructions.req_base = FALSE;
	instructions.draw_box = FALSE;
	instructions.output = &response;
	call_hook(&instructions);

	return response;
}

bool write_memory(UINT_PTR write_address, UINT_PTR source_address, SIZE_T write_size)
{
	NULL_MEMORY instructions;
	instructions.address = write_address;
	instructions.pid = process_id;
	instructions.write = TRUE;
	instructions.read = FALSE;
	instructions.req_base = FALSE;
	instructions.draw_box = FALSE;
	instructions.buffer_address = (void*)source_address;
	instructions.size = write_size;

	call_hook(&instructions);

	return true;
}

bool draw_box(int x, int y, int w, int h, int t, int r, int g, int b)
{
	NULL_MEMORY instructions;
	instructions.write = FALSE;
	instructions.read = FALSE;
	instructions.req_base = FALSE;
	instructions.draw_box = TRUE;
	instructions.draw_line = FALSE;
	
	instructions.x = x;
	instructions.y = y;
	instructions.w = w;
	instructions.h = h;
	instructions.t = t;

	instructions.r = r;
	instructions.g = g;
	instructions.b = b;

	instructions.dx = -1;
	instructions.dy = -1;

	call_hook(&instructions);

	return true;
}


bool draw_line(int x, int y, int dx, int dy, int t, int r, int g, int b) 
{
	NULL_MEMORY instructions;
	instructions.write = FALSE;
	instructions.read = FALSE;
	instructions.req_base = FALSE;
	instructions.draw_box = FALSE;
	instructions.draw_line = TRUE;

	instructions.x = x;
	instructions.y = y;
	instructions.w = -1;
	instructions.h = -1;
	instructions.t = t;

	instructions.r = r;
	instructions.g = g;
	instructions.b = b;

	instructions.dx = dx;
	instructions.dy = dy;

	instructions.x_step = int(sqrt((float)((dx - x) * (dx - x) + (dy - y) * (dy - y))) / (dx - x));
	instructions.y_step = int(sqrt((float)((dx - x) * (dx - x) + (dy - y) * (dy - y))) / (dy - y));

	call_hook(&instructions);

	return true;
}

template<typename S>
bool write(UINT_PTR write_address, const S& value)
{
	return write_memory(write_address, (UINT_PTR)&value, sizeof(S));
}

int main()
{
	LoadLibrary("gdi32.dll");
	LoadLibrary("user32.dll");

	while (true)
	{
		draw_box(150, 150, 150, 150, 3, 0, 255, 0);
		draw_line(150, 150, 300, 300, 2, 0, 255, 0);
		draw_line(150, 300, 300, 150, 2, 0, 255, 0);
	}

	system("pause");
}