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

// === tests/cases/compiler/controlFlowForCatchAndFinally.ts ===
declare function AssertType(value:any, type:string):void;
type Page = {close(): Promise<void>; content(): Promise<string>};
type Browser = {close(): Promise<void>};
declare function test1(): Promise<Browser>;
declare function test2(obj: Browser): Promise<Page>;
async function test(): Promise<string> {
    let browser: Browser | undefined = undefined;
AssertType(browser, "union");
AssertType(undefined, "undefined");

    let page: Page | undefined = undefined;
AssertType(page, "union");
AssertType(undefined, "undefined");

    try {
        browser = await test1();
AssertType(browser = await test1(), "Browser");
AssertType(browser, "union");
AssertType(await test1(), "Browser");
AssertType(test1(), "Promise<Browser>");
AssertType(test1, "() => Promise<Browser>");

        page = await test2(browser);
AssertType(page = await test2(browser), "Page");
AssertType(page, "union");
AssertType(await test2(browser), "Page");
AssertType(test2(browser), "Promise<Page>");
AssertType(test2, "(Browser) => Promise<Page>");
AssertType(browser, "Browser");

AssertType(await page.content(), "string");
AssertType(page.content(), "Promise<string>");
AssertType(page.content, "() => Promise<string>");
        return await page.content();;

    } finally {
        if (page) {
AssertType(page, "union");

            await page.close(); // ok
AssertType(await page.close(), "void");
AssertType(page.close(), "Promise<void>");
AssertType(page.close, "() => Promise<void>");
        }

        if (browser) {
AssertType(browser, "union");

            await browser.close(); // ok
AssertType(await browser.close(), "void");
AssertType(browser.close(), "Promise<void>");
AssertType(browser.close, "() => Promise<void>");
        }
    }
}

declare class Aborter { abort(): void };
class Foo {
    abortController: Aborter | undefined = undefined;

    async operation() {
        if (this.abortController !== undefined) {
AssertType(this.abortController !== undefined, "boolean");
AssertType(this.abortController, "union");
AssertType(this, "this");
AssertType(undefined, "undefined");

            this.abortController.abort();
AssertType(this.abortController.abort(), "void");
AssertType(this.abortController.abort, "() => void");
AssertType(this.abortController, "Aborter");
AssertType(this, "this");

            this.abortController = undefined;
AssertType(this.abortController = undefined, "undefined");
AssertType(this.abortController, "union");
AssertType(this, "this");
AssertType(undefined, "undefined");
        }
        try {
            this.abortController = new Aborter();
AssertType(this.abortController = new Aborter(), "Aborter");
AssertType(this.abortController, "union");
AssertType(this, "this");
AssertType(new Aborter(), "Aborter");
AssertType(Aborter, "typeof Aborter");

        } catch (error) {
AssertType(error, "unknown");

            if (this.abortController !== undefined) {
AssertType(this.abortController !== undefined, "boolean");
AssertType(this.abortController, "union");
AssertType(this, "this");
AssertType(undefined, "undefined");

                this.abortController.abort(); // ok
AssertType(this.abortController.abort(), "void");
AssertType(this.abortController.abort, "() => void");
AssertType(this.abortController, "Aborter");
AssertType(this, "this");
            }
        }
    }
}

