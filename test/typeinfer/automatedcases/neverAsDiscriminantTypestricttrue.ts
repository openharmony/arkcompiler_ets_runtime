/*
* Copyright (c) Microsoft Corporation. All rights reserved.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/neverAsDiscriminantType.ts ===
declare function AssertType(value:any, type:string):void;
type Foo1 = { kind: 'a', a: number } | { kind: 'b' } | { kind: never };

function f1(foo: Foo1) {
    if (foo.kind === 'a') {
AssertType(foo.kind === 'a', "boolean");
AssertType(foo.kind, "union");
AssertType('a', "string");

        foo.a;
AssertType(foo.a, "number");
    }
}

type Foo2 = { kind?: 'a', a: number } | { kind?: 'b' } | { kind?: never };

function f2(foo: Foo2) {
    if (foo.kind === 'a') {
AssertType(foo.kind === 'a', "boolean");
AssertType(foo.kind, "union");
AssertType('a', "string");

        foo.a;
AssertType(foo.a, "number");
    }
}

// Repro from #50716

export interface GatewayPayloadStructure<O extends GatewayOpcode, T extends keyof GatewayEvents, D> {
    op: O
    d: D
    t?: T
    s?: number
}

export type GatewayPayload = {
    [O in GatewayOpcode]: O extends GatewayOpcode.DISPATCH
    ? {
        [T in keyof GatewayEvents]: GatewayPayloadStructure<GatewayOpcode.DISPATCH, T, GatewayEvents[T]>
    }[keyof GatewayEvents]
    : GatewayPayloadStructure<O, never, O extends keyof GatewayParams ? GatewayParams[O] : never>
}[GatewayOpcode]

export interface GatewayParams {
    [GatewayOpcode.HELLO]: { b: 1 }
}

export enum GatewayOpcode {
    DISPATCH = 0,
    HEARTBEAT = 1,
    IDENTIFY = 2,
    PRESENCE_UPDATE = 3,
    VOICE_STATE_UPDATE = 4,
    RESUME = 6,
    RECONNECT = 7,
    REQUEST_GUILD_MEMBERS = 8,
    INVALID_SESSION = 9,
    HELLO = 10,
    HEARTBEAT_ACK = 11,
}

export interface GatewayEvents {
    MESSAGE_CREATE: { a: 1 }
    MESSAGE_UPDATE: { a: 2 }
    MESSAGE_DELETE: { a: 3 }
}

function assertMessage(event: { a: 1 }) { }

export async function adaptSession(input: GatewayPayload) {
    if (input.t === 'MESSAGE_CREATE') {
AssertType(input.t === 'MESSAGE_CREATE', "boolean");
AssertType(input.t, "union");
AssertType('MESSAGE_CREATE', "string");

        assertMessage(input.d)
AssertType(assertMessage(input.d), "void");
AssertType(assertMessage, "({ a: 1; }) => void");
AssertType(input.d, "{ a: 1; }");
    }
}


