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

/*
 * @tc.name:builtinsgc
 * @tc.desc:gc builtins sanity check
 * @tc.type: FUNC
 */
let freesize = ArkTools.GC.getFreeHeapSize();
if (freesize <= 0) {
    print("not positive free heap size");
}

let maxsize = ArkTools.GC.getReservedHeapSize();
if (maxsize <= 0) {
    print("not positive max heap size");
}

let usedsize = ArkTools.GC.getUsedHeapSize();
if (usedsize <= 0) {
    print("not positive used heap size");
}

let myobj = new Object();
let addr = ArkTools.GC.getObjectAddress(myobj);
if (addr <= 0) {
    print("not positive object address");
}

let spacetype = ArkTools.GC.getObjectSpaceType(myobj);
if (spacetype != 9 && spacetype != 12) {
    print("unexpected object space type");
}

try {
    ArkTools.GC.registerNativeAllocation(-1);
    print("negative value exception is expected")
} catch (e) {
}

try {
    ArkTools.GC.registerNativeFree(-1);
    print("negative value exception is expected")
} catch (e) {
}

ArkTools.GC.registerNativeFree(100);
ArkTools.GC.registerNativeAllocation(20000);
ArkTools.GC.waitForFinishGC();
ArkTools.GC.startGC();
