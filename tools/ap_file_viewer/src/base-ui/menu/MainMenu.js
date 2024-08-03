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
import './MainMenuItem.js';
import './MainMenuGroup.js';
import { MainMenuGroup } from './MainMenuGroup.js';
import { MainMenuItem } from './MainMenuItem.js';
const initHtmlStyle = `
    <style>
        :host{
            width: 248px;
            height: 100vh;
            display: flex;
            flex-direction: column;
            background-color: #fff;
        }
        .menu-body ::-webkit-scrollbar-thumb
        {
            background-color: var(--dark-background,#FFFFFF);
            border-radius:10px;

        }
        .menu-body ::-webkit-scrollbar-track
        {
            border-radius:10px;
            background-color:#F5F5F5;
            
        }
        .header{
            display: grid;
            width: 100%;
            height: 56px;
            font-size: 1.4rem;
            padding-left: 20px;
            gap: 0 20px;
            box-sizing: border-box;
            grid-template-columns: min-content 1fr min-content;
            grid-template-rows: auto;
            color: #47A7E0;
            background-color: var(--dark-background1);
            border-bottom: 1px solid var(--dark-background1,#EFEFEF);
        }
        .bottom{
            width: 100%;
            display: flex;
            justify-content: space-between;
        }
        .header *{
            user-select: none;
            align-self: center;
        }
        *{
            box-sizing: border-box;
        }
        </style>
    `;
export class MainMenu extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' }).innerHTML = this.initHtml();
        this.initElements();
    }
    get menus() {
        return this._menus;
    }
    set menus(value) {
        this._menus = value;
        this.shadowRoot?.querySelectorAll('lit-main-menu-group').forEach((menuItem) => menuItem.remove());
        let menuBody = this.shadowRoot?.querySelector('.menu-body');
        if (this.getAttribute('main_menu') === '1' && window.localStorage.getItem('Theme') === 'dark') {
            this.style.backgroundColor = '#262f3c';
        }
        else {
            this.style.backgroundColor = '#fff';
        }
        value === null || value === void 0 ? void 0 : value.forEach((it) => {
            let group = new MainMenuGroup();
            group.setAttribute('title', it.title || '');
            if (it.describe !== '') {
                group.setAttribute('describe', it.describe || '');
            }
            else {
                group.removeAttribute('describe');
            }
            group.setAttribute('icon', it.icon || '');
            if (it.collapsed) {
                group.setAttribute('collapsed', '');
            }
            else {
                group.removeAttribute('collapsed');
            }
            let groupName = group.shadowRoot.querySelector('.group-name');
            let groupDescribe = group.shadowRoot.querySelector('.group-describe');
            menuBody === null || menuBody === void 0 ? void 0 : menuBody.appendChild(group);
            it.children?.forEach((item) => {
                if (item.fileModel !== undefined && item.fileModel === 'db') {
                    return;
                }
                this.notChildren(item, group, groupName, groupDescribe);
            });
        });
    }
    notChildren(item, group, groupName, groupDescribe) {
        let th = new MainMenuItem();
        th.setAttribute('icon', item.icon || '');
        th.setAttribute('title', item.title || '');
        if (this.getAttribute('main_menu') === '1' && window.localStorage.getItem('Theme') === 'dark') {
            groupName.style.color = 'white';
            groupDescribe.style.color = 'white';
            th.style.color = 'white';
        }
        else {
            groupName.style.color = 'black';
            groupDescribe.style.color = 'black';
            th.style.color = 'black';
        }
        if (item.fileChoose) {
            th.setAttribute('file', '');
            th.addEventListener('file-change', (event) => {
                if (item.fileHandler && !th.disabled) {
                    item.fileHandler(event);
                }
            });
        }
        else {
            th.removeAttribute('file');
            th.addEventListener('click', (event) => {
                if (item.clickHandler && !th.disabled) {
                    item.clickHandler(item);
                }
            });
        }
        if (item.multi) {
            th.multi = true;
        }
        if (item.disabled !== undefined) {
            th.disabled = item.disabled;
        }
        group === null || group === void 0 ? void 0 : group.appendChild(th);
    }
    initElements() {
    }
    initHtml() {
        return `
        ${initHtmlStyle}
        <div class="header" name="header">
            <img src="img/ap_logo.png"/>
            <div class="menu-button">
                    <lit-icon name="menu" size="20" color="var(blue,#4D4D4D)"></lit-icon>
              </div>
            </div>
            <div class="menu-body" style="overflow: auto;overflow-x:hidden;height: 100%">
                <slot id="st" ></slot>
            </div>
      `;
    }
}
if (!customElements.get('lit-main-menu')) {
    customElements.define('lit-main-menu', MainMenu);
}
