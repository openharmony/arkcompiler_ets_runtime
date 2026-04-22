/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LINXKIT_ATOMIC_H
#define LINXKIT_ATOMIC_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

bool __attribute__((always_inline)) LinxkitAtomicUpdateHalfword(uint16_t *ptr, uint16_t newState)
{
    asm volatile (
        "stlrh %w0, [%1]\n"
        :
        : "r" (newState), "r" (ptr)
        : "memory"
    );

    return true;
}

bool __attribute__((always_inline)) LinxkitAtomicCmpUpdateState(uint32_t *ptr, uint32_t oldVal, uint16_t newState)
{
    uint32_t res;
    uint32_t tmp;
    asm volatile (
        "ldxr %w1, [%3]\n"
        "cmp %w1, %w4\n"
        "mov %w0, #1\n"
        "b.ne 1f\n"
        "bfi %w1, %w2, #16, #16\n"
        "stlxr %w0, %w1, [%3]\n"
        "1: clrex\n"
        : "=&r" (res), "=&r" (tmp)
        : "r" (newState), "r" (ptr), "r" (oldVal)
        : "memory", "cc"
    );

    return !res;
}

#ifdef __cplusplus
}
#endif
#endif
