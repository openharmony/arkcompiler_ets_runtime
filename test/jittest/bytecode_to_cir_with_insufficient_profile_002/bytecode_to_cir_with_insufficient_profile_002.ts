/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function Test(value) {
    // MUL2_IMM8_V8
    var tmp = value * 1.1;
    // NEWOBJRANGE_IMM8_IMM8_V8
    let arr = new Int32Array(1);
    // STOBJBYVALUE_IMM8_V8_V8
    arr[0] = tmp;
    // LDOBJBYVALUE_IMM8_V8
    // CALLARG1_IMM8_V8
    print(arr[0]);

    // ADD2_IMM8_V8
    tmp = value + 2;
    print(tmp);
    // SUB2_IMM8_V8
    tmp = value - 2;
    print(tmp);
    // DIV2_IMM8_V8
    tmp = value / 2;
    print(tmp);
    // MOD2_IMM8_V8
    tmp = value % 2;
    print(tmp);
    // SHL2_IMM8_V8
    tmp = value << 1;
    print(tmp);
    // SHR2_IMM8_V8
    tmp = value >>> 1;
    print(tmp);
    // AND2_IMM8_V8
    tmp = value & 2;
    print(tmp);
    // OR2_IMM8_V8
    tmp = value | 2;
    print(tmp);
    // XOR2_IMM8_V8
    tmp = value ^ 2;
    print(tmp);
    // ASHR2_IMM8_V8
    tmp = value >> 1;
    print(tmp);
    // EXP_IMM8_V8
    tmp = value ** 1;
    print(tmp);
    // NEG_IMM8
    tmp = -value;
    print(tmp);
    // NOT_IMM8
    tmp = ~value;
    print(tmp);
    
}

Test(1.1);
ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));
Test(2.2);
