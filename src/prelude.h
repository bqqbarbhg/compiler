#pragma once

#define _CRT_SECURE_NO_WARNINGS

#ifdef _MSC_VER
#pragma warning(disable:4345)
#endif

#include <stdint.h>
#include <stddef.h>

#define ArrayCount(arr) (sizeof(arr) / sizeof(*(arr)))
