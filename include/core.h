#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C" {
#endif

// calls assembly function to wake up secondary cores 1, 2, and 3
void wake_up_cores();

#ifdef __cplusplus
}
#endif

#endif // CORE_H
