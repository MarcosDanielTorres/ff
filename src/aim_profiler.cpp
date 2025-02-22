#ifdef AIM_PROFILER
void aim_profiler_print() {
	LONGLONG total_time = profiler.os_end - profiler.os_start;
    char buf[100];
    sprintf(buf, "Total execution time: %f ms\n", aim_timer_ticks_to_ms(total_time, profiler.os_freq));
    OutputDebugStringA(buf);
	for (int i = 1; i < sizeof(profiled_anchors) / sizeof(profiled_anchors[0]); i++) {
		ProfiledAnchor anchor = profiled_anchors[i];
		if (anchor.elapsed_inclusive) {
            char buf[500];
            sprintf(buf, "   %s[%d] %.4fms (%.4f%%) ",
                   anchor.label,
                   anchor.hitcount,
                   aim_timer_ticks_to_ms(anchor.elapsed_inclusive, profiler.os_freq),
                   ((double)anchor.elapsed_inclusive / (double)total_time) * 100.0f);
            OutputDebugStringA(buf);
            
			if (anchor.elapsed_exclusive != anchor.elapsed_inclusive) {
                char buf[500];
                sprintf(buf, "without children: %.4fms (%.4f%%)", aim_timer_ticks_to_ms(anchor.elapsed_exclusive, profiler.os_freq), ((double)(anchor.elapsed_exclusive)) / (double)(total_time) * 100.0f);
                OutputDebugStringA(buf);
			}
            
			if (anchor.bytes_count) {
				uint64_t mb =  mb(1);
				uint64_t gb = gb(1);
                char buf[500];
                sprintf(buf, "%.3f mb at %.2f gb/s", (double)anchor.bytes_count / mb, (((double)anchor.bytes_count / gb) / aim_timer_ticks_to_sec(anchor.elapsed_inclusive, profiler.os_freq)));
                OutputDebugStringA(buf);
			}
			printf("\n");
		}
	}
}

ProfiledBlock::ProfiledBlock(const char* label, uint32_t index, uint64_t bytes_count) {
	this->parent_index = global_parent_index;
	this->label = label;
	this->index = index;
	this->time_start = aim_timer_get_os_time();
    
	ProfiledAnchor* anchor = &profiled_anchors[this->index];
	OldTSCElapsedAtRoot = anchor->elapsed_inclusive;
	anchor->bytes_count += bytes_count;
	if (OldTSCElapsedAtRoot != 0) {
		//abort();
	}
    
	global_parent_index = index;
}

ProfiledBlock::~ProfiledBlock() {
	LONGLONG elapsed = aim_timer_get_os_time() - this->time_start;
	ProfiledAnchor* anchor = &profiled_anchors[this->index];
	ProfiledAnchor* parent = &profiled_anchors[this->parent_index];
	anchor->label = this->label;
	anchor->hitcount++;
    
	/*
		parent->elapsed	+= elapsed				exclusive
		anchor->TSCElapsedAtRoot = Elapsed;		inclusive
	*/
    
	parent->elapsed_exclusive -= elapsed;
	anchor->elapsed_exclusive += elapsed;
	anchor->elapsed_inclusive = OldTSCElapsedAtRoot + elapsed;
    
	global_parent_index = this->parent_index;
}
#else
void aim_profiler_print() {}
#endif

void aim_profiler_begin() {
	profiler.os_freq = aim_timer_get_os_freq();
	profiler.os_start = aim_timer_get_os_time();
	//profiler.cpu_start = aim_timer_cpu_cycles();
}

void aim_profiler_end() {
	profiler.os_end = aim_timer_get_os_time();
	//profiler.cpu_end = aim_timer_cpu_cycles();
	aim_profiler_print();
}
