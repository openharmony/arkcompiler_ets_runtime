/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
export class ApWelcome extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' }).innerHTML = this.initHtml();
    }
    initHtml() {
        return `
        <style>
        :host {
            width: 100%;
            height: 100%;
            display: grid;
            justify-content: center;
            align-content: center;
            background: #F6F6F6;
        }
        .lit-icon{
            content: url('img/pic.png');
        }
        </style>
        <div>
            <img class="lit-icon" >
        </div>
        `;
    }
}
if (!customElements.get('ap-welcome')) {
    customElements.define('ap-welcome', ApWelcome);
}
