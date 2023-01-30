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

// === tests/cases/compiler/genericClassWithStaticFactory.ts ===
declare function AssertType(value:any, type:string):void;
module Editor {

    export class List<T> {
        public next: List<T>;
        public prev: List<T>;
        private listFactory: ListFactory<T>;

        constructor(public isHead: boolean, public data: T) {
            this.listFactory = new ListFactory<T>();
AssertType(this.listFactory = new ListFactory<T>(), "ListFactory<T>");
AssertType(this.listFactory, "ListFactory<T>");
AssertType(this, "this");
AssertType(new ListFactory<T>(), "ListFactory<T>");
AssertType(ListFactory, "typeof ListFactory");
           
        }

        public add(data: T): List<T> {
            let entry = this.listFactory.MakeEntry(data);
AssertType(entry, "List<T>");
AssertType(this.listFactory.MakeEntry(data), "List<T>");
AssertType(this.listFactory.MakeEntry, "<T>(T) => List<T>");
AssertType(this.listFactory, "ListFactory<T>");
AssertType(this, "this");
AssertType(data, "T");
            
            this.prev.next = entry;
AssertType(this.prev.next = entry, "List<T>");
AssertType(this.prev.next, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

            entry.next = this;
AssertType(entry.next = this, "this");
AssertType(entry.next, "List<T>");
AssertType(this, "this");

            entry.prev = this.prev;
AssertType(entry.prev = this.prev, "List<T>");
AssertType(entry.prev, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");

            this.prev = entry;
AssertType(this.prev = entry, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

AssertType(entry, "List<T>");
            return entry;
        }

        public count(): number {
            let entry: List<T>;
AssertType(entry, "List<T>");

            let i: number;
AssertType(i, "number");

            entry = this.next;
AssertType(entry = this.next, "List<T>");
AssertType(entry, "List<T>");
AssertType(this.next, "List<T>");
AssertType(this, "this");

            for (i = 0; !(entry.isHead); i++) {
AssertType(i = 0, "int");
AssertType(i, "number");
AssertType(0, "int");
AssertType(!(entry.isHead), "boolean");
AssertType((entry.isHead), "boolean");
AssertType(entry.isHead, "boolean");
AssertType(i++, "number");
AssertType(i, "number");

                entry = entry.next;
AssertType(entry = entry.next, "List<T>");
AssertType(entry, "List<T>");
AssertType(entry.next, "List<T>");
            }

AssertType((i), "number");
AssertType(i, "number");
            return (i);
        }

        public isEmpty(): boolean {
AssertType((this.next == this), "boolean");
AssertType(this.next == this, "boolean");
AssertType(this.next, "List<T>");
AssertType(this, "this");
AssertType(this, "this");
            return (this.next == this);
        }

        public first(): T {
            if (this.isEmpty())
AssertType(this.isEmpty(), "boolean");
AssertType(this.isEmpty, "() => boolean");
AssertType(this, "this");
            {
AssertType(this.next.data, "T");
AssertType(this.next, "List<T>");
AssertType(this, "this");
                return this.next.data;
            }
            else {
AssertType(null, "null");
                return null;
            }
        }

        public pushEntry(entry: List<T>): void {
            entry.isHead = false;
AssertType(entry.isHead = false, "boolean");
AssertType(entry.isHead, "boolean");
AssertType(false, "boolean");

            entry.next = this.next;
AssertType(entry.next = this.next, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(this.next, "List<T>");
AssertType(this, "this");

            entry.prev = this;
AssertType(entry.prev = this, "this");
AssertType(entry.prev, "List<T>");
AssertType(this, "this");

            this.next = entry;
AssertType(this.next = entry, "List<T>");
AssertType(this.next, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

            entry.next.prev = entry; // entry.next.prev does not show intellisense, but entry.prev.prev does
AssertType(entry.next.prev = entry, "List<T>");
AssertType(entry.next.prev, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(entry, "List<T>");
        }

        public push(data: T): void {
            let entry = this.listFactory.MakeEntry(data);
AssertType(entry, "List<T>");
AssertType(this.listFactory.MakeEntry(data), "List<T>");
AssertType(this.listFactory.MakeEntry, "<T>(T) => List<T>");
AssertType(this.listFactory, "ListFactory<T>");
AssertType(this, "this");
AssertType(data, "T");

            entry.data = data;
AssertType(entry.data = data, "T");
AssertType(entry.data, "T");
AssertType(data, "T");

            entry.isHead = false;
AssertType(entry.isHead = false, "boolean");
AssertType(entry.isHead, "boolean");
AssertType(false, "boolean");

            entry.next = this.next;
AssertType(entry.next = this.next, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(this.next, "List<T>");
AssertType(this, "this");

            entry.prev = this;
AssertType(entry.prev = this, "this");
AssertType(entry.prev, "List<T>");
AssertType(this, "this");

            this.next = entry;
AssertType(this.next = entry, "List<T>");
AssertType(this.next, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

            entry.next.prev = entry;  // entry.next.prev does not show intellisense, but entry.prev.prev does
AssertType(entry.next.prev = entry, "List<T>");
AssertType(entry.next.prev, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(entry, "List<T>");
        }

        public popEntry(head: List<T>): List<T> {
            if (this.next.isHead) {
AssertType(this.next.isHead, "boolean");
AssertType(this.next, "List<T>");
AssertType(this, "this");

AssertType(null, "null");
                return null;
            }
            else {
AssertType(this.listFactory.RemoveEntry(this.next), "List<T>");
AssertType(this.listFactory.RemoveEntry, "<T>(List<T>) => List<T>");
AssertType(this.listFactory, "ListFactory<T>");
AssertType(this, "this");
AssertType(this.next, "List<T>");
AssertType(this, "this");
                return this.listFactory.RemoveEntry(this.next);
            }
        }

        public insertEntry(entry: List<T>): List<T> {
            entry.isHead = false;
AssertType(entry.isHead = false, "boolean");
AssertType(entry.isHead, "boolean");
AssertType(false, "boolean");

            this.prev.next = entry;
AssertType(this.prev.next = entry, "List<T>");
AssertType(this.prev.next, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

            entry.next = this;
AssertType(entry.next = this, "this");
AssertType(entry.next, "List<T>");
AssertType(this, "this");

            entry.prev = this.prev;
AssertType(entry.prev = this.prev, "List<T>");
AssertType(entry.prev, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");

            this.prev = entry;
AssertType(this.prev = entry, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

AssertType(entry, "List<T>");
            return entry;
        }

        public insertAfter(data: T): List<T> {
            let entry: List<T> = this.listFactory.MakeEntry(data);
AssertType(entry, "List<T>");
AssertType(this.listFactory.MakeEntry(data), "List<T>");
AssertType(this.listFactory.MakeEntry, "<T>(T) => List<T>");
AssertType(this.listFactory, "ListFactory<T>");
AssertType(this, "this");
AssertType(data, "T");

            entry.next = this.next;
AssertType(entry.next = this.next, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(this.next, "List<T>");
AssertType(this, "this");

            entry.prev = this;
AssertType(entry.prev = this, "this");
AssertType(entry.prev, "List<T>");
AssertType(this, "this");

            this.next = entry;
AssertType(this.next = entry, "List<T>");
AssertType(this.next, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

            entry.next.prev = entry;// entry.next.prev does not show intellisense, but entry.prev.prev does
AssertType(entry.next.prev = entry, "List<T>");
AssertType(entry.next.prev, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(entry, "List<T>");

AssertType(entry, "List<T>");
            return entry;
        }

        public insertEntryBefore(entry: List<T>): List<T> {
            this.prev.next = entry;
AssertType(this.prev.next = entry, "List<T>");
AssertType(this.prev.next, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

            entry.next = this;
AssertType(entry.next = this, "this");
AssertType(entry.next, "List<T>");
AssertType(this, "this");

            entry.prev = this.prev;
AssertType(entry.prev = this.prev, "List<T>");
AssertType(entry.prev, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");

            this.prev = entry;
AssertType(this.prev = entry, "List<T>");
AssertType(this.prev, "List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");

AssertType(entry, "List<T>");
            return entry;
        }

        public insertBefore(data: T): List<T> {
            let entry = this.listFactory.MakeEntry(data);
AssertType(entry, "List<T>");
AssertType(this.listFactory.MakeEntry(data), "List<T>");
AssertType(this.listFactory.MakeEntry, "<T>(T) => List<T>");
AssertType(this.listFactory, "ListFactory<T>");
AssertType(this, "this");
AssertType(data, "T");

AssertType(this.insertEntryBefore(entry), "List<T>");
AssertType(this.insertEntryBefore, "(List<T>) => List<T>");
AssertType(this, "this");
AssertType(entry, "List<T>");
            return this.insertEntryBefore(entry);
        }
    }

    export class ListFactory<T> {

        public MakeHead<T>(): List<T> {
            let entry: List<T> = new List<T>(true, null);
AssertType(entry, "List<T>");
AssertType(new List<T>(true, null), "List<T>");
AssertType(List, "typeof List");
AssertType(true, "boolean");
AssertType(null, "null");

            entry.prev = entry;
AssertType(entry.prev = entry, "List<T>");
AssertType(entry.prev, "List<T>");
AssertType(entry, "List<T>");

            entry.next = entry;
AssertType(entry.next = entry, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(entry, "List<T>");

AssertType(entry, "List<T>");
            return entry;
        }

        public MakeEntry<T>(data: T): List<T> {
            let entry: List<T> = new List<T>(false, data);
AssertType(entry, "List<T>");
AssertType(new List<T>(false, data), "List<T>");
AssertType(List, "typeof List");
AssertType(false, "boolean");
AssertType(data, "T");

            entry.prev = entry;
AssertType(entry.prev = entry, "List<T>");
AssertType(entry.prev, "List<T>");
AssertType(entry, "List<T>");

            entry.next = entry;
AssertType(entry.next = entry, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(entry, "List<T>");

AssertType(entry, "List<T>");
            return entry;
        }

        public RemoveEntry<T>(entry: List<T>): List<T> {
            if (entry == null) {
AssertType(entry == null, "boolean");
AssertType(entry, "List<T>");
AssertType(null, "null");

AssertType(null, "null");
                return null;
            }
            else if (entry.isHead) {
AssertType(entry.isHead, "boolean");

            // Can't remove the head of a list!
AssertType(null, "null");
                return null;
            }
            else {
                entry.next.prev = entry.prev;
AssertType(entry.next.prev = entry.prev, "List<T>");
AssertType(entry.next.prev, "List<T>");
AssertType(entry.next, "List<T>");
AssertType(entry.prev, "List<T>");

                entry.prev.next = entry.next;
AssertType(entry.prev.next = entry.next, "List<T>");
AssertType(entry.prev.next, "List<T>");
AssertType(entry.prev, "List<T>");
AssertType(entry.next, "List<T>");

AssertType(entry, "List<T>");
                return entry;
            }
        }
    }
}

