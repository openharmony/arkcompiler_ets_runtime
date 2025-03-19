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

function Choose(condition) {
    // EQ_IMM8_V8
    if (condition == 1) {
        return 1;
    }

    // GREATEREQ_IMM8_V8
    // LESSEQ_IMM8_V8
    if (condition >= 2 && condition <= 5) {
        return 2;
    }

    // GREATER_IMM8_V8
    // LESS_IMM8_V8
   if (condition > 5 && condition <9) {
        return 3;
    }

    // STRICTEQ_IMM8_V8
    if (condition === 9) {
        return 4;
    }

    // STRICTNOTEQ_IMM8_V8
    // NOTEQ_IMM8_V8
    if (condition != 9 && condition != 10) {
        return 5;
    }
}

function Test() {
    // INC_IMM8
    for (var i = 0; i < 6; ++i) {
        print(Choose(i));
    }
    // DEC_IMM8
    for (var i = 12; i > 5; --i) {
        print(Choose(i));
    }
}

Test()

ArkTools.jitCompileAsync(Choose)
print(ArkTools.waitJitCompileFinish(Choose));
ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test()
