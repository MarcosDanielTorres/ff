internal LONGLONG aim_timer_get_os_freq() {
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return frequency.QuadPart;
}

internal LONGLONG aim_timer_get_os_time() {
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result.QuadPart;
}

internal DWORD64 aim_timer_cpu_cycles() {
	return __rdtsc();
}

internal double aim_timer_ticks_to_sec(LONGLONG time, LONGLONG frequency) {
	double result = ((double)time / (double)frequency);
	return result;
}

internal double aim_timer_ticks_to_ms(LONGLONG time, LONGLONG frequency) {
	double result = ((double)time * 1000.0f / (double)frequency);
	return result;
}
