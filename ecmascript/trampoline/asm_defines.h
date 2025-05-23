/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TRAMPOLINES_ASM_DEFINES_H
#define ECMASCRIPT_TRAMPOLINES_ASM_DEFINES_H

#define REG_PAIR_SIZE_ARM64 16
#define PRE_SP_OFFSET_ARM64 176
#define RETURN_ADDRESS_OFFSET_ARM64 168
#define PRE_SP_OFFSET_X64 64
#define RETURN_ADDRESS_OFFSET_X64 56
#define LAZY_DEOPT_TYPE_OFFSET 1

#ifdef PANDA_TARGET_MACOS
    #define LazyDeoptEntryName _LazyDeoptEntry
    #define GetGlueFromThreadLocalName _GetGlueFromThreadLocal
    #define GetFixedReturnAddrName _GetFixedReturnAddr
    #define GetDeoptHandlerAsmOffsetName _GetDeoptHandlerAsmOffset
#else
    #define LazyDeoptEntryName LazyDeoptEntry
    #define GetGlueFromThreadLocalName GetGlueFromThreadLocal
    #define GetFixedReturnAddrName GetFixedReturnAddr
    #define GetDeoptHandlerAsmOffsetName GetDeoptHandlerAsmOffset
#endif
#endif  // ECMASCRIPT_TRAMPOLINES_ASM_DEFINES_H