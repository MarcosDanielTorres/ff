#pragma once
#include <stdio.h>
#include "aim_timer.h"

#ifdef AIM_PROFILER
struct ProfiledAnchor {
	const char* label;
	LONGLONG elapsed_inclusive;
	LONGLONG elapsed_exclusive;
	LONGLONG cpu_start;
	uint64_t bytes_count;
	uint32_t hitcount{ 0 };
};

static ProfiledAnchor profiled_anchors[100];
static uint32_t global_parent_index;
//static int profiler_address_test = (int)&profiler;

struct ProfiledBlock {
	const char* label;
	uint32_t index;
	uint32_t parent_index;
	LONGLONG time_start;
	LONGLONG OldTSCElapsedAtRoot;

	ProfiledBlock(const char* label, uint32_t index, uint64_t bytes_count);
	~ProfiledBlock();
};

#define NameConcat2(A, B) A##B
#define NameConcat(A, B) NameConcat2(A, B)

#define aim_profiler_bandwidth(name, bytes_count) ProfiledBlock NameConcat(Block, __LINE__)(name, __COUNTER__ + 1, bytes_count);
#define aim_profiler_time_block(name) aim_profiler_bandwidth(name, 0);
#define aim_profiler_time_function aim_profiler_time_block(__func__);
#else
#define aim_profiler_bandwidth(...)
#define aim_profiler_time_block(...) 
#define aim_profiler_time_function
#endif

struct Profiler {
	LONGLONG os_freq;
	LONGLONG os_start;
	LONGLONG os_end;
};
static Profiler profiler;
void aim_profiler_begin();
void aim_profiler_end();
void aim_profiler_print();
