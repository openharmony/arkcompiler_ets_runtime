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
declare function print(...args: any[]): void;

function loop_while_many_variables_1(n: number, P: number, Q: number): number {
    let x0 = 1;
    let x1 = 2;
    let x2 = 3;
    let x3 = 4;
    let x4 = 5;
    let x5 = 6;
    let x6 = 7;
    let x7 = 8;
    let x8 = 9;
    let x9 = 10;
    let x10 = 11;
    let x11 = 12;
    let x12 = 13;
    let x13 = 14;
    let x14 = 15;
    let x15 = 16;
    let x16 = 17;
    let x17 = 18;
    let x18 = 19;
    let x19 = 20;
    let x20 = 21;
    let x21 = 22;
    let x22 = 23;
    let x23 = 24;
    let x24 = 25;
    let x25 = 26;
    let x26 = 27;
    let x27 = 28;
    let x28 = 29;
    let x29 = 30;
    let x30 = 31;
    let x31 = 32;
    let x32 = 33;
    let x33 = 34;
    let x34 = 35;
    let x35 = 36;
    let x36 = 37;
    let x37 = 38;
    let x38 = 39;
    let x39 = 40;
    let x40 = 41;
    let x41 = 42;
    let x42 = 43;
    let x43 = 44;
    let x44 = 45;
    let x45 = 46;
    let x46 = 47;
    let x47 = 48;
    let x48 = 49;
    let x49 = 50;
    let x50 = 51;
    let x51 = 52;
    let x52 = 53;
    let x53 = 54;
    let x54 = 55;
    let x55 = 56;
    let x56 = 57;
    let i = 0;
    while (i < n) {
        x0 = (x0 + x1) ^ P;
        x1 = (x0 ^ x2) & Q;
        x2 = (x2 + x3) ^ P;
        x3 = (x2 ^ x4) & Q;
        x4 = (x4 + x5) ^ P;
        x5 = (x4 ^ x6) & Q;
        x6 = (x6 + x7) ^ P;
        x7 = (x6 ^ x8) & Q;
        x8 = (x8 + x9) ^ P;
        x9 = (x8 ^ x10) & Q;
        x10 = (x10 + x11) ^ P;
        x11 = (x10 ^ x12) & Q;
        x12 = (x12 + x13) ^ P;
        x13 = (x12 ^ x14) & Q;
        x14 = (x14 + x15) ^ P;
        x15 = (x14 ^ x16) & Q;
        x16 = (x16 + x17) ^ P;
        x17 = (x16 ^ x18) & Q;
        x18 = (x18 + x19) ^ P;
        x19 = (x18 ^ x20) & Q;
        x20 = (x20 + x21) ^ P;
        x21 = (x20 ^ x22) & Q;
        x22 = (x22 + x23) ^ P;
        x23 = (x22 ^ x24) & Q;
        x24 = (x24 + x25) ^ P;
        x25 = (x24 ^ x26) & Q;
        x26 = (x26 + x27) ^ P;
        x27 = (x26 ^ x28) & Q;
        x28 = (x28 + x29) ^ P;
        x29 = (x28 ^ x30) & Q;
        x30 = (x30 + x31) ^ P;
        x31 = (x30 ^ x32) & Q;
        x32 = (x32 + x33) ^ P;
        x33 = (x32 ^ x34) & Q;
        x34 = (x34 + x35) ^ P;
        x35 = (x34 ^ x36) & Q;
        x36 = (x36 + x37) ^ P;
        x37 = (x36 ^ x38) & Q;
        x38 = (x38 + x39) ^ P;
        x39 = (x38 ^ x40) & Q;
        x40 = (x40 + x41) ^ P;
        x41 = (x40 ^ x42) & Q;
        x42 = (x42 + x43) ^ P;
        x43 = (x42 ^ x44) & Q;
        x44 = (x44 + x45) ^ P;
        x45 = (x44 ^ x46) & Q;
        x46 = (x46 + x47) ^ P;
        x47 = (x46 ^ x48) & Q;
        x48 = (x48 + x49) ^ P;
        x49 = (x48 ^ x50) & Q;
        x50 = (x50 + x51) ^ P;
        x51 = (x50 ^ x52) & Q;
        x52 = (x52 + x53) ^ P;
        x53 = (x52 ^ x54) & Q;
        x54 = (x54 + x55) ^ P;
        x55 = (x54 ^ x56) & Q;
        i++;
    }
    return x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7 + i;
}

ArkTools.arkSteedCompileSync(loop_while_many_variables_1);
print(loop_while_many_variables_1(56, 0b1010_1010, 0b1111_1111));
