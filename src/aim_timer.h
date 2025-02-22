#pragma once

internal LONGLONG aim_timer_get_os_freq();
internal LONGLONG aim_timer_get_os_time();
internal DWORD64 aim_timer_cpu_cycles();
internal double aim_timer_ticks_to_sec(LONGLONG time, LONGLONG frequency);
internal double aim_timer_ticks_to_ms(LONGLONG time, LONGLONG frequency);