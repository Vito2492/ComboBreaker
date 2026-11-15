// Which kind of machine the program is built for, where that changes what
// it does. UC_PHONE: a phone or tablet (Android, iOS): touch controls, no
// compiler to run. UC_IOS: an iPhone or iPad, which also can't start
// programs or load code made at run time, so the game runs on the
// interpreter.
#pragma once
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined TARGET_OS_IPHONE && TARGET_OS_IPHONE
#define UC_IOS 1
#endif
#if defined __ANDROID__ || defined UC_IOS
#define UC_PHONE 1
#endif
