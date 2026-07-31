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

declare function print(value: number): void;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

class SharedProto {
    constructor() {
        'use sendable';
    }
}

function getLiveValue() {
    return 100;
}

function buildObjectWithProto(proto: any, values: [number, number]): number {
    let live = getLiveValue();
    let marker = 8;
    try {
        for (let i = 0; i < 2; i++) {
            marker = marker + values[i];
        }
        const obj = { __proto__: proto, value: 33 };
        return live + marker + obj.value;
    } catch (e) {
        return 2000 + live + marker;
    }
}

const REP = 60000;
for (let i = 0; i < REP; i++) {
    buildObjectWithProto({ base: 1 }, [1, 2]);
}

function lazy_deopt_setobjectwithproto_frame_state(): void {
    print(buildObjectWithProto({ base: 1 }, [1, 2]));
    ArkTools.arkSteedCompileSync(buildObjectWithProto);
    print(buildObjectWithProto(new SharedProto(), [1, 2]));
}

lazy_deopt_setobjectwithproto_frame_state();
