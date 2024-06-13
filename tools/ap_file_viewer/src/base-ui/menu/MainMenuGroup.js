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
const initHtmlStyle = `
    <style>
        :host(:not([collapsed])){ 
            width: 248px;
            display: flex;
            background-color: var(--dark-background);
            cursor: pointer;
            flex-direction: column;
        }
        :host{
            user-select: none;
            transition: background-color .3s;
        }
        :host(:not([collapsed])),:host(:not([second])) ::slotted(lit-main-menu-item){
            display: flex;
        }
        host(:not([collapsed])) :host([second]) ::slotted(lit-main-menu-group){
          display:flex;
        }
        :host([second]) .group-name{
          padding-left:40px;
        }
        :host(:not([collapsed])) .group-describe{
            height: 0;
            visibility: hidden;
            padding:0;
        }
        :host([collapsed]):hover){
            background-color: #FFFFFF;
        }
        :host([collapsed]){
            width: 248px;
            display: flex;
            flex-direction: column;
            cursor: pointer;
            background-color: var(--dark-background);
        }
        :host([collapsed]) .group-describe{
            height: auto;
            visibility: visible;
        }
        :host([radius]) {
            border-radius: 16px 0px 0px 16px ;
        }
        :host([collapsed]) ::slotted(lit-main-menu-item){
            display: none;
        }
        :host([collapsed]) ::slotted(lit-main-menu-group){
          display:none;
        }
        :host(:not([describe])) .group-describe{
          display:none;
        }
        :host([describe]) .group-describe{
          padding: 4px 24px 0 24px;
          color: #999 !important;
          font-size: 12px;
        }
        :host([describe]) .group-name{
          margin-top: 10px;
        }
        .group-name{
            display:flex;
            font-size: 14px;
            font-family: Helvetica;
            color: #000;
            padding: 15px 24px 5px 10px;
            line-height: 16px;
            font-weight: 400;
            text-align: left;
        }
        :host([collapsed]) .icon{
          transform: rotateZ(-90deg);
        }
        </style>
    `;
export class MainMenuGroup extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' }).innerHTML = this.initHtml();
        this.initElements();
    }
    static get observedAttributes() {
        return ['title', 'describe', 'collapsed', 'nocollapse', 'radius', 'second', 'icon'];
    }
    get second() {
        return this.hasAttribute('second');
    }
    set second(value) {
        if (value) {
            this.setAttribute('second', '');
        }
        else {
            this.removeAttribute('second');
        }
    }
    get collapsed() {
        return this.hasAttribute('collapsed');
    }
    set collapsed(value) {
        if (value) {
            this.setAttribute('collapsed', '');
        }
        else {
            this.removeAttribute('collapsed');
        }
    }
    get nocollapsed() {
        return this.hasAttribute('nocollapsed');
    }
    set nocollapsed(value) {
        if (value) {
            this.setAttribute('nocollapsed', '');
        }
        else {
            this.removeAttribute('nocollapsed');
        }
    }
    get radius() {
        return this.hasAttribute('radius');
    }
    initElements() {
        this.groupNameEl = this.shadowRoot?.querySelector('.group-title');
        this.groupDescEl = this.shadowRoot?.querySelector('.group-describe');
        this.iconEl = this.shadowRoot?.querySelector('.icon');
        this.group = this.shadowRoot?.querySelector('#group');
        this.group.addEventListener('click', (event) => {
            if (this.nocollapsed) {
                return;
            }
            this.collapsed = !this.collapsed;
        });
    }
    initHtml() {
        return `
        ${initHtmlStyle}
        <div id="group">
        <div class="group-name">
          <lit-icon class="icon" name="user" size="20"></lit-icon>
          <span class="group-title"></span>
        </div>
        <div class="group-describe"></div>
      </div>
        <slot></slot>
        `;
    }
    attributeChangedCallback(name, oldValue, newValue) {
        switch (name) {
            case 'title':
                if (this.groupNameEl) {
                    this.groupNameEl.textContent = newValue;
                }
                break;
            case 'describe':
                if (this.groupDescEl) {
                    this.groupDescEl.textContent = newValue;
                }
                break;
            case 'icon':
                if (this.iconEl) {
                    this.iconEl.setAttribute('name', newValue);
                }
                break;
        }
    }
}
if (!customElements.get('lit-main-menu-group')) {
    customElements.define('lit-main-menu-group', MainMenuGroup);
}
