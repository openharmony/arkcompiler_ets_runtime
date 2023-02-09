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

// === tests/cases/conformance/controlFlow/controlFlowBindingElement.ts ===
declare function AssertType(value:any, type:string):void;
{
    const data =  { param: 'value' };
AssertType(data, "{ param: string; }");
AssertType({ param: 'value' }, "{ param: string; }");
AssertType(param, "string");
AssertType('value', "string");

    const {
        param = (() => { throw new Error('param is not defined') })(),
AssertType(param, "string");
AssertType((() => { throw new Error('param is not defined') })(), "never");
AssertType((() => { throw new Error('param is not defined') }), "() => never");
AssertType(() => { throw new Error('param is not defined') }, "() => never");
AssertType(new Error('param is not defined'), "Error");
AssertType(Error, "ErrorConstructor");
AssertType('param is not defined', "string");

    } = data;
AssertType(data, "{ param: string; }");
    
    console.log(param); // should not trigger 'Unreachable code detected.'    
AssertType(console.log(param), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(param, "string");
}


{
    const data =  { param: 'value' };
AssertType(data, "{ param: string; }");
AssertType({ param: 'value' }, "{ param: string; }");
AssertType(param, "string");
AssertType('value', "string");

    let foo: string | undefined = "";
AssertType(foo, "union");
AssertType("", "string");

    const {
        param = (() => { throw new Error('param is not defined') })(),
AssertType(param, "string");
AssertType((() => { throw new Error('param is not defined') })(), "never");
AssertType((() => { throw new Error('param is not defined') }), "() => never");
AssertType(() => { throw new Error('param is not defined') }, "() => never");
AssertType(new Error('param is not defined'), "Error");
AssertType(Error, "ErrorConstructor");
AssertType('param is not defined', "string");

    } = data;
AssertType(data, "{ param: string; }");
    
    foo;  // should be string  
AssertType(foo, "string");
}

{
    const data =  { param: 'value' };
AssertType(data, "{ param: string; }");
AssertType({ param: 'value' }, "{ param: string; }");
AssertType(param, "string");
AssertType('value', "string");

    let foo: string | undefined = "";
AssertType(foo, "union");
AssertType("", "string");

    const {
        param = (() => { foo = undefined })(),
AssertType(param, "union");
AssertType((() => { foo = undefined })(), "void");
AssertType((() => { foo = undefined }), "() => void");
AssertType(() => { foo = undefined }, "() => void");
AssertType(foo = undefined, "undefined");
AssertType(foo, "union");
AssertType(undefined, "undefined");

    } = data;
AssertType(data, "{ param: string; }");
    
    foo;  // should be string | undefined
AssertType(foo, "union");
}

{
    const data =  { param: 'value' };
AssertType(data, "{ param: string; }");
AssertType({ param: 'value' }, "{ param: string; }");
AssertType(param, "string");
AssertType('value', "string");

    let foo: string | undefined = "";
AssertType(foo, "union");
AssertType("", "string");

    const {
        param = (() => { 
AssertType(param, "string");
AssertType((() => { return "" + 1 })(), "string");
AssertType((() => { return "" + 1 }), "() => string");
AssertType(() => { return "" + 1 }, "() => string");
AssertType("" + 1, "string");
AssertType("", "string");
AssertType(1, "int");
return "" + 1 })(),

    } = data;
AssertType(data, "{ param: string; }");
    
    foo;  // should be string
AssertType(foo, "string");
}

{
    interface Window {
        window: Window;
AssertType(window, "Window");
    }

    let foo: string | undefined;
AssertType(foo, "union");

    let window = {} as Window;
AssertType(window, "Window");
AssertType({} as Window, "Window");
AssertType({}, "{}");

    window.window = window;
AssertType(window.window = window, "Window");
AssertType(window.window, "Window");
AssertType(window, "Window");

    const { [(() => { foo = ""; 
AssertType((() => { foo = ""; return 'window' as const })(), "string");
AssertType((() => { foo = ""; return 'window' as const }), "() => "window"");
AssertType(() => { foo = ""; return 'window' as const }, "() => "window"");
AssertType(foo = "", "string");
AssertType(foo, "union");
AssertType("", "string");
AssertType('window' as const, "string");
AssertType('window', "string");
return 'window' as const })()]:

        { [(() => { 
AssertType((() => { return 'window' as const })(), "string");
AssertType((() => { return 'window' as const }), "() => "window"");
AssertType(() => { return 'window' as const }, "() => "window"");
AssertType('window' as const, "string");
AssertType('window', "string");
AssertType(bar, "Window");
AssertType(window, "Window");
return 'window' as const })()]: bar } } = window;

    foo;  // should be string
AssertType(foo, "string");
}

{
    interface Window {
        window: Window;
AssertType(window, "Window");
    }

    let foo: string | undefined;
AssertType(foo, "union");

    let window = {} as Window;
AssertType(window, "Window");
AssertType({} as Window, "Window");
AssertType({}, "{}");

    window.window = window;
AssertType(window.window = window, "Window");
AssertType(window.window, "Window");
AssertType(window, "Window");

    const { [(() => {  
AssertType((() => {  return 'window' as const })(), "string");
AssertType((() => {  return 'window' as const }), "() => "window"");
AssertType(() => {  return 'window' as const }, "() => "window"");
AssertType('window' as const, "string");
AssertType('window', "string");
return 'window' as const })()]:

        { [(() => { foo = ""; 
AssertType((() => { foo = ""; return 'window' as const })(), "string");
AssertType((() => { foo = ""; return 'window' as const }), "() => "window"");
AssertType(() => { foo = ""; return 'window' as const }, "() => "window"");
AssertType(foo = "", "string");
AssertType(foo, "union");
AssertType("", "string");
AssertType('window' as const, "string");
AssertType('window', "string");
AssertType(bar, "Window");
AssertType(window, "Window");
return 'window' as const })()]: bar } } = window;

    foo;  // should be string
AssertType(foo, "string");
}

{
    interface Window {
        window: Window;
AssertType(window, "Window");
    }

    let foo: string | undefined;
AssertType(foo, "union");

    let window = {} as Window;
AssertType(window, "Window");
AssertType({} as Window, "Window");
AssertType({}, "{}");

    window.window = window;
AssertType(window.window = window, "Window");
AssertType(window.window, "Window");
AssertType(window, "Window");

    const { [(() => { 
AssertType((() => { return 'window' as const })(), "string");
AssertType((() => { return 'window' as const }), "() => "window"");
AssertType(() => { return 'window' as const }, "() => "window"");
AssertType('window' as const, "string");
AssertType('window', "string");
return 'window' as const })()]:

        { [(() => { 
AssertType((() => { return 'window' as const })(), "string");
AssertType((() => { return 'window' as const }), "() => "window"");
AssertType(() => { return 'window' as const }, "() => "window"");
AssertType('window' as const, "string");
AssertType('window', "string");
AssertType(bar, "Window");
AssertType((() => { foo = ""; return window; })(), "Window");
AssertType((() => { foo = ""; return window; }), "() => Window");
AssertType(() => { foo = ""; return window; }, "() => Window");
AssertType(foo = "", "string");
AssertType(foo, "union");
AssertType("", "string");
AssertType(window, "Window");
AssertType(window, "Window");
return 'window' as const })()]: bar = (() => { foo = ""; return window; })() } } = window;

    foo;  // should be string | undefined
AssertType(foo, "union");
}


