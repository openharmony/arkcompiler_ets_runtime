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

// === tests/cases/compiler/listFailure.ts ===
declare function AssertType(value:any, type:string):void;
module Editor {

    export class Buffer {
    	lines: List<Line> = ListMakeHead<Line>();
        
        addLine(lineText: string): List<Line> {
            
            let line: Line = new Line();
AssertType(line, "Line");
AssertType(new Line(), "Line");
AssertType(Line, "typeof Line");

            let lineEntry = this.lines.add(line);
AssertType(lineEntry, "List<Line>");
AssertType(this.lines.add(line), "List<Line>");
AssertType(this.lines.add, "(Line) => List<Line>");
AssertType(this.lines, "List<Line>");
AssertType(this, "this");
AssertType(line, "Line");

AssertType(lineEntry, "List<Line>");
            return lineEntry;
        }    
    }
    
    export function ListRemoveEntry<U>(entry: List<U>): List<U> { 
AssertType(entry, "List<U>");
    	return entry;
    }

    export function ListMakeHead<U>(): List<U> {
AssertType(null, "null");
		return null;
    }

    export function ListMakeEntry<U>(data: U): List<U> {
AssertType(null, "null");
		return null;
    }    

    class List<T> { 
        public next: List<T>; 

        add(data: T): List<T> {
            this.next = ListMakeEntry(data);
AssertType(this.next = ListMakeEntry(data), "List<T>");
AssertType(this.next, "List<T>");
AssertType(this, "this");
AssertType(ListMakeEntry(data), "List<T>");
AssertType(ListMakeEntry, "<U>(U) => List<U>");
AssertType(data, "T");

AssertType(this.next, "List<T>");
AssertType(this, "this");
            return this.next;
        }

        popEntry(head: List<T>): List<T> {
AssertType((ListRemoveEntry(this.next)), "List<T>");
AssertType(ListRemoveEntry(this.next), "List<T>");
AssertType(ListRemoveEntry, "<U>(List<U>) => List<U>");
AssertType(this.next, "List<T>");
AssertType(this, "this");
            return (ListRemoveEntry(this.next));
        }      
    }

	export class Line {}
}

