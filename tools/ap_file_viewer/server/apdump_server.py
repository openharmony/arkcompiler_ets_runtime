#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2024 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import cgi
import ctypes
import http
import json
import os
import re
import signal
import sys
import threading
import time
import webbrowser
from datetime import datetime
from http.server import ThreadingHTTPServer, SimpleHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse

VERSION = 'v1.0.0'
LIB_NAME = 'libprofDumpJson'
current = os.path.dirname(os.path.abspath(__file__))
libs_directory = current
dependencies = ['hmicuuc', 'hmicui18n']
keep_running = True
httpd = None


def is_windows():
    return os.name == 'nt'


def is_linux():
    return os.name == 'posix'


def load_dependencies(load_dependencies_lib, lib_directory):
    for dep in load_dependencies_lib:
        dep_path = f"{lib_directory}/lib{dep}.so"
        try:
            ctypes.CDLL(dep_path)
        except OSError as e:
            raise OSError(f"Failed to load dependency {dep_path}: {e}")


def load_library_os_compatible(load_lib_name, lib_directory=None, lib_dependencies=None):
    if lib_directory is None:
        lib_directory = Path.cwd()

    if is_windows():
        lib_extension = '.dll'
        load_lib_path = f"{lib_directory}/{load_lib_name}{lib_extension}"
        load_lib = ctypes.CDLL(str(load_lib_path))
    else:
        lib_extension = '.so'
        load_lib_path = f"{lib_directory}/{load_lib_name}{lib_extension}"
        if lib_dependencies:
            load_dependencies(lib_dependencies, lib_directory)
        try:
            load_lib = ctypes.CDLL(str(load_lib_path))
        except OSError as e:
            raise OSError(f"Failed to load {load_lib_path}: {e}")

    if load_lib is None:
        raise ImportError(f"Could not load library {load_lib_name}.")

    return load_lib


lib = load_library_os_compatible(LIB_NAME, libs_directory, dependencies)
ConvertApToJson = lib.ConvertApToJson
ConvertApToJson.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
ConvertApToJson.restype = ctypes.c_size_t
GetConvertResult = lib.GetConvertResult
GetConvertResult.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
GetConvertResult.restype = ctypes.c_bool


def get_content_type(file_extension):
    if file_extension == '.js':
        return 'application/javascript'
    if file_extension == '.wasm':
        return 'application/wasm'
    if file_extension == '.json':
        return 'application/json'
    if file_extension == '.html':
        return 'text/html'
    if file_extension == '.svg':
        return 'image/svg+xml'
    return 'text/plain'


def parse_ap_file(file_path):
    encoded_path = file_path.encode('utf-8')
    path_length = len(encoded_path)
    written_size = ConvertApToJson(encoded_path, path_length)
    if written_size > 0:
        buffer = ctypes.create_string_buffer(int(written_size))
        GetConvertResult(buffer, written_size)
        json_result = buffer[:written_size].decode('utf-8')
        return json_result
    else:
        return None


def is_subpath(parent_path, child_path):
    try:
        relative_path = os.path.relpath(child_path, parent_path)
        return relative_path != os.pardir
    except ValueError:
        return False


def open_web(url):
    webbrowser.open(url)


class SafeFileHandler:
    def __init__(self):
        self.lock = threading.Lock()

    def parse_ap_file_safely(self, file_path):
        with self.lock:
            return parse_ap_file(file_path)


safe_handler = SafeFileHandler()


class ThreadedHTTPServer(ThreadingHTTPServer):
    daemon_threads = True


class ApRequestHandler(SimpleHTTPRequestHandler):
    global VERSION

    def log_message(self, formate, *arg):
        return

    def simple_secure_filename(self, filename):
        ascii_filename = filename.encode('ascii', errors='ignore').decode('ascii')
        safe_chars = re.compile(r'[^\w\.\- ]')
        safe_filename = safe_chars.sub('_', ascii_filename)
        safe_filename = safe_filename.lstrip('.')
        if not safe_filename:
            return "index.html"
        return safe_filename

    def do_GET(self):
        parse_result = urlparse(self.path)
        if parse_result.path.startswith('/ap'):
            self.application_handler(parse_result)
        else:
            self.send_error(http.HTTPStatus.NOT_FOUND, 'Not found')

    def upload_handler(self):
        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={
                'REQUEST_METHOD': 'POST',
                'CONTENT_TYPE': self.headers['Content-Type'],
            }
        )
        if 'file' in form:
            file_item = form['file']
            filename = self.simple_secure_filename(file_item.filename)
            save_path = os.path.join(current, 'uploads', datetime.now().strftime('%Y%m%d%H%M%S%f'))
            file_path = os.path.join(save_path, filename)
            if not os.path.exists(save_path):
                os.makedirs(save_path, mode=0o755)
            fd = os.open(file_path, os.O_WRONLY | os.O_CREAT, 0o644)
            with os.fdopen(fd, 'wb') as f:
                f.write(file_item.file.read())
            ap_res = safe_handler.parse_ap_file_safely(file_path)
            os.remove(file_path)
            dir_path = os.path.dirname(file_path)
            if not os.listdir(dir_path):
                os.rmdir(dir_path)
            self.send_response(http.HTTPStatus.OK)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            if ap_res is None:
                response = {"success": False, "code": -1, "message": "parse ap failed", "data": ""}
            else:
                response = {"success": True, "code": 0, "message": "success", "data": ap_res}
            self.wfile.write(bytes(json.dumps(response), "utf-8"))
        else:
            self.send_error(http.HTTPStatus.BAD_REQUEST, 'Bad request')

    def do_POST(self):
        parse_result = urlparse(self.path)
        if parse_result.path.startswith('/ap/upload'):
            self.upload_handler()
        else:
            self.send_error(http.HTTPStatus.NOT_FOUND, 'Not found')

    def application_handler(self, parse_result):
        file_path = parse_result.path[3:]
        file_extension = os.path.splitext(file_path)[1]
        safe_path = os.path.normpath(file_path).lstrip('/')
        if is_windows():
            safe_path = os.path.normpath(file_path).lstrip("\\")
        full_path = os.path.join(current, safe_path)
        if file_path == '' or file_path == '/' or file_path is None or safe_path.strip() == ".":
            full_path = os.path.join(current, "index.html")
            file_extension = '.html'
        elif not is_subpath(current, full_path):
            self.send_error(http.HTTPStatus.NOT_FOUND, 'Not found')
            return
        try:
            with open(full_path, 'rb') as file:
                content = file.read()
            self.send_response(http.HTTPStatus.OK)
            self.send_header('Content-type', get_content_type(file_extension))
            self.send_header("Cross-Origin-Opener-Policy", "unsafe-none")
            self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Access-Control-Allow-Credentials", "true")
            self.send_header("Access-Control-Allow-Headers", "x-requested-with, authorization, blade-auth")
            self.send_header("Access-Control-Allow-Methods", "*")
            self.send_header("Access-Control-Max-Age", "3600")
            self.send_header("data-version", VERSION)
            self.end_headers()
            self.wfile.write(content)
        except FileNotFoundError:
            self.send_error(http.HTTPStatus.NOT_FOUND, 'File not found')
        except Exception as e:
            self.log_message("ERROR", f"Error handling GET request: {str(e)}")
            self.send_error(http.HTTPStatus.INTERNAL_SERVER_ERROR, 'Internal Server Error')


def sigint_handler(sig, frame):
    print("\nServer stopped by user")
    global keep_running
    keep_running = False
    httpd.shutdown()
    sys.exit(0)


def main(port):
    global httpd, keep_running
    server_address = ('', port)
    httpd = ThreadedHTTPServer(server_address, ApRequestHandler)
    print(f'Starting http server on port {port}...')
    thread = threading.Thread(target=httpd.serve_forever, daemon=True)
    thread.start()
    if is_windows():
        open_web(f'http:127.0.0.1:{args.port}/ap/')
    while keep_running:
        time.sleep(1)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Run a ap dump HTTP server.")
    parser.add_argument("-p", "--port", type=int, default=9001, help="Specify the server port.")
    args = parser.parse_args()
    signal.signal(signal.SIGINT, sigint_handler)
    main(args.port)
