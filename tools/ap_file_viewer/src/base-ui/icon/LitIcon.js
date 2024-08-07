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
export class LitIcon extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' }).innerHTML = this.initHtml();
        this.initElements();
    }
    static get observedAttributes() {
        return ['name', 'size', 'color', 'path'];
    }
    get name() {
        return this.getAttribute('name') || '';
    }
    set name(value) {
        this.setAttribute('name', value);
    }
    get size() {
        return parseInt(this.getAttribute('size') || '0', 10);
    }
    set size(value) {
        this.setAttribute('size', `${value}`);
    }
    set color(value) {
        this.setAttribute('color', value);
    }
    set path(value) {
        this._path = value;
        this.setAttribute('path', value);
    }
    initHtml() {
        return `
            <style>
                :host{
                    display: inline-block;
                    font-size: inherit;
                 }
                 .icon{
                    width: 1em;
                    height: 1em;
                    display: block;
                    fill: currentColor;
                    overflow: hidden;
                    margin: auto;
                 }
                 @keyframes rotate {
                    to{
                        transform: rotate(360deg);
                    }         
                 }
                 :host([spin]){
                    animation: rotate 1.75s linear infinite;
                 }
            </style>
            <svg class="icon" id="icon" aria-hidden="true" viewBox="0 0 ${this.view || 1024} ${this.view || 1024}">
                 ${this._path ? '<path id="path"></path>' : '<use id="use"></use>'}
            </svg>
            `;
    }
    initElements() {
        if (this.shadowRoot) {
            this.icon = this.shadowRoot.getElementById('icon');
            this.use = this.shadowRoot.querySelector('use');
            this.apPath = this.shadowRoot.querySelector('path');
        }
    }
    attributeChangedCallback(name, oldValue, value) {
        switch (name) {
            case 'name':
                if (this.use) {
                    this.use.href.baseVal = `./base-ui/icon.svg#icon-${value}`;
                }
                break;
            case 'path':
                if (this.apPath) {
                    this.apPath.setAttribute('apPath', value);
                }
                break;
            case 'color':
                if (this.icon) {
                    this.icon.style.color = value;
                }
                break;
            case 'size':
                if (this.icon) {
                    this.icon.style.fontSize = `${value}px`;
                }
                break;
        }
    }
}
if (!customElements.get('lit-icon')) {
    customElements.define('lit-icon', LitIcon);
}
