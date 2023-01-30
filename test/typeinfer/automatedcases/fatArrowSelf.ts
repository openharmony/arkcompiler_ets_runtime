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

// === tests/cases/compiler/fatArrowSelf.ts ===
declare function AssertType(value:any, type:string):void;
module Events {
    export interface ListenerCallback {
        (value:any):void;
    }
    export class EventEmitter {
         public addListener(type:string, listener:ListenerCallback) {
         }
    }
}

module Consumer {
    class EventEmitterConsummer {
        constructor (private emitter: Events.EventEmitter) { }

        private register() {
            this.emitter.addListener('change', (e) => {
AssertType(this.emitter.addListener('change', (e) => {                this.changed();            }), "void");
AssertType(this.emitter.addListener, "(string, Events.ListenerCallback) => void");
AssertType(this.emitter, "Events.EventEmitter");
AssertType(this, "this");
AssertType('change', "string");
AssertType((e) => {                this.changed();            }, "(any) => void");
AssertType(e, "any");

                this.changed();
AssertType(this.changed(), "void");
AssertType(this.changed, "() => void");
AssertType(this, "this");

            });
        }      

        private changed() {
        }
    }
}

