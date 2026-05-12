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
function va_args_branch(...args: number[]) {
    if (args.length === 0) {
        print(10);
    } else if (args.length === 1) {
        print(args[0]);
    } else {
        print(args[0] + args[args.length - 1]);
    }
}

ArkTools.arkSteedCompileAsync(va_args_branch);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

va_args_branch();
va_args_branch(7);
va_args_branch(3, 4, 5);
