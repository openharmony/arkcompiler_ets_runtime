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
            :host{
                user-select: none;
                display: flex;
                font-family: Helvetica;
                font-size: 14px;
                color: #000;
                text-align: left;
                line-height: 20px;
                font-weight: 400
                background-color: #FFFFFF;
                transition: background-color .3s;
            }
            :host(:not([disabled]):hover){
                display: flex;
                background-color: #6C9BFA;
                color: #FFFFFF;
                cursor: pointer;
            }
            :host([disabled]:hover){
                display: flex;
                cursor:not-allowed;
            }
            :host([disabled]) .root{
                cursor:not-allowed;
                display: flex;
                align-items: center;
                padding: 10px 24px;
                width: 100%;
            }
            :host(:not([disabled])) .root{
                cursor:pointer;
                display: flex;
                align-items: center;
                padding: 10px 24px;
                width: 100%;
            }
            .name{
                padding-left: 10px;
                cursor: pointer;
                overflow-wrap: anywhere;
            }
            .icon{
                pointer-events: none;
            }
            :host(:not([file])) .name{
                pointer-events: none;
            }
            :host(:not([file])) .root{
                pointer-events: none;
            }
            :host([file]) .name{
                pointer-events: none;
            }
            :host([file]) .icon{
                pointer-events: none;
            }
            
            :host([back]) {
                background-color: #6C9BFA;
            } 
            
        </style>
    `;

export class MainMenuItem extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({mode: 'open'}).innerHTML = this.initHtml();
        this.initElements();
    }

    static get observedAttributes() {
        return ['title', 'icon', 'file', 'multi', 'disabled'];
    }

    get title() {
        return this.getAttribute('title') || '';
    }

    set title(val) {
        this.setAttribute('title', val);
    }

    get multi() {
        return this.hasAttribute('multi');
    }

    set multi(val) {
        if (val) {
            this.setAttribute('multi', '');
        } else {
            this.removeAttribute('multi');
        }
    }

    get disabled() {
        return this.hasAttribute('disabled');
    }

    set disabled(val) {
        if (val) {
            this.setAttribute('disabled', val.toString());
            this.fileEL?.setAttribute('disabled', val.toString());
        } else {
            this.removeAttribute('disabled');
            this.fileEL?.removeAttribute('disabled');
        }
    }

    get back() {
        return this.hasAttribute('back');
    }

    set back(isShowBack) {
        if (isShowBack) {
            this.setAttribute('back', '');
        } else {
            this.removeAttribute('back');
        }
    }

    initElements() {
        this.rootEL = this.shadowRoot?.querySelector('.root');
        this.titleEl = this.shadowRoot?.querySelector('.name');
        this.iconEl = this.shadowRoot?.querySelector('.icon');
        this.fileEL = this.shadowRoot?.querySelector('.file');
    }

    isFile() {
        if (this.hasAttribute('file')) {
            if (this.fileEL) {
                return true;
            }
        }
        return false;
    }

    connectedCallback() {
		if (this.hasAttribute('file')) {
			this.setupFileChangeEvents();
			this.setupClickEvents();
		}
    }

	setupFileChangeEvents() {
		if (this.fileEL) {
			this.fileEL.addEventListener('change', event => {
				const files = this.fileEL.files;
				if (files && files.length > 0) {
					this.dispatchEvent(new CustomEvent('file-change', { detail: files[0] }));
					this.resetFileInput();
				}
			});
		}
	}
	
	resetFileInput() {
		if (this.fileEL) {
			this.fileEL.value = '';
		}
	}

	setupClickEvents() {
		this.addEventListener('click', mouseEvent => {
			mouseEvent.stopPropagation();
		});
	}
	
    initHtml() {
        return `
        ${initHtmlStyle}
        <input id="file" class="file" type="file" style="display:none;pointer-events: none" />
        <label class="root" for="file">
            <lit-icon class="icon" name="user" size="20"></lit-icon>
            <label class="name"></label>
        </label>
        `;
    }

    attributeChangedCallback(name, oldValue, newValue) {
        switch (name) {
            case 'title':
                if (this.titleEl) {
                    this.titleEl.textContent = newValue;
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

if (!customElements.get('lit-main-menu-item')) {
    customElements.define('lit-main-menu-item', MainMenuItem);
}
