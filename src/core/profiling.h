/**
 * file:    profiling.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef PROFILING_H
#define PROFILING_H

#ifndef PROFILE_TIMERS_ENABLE
#define PROFILE_TIMERS_ENABLE 1
#endif

#include "core/array.h"

struct ProfileTimers {
	SARRAY(const char*) names;
	SARRAY(u64)         cycles;
	SARRAY(u64)         cycles_last;
	SARRAY(bool)        open;
};

struct ProfileState {
	ProfileTimers timers;
	ProfileTimers prev_timers;
};


#if PROFILE_TIMERS_ENABLE

#define NUM_PROFILE_TIMERS (256)

#define PROFILE_START(name)\
	u64 start_##name = rdtsc();\
	i32 profile_timer_id_##name = profile_start_timer(#name)

#define PROFILE_END(name)\
	u64 end_##name = rdtsc();\
	u64 difference_##name = end_##name - start_##name;\
	profile_end_timer(profile_timer_id_##name, difference_##name)

#define PROFILE_BLOCK(name) ProfileBlock profile_block_##name(#name)
#define PROFILE_FUNCTION() ProfileBlock profile_block_##__FUNCTION__(__FUNCTION__)

#else

#define PROFILE_START(...)
#define PROFILE_END(...)

#define PROFILE_BLOCK(...)
#define PROFILE_FUNCTION(...)

#endif

#endif /* PROFILING_H */

