# Redis-Valkey

![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C.svg)
![Tests](https://img.shields.io/badge/tests-GoogleTest-4285F4.svg)

An educational in-memory key-value store in C++20 with a Redis/Valkey-style console interface. Work with strings, lists, sets, and geographic coordinates, set key expiration times, and explore command execution from parsing to storage.

## Features

- **Four value types**: strings, lists, sets, and named geographic points.
- **38 commands**: key management, collection operations, geospatial queries, and memory configuration.
- **Key expiration**: `EXPIRE` and `TTL` with cleanup of expired entries.
- **Geospatial search**: distance queries, radius searches around coordinates, sorting, and result limits.
- **Memory controls**: estimated per-key usage and a configurable memory limit.
- **Console workflow**: read commands interactively or from standard input; command names are case-insensitive.
- **GoogleTest coverage**: command behavior, argument validation, parsers, output formatting, and equivalence classes, integrated with CTest.

## Quick Start

Requirements: a C++20 compiler and standard library, CMake 3.16 or newer, and a build tool such as Ninja or Make. Tests use GoogleTest v1.14.0, fetched through Git during CMake configuration; the first test-enabled configuration needs Git and network access.

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Start the console with a single-configuration generator such as Ninja or Make:

```bash
./build/bin/redis
```

On Windows with Ninja:

```powershell
.\build\bin\redis.exe
```

For a multi-configuration generator such as Visual Studio, build with `--config Debug`, run CTest with `-C Debug`, and use `build/bin/Debug/redis.exe`.

Enter one command per line. Use `EXIT` or end-of-file to close the application. Data lives only for the duration of the process.

## Examples

### Strings and expiration

```text
SET greeting hello
GET greeting
APPEND greeting ,world
STRLEN greeting
EXPIRE greeting 60
TTL greeting
```

`GET greeting` returns `"hello"`; `APPEND` and `STRLEN` return `(integer) 11`. `TTL` reports the remaining lifetime in seconds, `-1` for a key without expiration, and `-2` for a missing key.

### Lists and sets

```text
RPUSH tasks build test document
LRANGE tasks 0 -1
LPOP tasks
SADD languages cpp rust cpp
SMEMBERS languages
SISMEMBER languages cpp
```

List ranges include both endpoints and support negative indices. Sets keep unique members, so the repeated `cpp` is stored once.

### Geographic queries

```text
GEOADD cities 13.361389 38.115556 Palermo 15.087269 37.502669 Catania
GEOPOS cities Palermo Catania
GEODIST cities Palermo Catania km
GEOSEARCH cities FROMLONLAT 13.361389 38.115556 BYRADIUS 200 km ASC COUNT 2
```

`GEOADD` accepts longitude before latitude. Search supports `FROMLONLAT` with `BYRADIUS` and optional `ASC`, `DESC`, and `COUNT`. Results can also be stored under another key with `GEOSEARCHSTORE`.

### Memory configuration

```bash
./build/bin/redis --maxmemory 64mb
```

The limit can also be inspected and changed during a session:

```text
CONFIG GET maxmemory
CONFIG SET maxmemory 32mb
MEMORY USAGE cities
```

Memory values accept bytes or the suffixes `b`, `kb`, `mb`, and `gb`. A limit of `0` means unlimited. Usage is an estimate based on the stored values, not a measurement of total process memory.

## Supported Commands

| Family | Commands |
| --- | --- |
| Strings | `SET`, `GET`, `STRLEN`, `APPEND` |
| Keys | `TYPE`, `DEL`, `EXISTS`, `DBSIZE`, `FLUSHDB`, `KEYS` |
| Expiration | `EXPIRE`, `TTL` |
| Memory | `MEMORY USAGE`, `CONFIG SET`, `CONFIG GET` |
| Lists | `LPUSH`, `RPUSH`, `LLEN`, `LINDEX`, `LSET`, `LPOP`, `RPOP`, `LINSERT`, `LRANGE` |
| Sets | `SADD`, `SMEMBERS`, `SCARD`, `SISMEMBER`, `SREM`, `SMOVE`, `SDIFF`, `SUNION`, `SINTER` |
| Geo | `GEOADD`, `GEOPOS`, `GEODIST`, `GEOSEARCH`, `GEOSEARCHSTORE` |

`CONFIG SET` and `CONFIG GET` support `maxmemory`. Command names follow Redis/Valkey conventions, but only the implemented argument forms and options are supported.

## Project Structure

```text
.
├── CMakeLists.txt
├── bin/
│   └── main.cpp                  # Console entry point
├── include/
│   ├── commands/                 # Command contracts and implementations
│   ├── database/                 # Storage, items, and Geo parser interfaces
│   ├── handlers/                 # Parsing, dispatch, and output interfaces
│   └── types/                    # Stored values and command results
├── src/
│   ├── database/                 # Storage operations by command family
│   └── handlers/                 # Parser, command handler, and printer
└── tests/                       # GoogleTest suites and CTest integration
```

## Design Highlights

The console follows a small execution pipeline:

```text
stdin → Parser → CommandHandler → ICommand → DataBase
                                               ↓
stdout / stderr ← Printer ← Result ─────────────┘
```

`CommandHandler` resolves one-word and two-word command names and dispatches through `ICommand`. Each command validates its arguments before calling the database. Geo search options have a dedicated parser.

`DataBase` stores keys in an `std::unordered_map`. Values use `std::variant` over string, list, set, and Geo types. Lists use `std::deque`, sets use `std::set`, and Geo values map member names to coordinates.

`Result` carries success, errors, missing values, integers, strings, arrays, and Geo positions. `Printer` converts these into console output, including ordered missing entries in geographic position results.

## Testing

Tests cover the four data families, shared key operations, expiration, memory limits, argument boundaries, Geo parsing, command dispatch, and console formatting.

Run the complete suite:

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

Run only the list command tests:

```bash
ctest --test-dir build -R ListCommands --output-on-failure
```

`BUILD_TESTS` defaults to `OFF`; enable it during configuration to build and register the tests.

## Scope

This is a standalone console application with a subset of Redis/Valkey-style behavior. It has no network server, RESP protocol, persistence, or replication. Input is split on whitespace; quoted strings and escape sequences are not parsed, so each key and value must be a single token.

## Explore the Code

- [Command implementations](include/commands/)
- [Database interface](include/database/database.hpp)
- [Stored value types](include/types/db_value.hpp)
- [Result model](include/types/result.hpp)
- [Command dispatcher](src/handlers/command_handler.cpp)
- [Test suites](tests/)
