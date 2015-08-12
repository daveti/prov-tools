#include <iostream>
#include <sys/time.h>
#include <vector>
#include "snap_handlers.hpp"
#include <sys/stat.h>

int plog_process(FILE *log);

int plog_process_raw(int log, ProvenanceGraph & graph);

void stat_test(ProvenanceGraph);
