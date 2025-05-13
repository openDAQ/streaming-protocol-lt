/*
 * Copyright 2022-2025 openDAQ d.o.o.
 *
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

#pragma once
namespace daq::jsonrpc {
    /// some error codes from the JSON RPC spec
    static const int parseError = -32700;
    static const int invalidRequest = -32600;
    static const int methodNotFound = -32601;
    static const int invalidParams = -32602;
    static const int internalError = -32603;

    /// some string constants from the JSON RPC spec
    static const char JSONRPC[] = "jsonrpc";
    static const char METHOD[] = "method";
    static const char RESULT[] = "result";
    static const char ERR[] = "error";
    static const char CODE[] = "code";
    static const char MESSAGE[] = "message";
    static const char DATA[] = "data";
    static const char PARAMS[] = "params";
    static const char ID[] = "id";

    // example of an successfull rpc
    // --> {"jsonrpc": "2.0", "method": "subtract", "params": [42, 23], "id": 1}
    // <-- {"jsonrpc": "2.0", "result": 19, "id": 1}

    // example of a failure
    //--> {"jsonrpc": "2.0", "method": "foobar", "id": "1"}
    //<-- {"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found", "data" : {}}, "id": "1"}
}
