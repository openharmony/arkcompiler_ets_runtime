/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
declare function print(arg:any):void;
let F : number[] = [0, 1, 2]
let G : number[] = [0, 1, 2]

function tryLoopOpt(f: number[], v : number): void {
    let idx = 1 - 1;
    let ret = f[idx];
    for (let i = 0; i < f.length; ++i) {
        f[i] += 1;
        f[i] += v;
    }
    print(ret);
    print(f[idx]);
    print(f[idx + 1]);
}

tryLoopOpt(F, <number><Object>'a');

function tryMergeOpt(g: number[], v : number): void {
    let idx = 1 - 1;
    let ret = g[idx];
    if (g[idx] < 10) {
        g[idx] -= 10;
    } else {
        v++;
    }
    print(ret + v);
    print(g[idx]);
}

tryMergeOpt(G, <number><Object>'b');