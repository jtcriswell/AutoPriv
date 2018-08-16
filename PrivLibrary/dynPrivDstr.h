/*
 * This library defines functions that executed instructions dynamically.
 * It is a helper library for the PrivAnalysis passes.
 *
 * @author Jie Zhou
 *
 * */


#ifndef __DYNPRIVCOUNT_H__
#define __DYNPRIVCOUNT_H__

#if defined(__cplusplus)
extern "C" {
#endif

extern uint64_t total_instr;

void initDynCount(uint64_t initialPrivSet);

void addBBLOI(uint32_t LOI);

void addPrivRmLOI(uint32_t LOI, uint64_t rmPrivSet);
uint64_t getNewPrivSet(uint64_t rmPrivSet);

void addIDChangeLOI(uint32_t LOI);

void reportPrivDstr();

void dumpPrivSet(uint64_t privSet);

void do_log(const char *str);

// for debugging
void forkHandler();
void calledAtStart();

#if defined(__cplusplus)
}
#endif

#endif // end of define __DYNPRIVCOUNT_H__

