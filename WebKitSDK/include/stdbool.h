/*
 * Copyright (c) 2007 Apple Inc.
 * All rights reserved.
 *
 */

#ifndef __STDBOOL_H__
#define __STDBOOL_H__	1

#define __bool_true_false_are_defined   1

#ifndef __cplusplus

#define false   0
#define true    1

#define bool    _Bool

#ifndef COCOTRON
typedef unsigned char _Bool;
#endif

#endif /* !__cplusplus */

#endif

