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

declare var ArkTools:any;
for (let i = 0; i < 10; ++i) {
    Math;
}
ArkTools.printTypedOpProfiler("LOAD_BUILTIN_OBJECT");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.clearTypedOpProfiler();

for (let i = 0; i < 1; ++i) {
    for (let j = 0; j < 10; ++j) {
        Math;
    }
}
ArkTools.printTypedOpProfiler("LOAD_BUILTIN_OBJECT");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.clearTypedOpProfiler();

for (let i = 0; i < 10; ++i) {
    Math.sqrt;
}
ArkTools.printTypedOpProfiler("LOAD_BUILTIN_OBJECT");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.clearTypedOpProfiler();

for (let i = 0; i < 1; ++i) {
    for (let j = 0; j < 10; ++j) {
        Math.sqrt;
    }
}
ArkTools.printTypedOpProfiler("LOAD_BUILTIN_OBJECT");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.clearTypedOpProfiler();

for (let i = 0; i < 10; ++i) {
    Math.sqrt(4);
}
ArkTools.printTypedOpProfiler("LOAD_BUILTIN_OBJECT");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.clearTypedOpProfiler();

for (let i = 0; i < 1; ++i) {
    for (let j = 0; j < 10; ++j) {
        Math.sqrt(4);
    }
}
ArkTools.printTypedOpProfiler("LOAD_BUILTIN_OBJECT");
ArkTools.printTypedOpProfiler("LOAD_PROPERTY");
ArkTools.clearTypedOpProfiler();
