# GitClient

A reimplementation of Git's plumbing and porcelain in modern C++23 from scratch.

Primary goal is to understand Git by building it: SHA-1 algorithm is hand-rolled, objects are
serialized byte-for-byte in Git's own format, and loose objects are zlib-deflated
into `.git/objects/ab/cdef…` Object hashes produced
by this client match the ones real `git` produces for the same content.

## Features

| Command | Description |
| --- | --- |
| `init` | Create a `.git` skeleton (`objects/`, `refs/heads/`, `HEAD` → `refs/heads/main`) |
| `hash-object <file>` | Hash a file as a blob and write it to the object store |
| `cat-file <sha>` | Print the raw payload of any object |
| `write-tree` | Snapshot the working directory into a tree object |
| `ls-tree <sha>` | List the entries of a tree object |
| `add <path>` | Stage a file, or recursively stage a directory |
| `commit -m <msg>` | Build a tree from the index, write a commit, advance `HEAD` |
| `log` | Walk first parents from `HEAD` and print each commit |

## Building

Requires a C++23 compiler, CMake 3.20+, and zlib. [doctest](https://github.com/doctest/doctest)
is fetched automatically at configure time.

```sh
cmake -S . -B build
cmake --build build
```

The build produces the `GitClient` CLI and a `gitcore` static library holding all
of the logic  `src/main.cpp` is only argument parsing and dispatch.

Debug builds additionally enable AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

## Testing

```sh
cmake --build build && ./build/tests
# or
ctest --test-dir build
```

45 doctest cases cover SHA-1 digests, zlib compressions and uncompressions, object
serialization and parsing, index behavior, and ref resolution. Several tests
assert that `write_tree` (walking the worktree) and `write_tree_from_index`
(walking the staging area) produce identical hashes.

## Usage

```sh
mkdir demo && cd demo
../build/GitClient init
echo "hello" > hello.txt
../build/GitClient add hello.txt
../build/GitClient commit -m "Initial commit"
../build/GitClient log
```

Because the on-disk object format is Git's, real `git` can read what this client
writes:

```sh
../build/GitClient hash-object hello.txt   # same digest as `git hash-object`
git cat-file -p <that digest>
```

## Architecture

All logic lives in `src/GitClient/`, layered from the bottom up:

- **`sha1`** — hand-written SHA-1 implementation, plus hex to byte conversion.
- **`compress`** — zlib deflate/inflate wrappers over `std::vector<std::byte>`.
- **`object_store`** — the loose object layer. `write_record` prepends the
  `"<type> <size>\0"` header, hashes the result, and writes the zlib-compressed
  bytes to `objects/<first-2>/<rest-38>`.
- **`objects`** — typed objects on top of the store. Tree entry parsing and
  serialization (including Git's quirky sort order), commit serialization and
  parsing, and a small `GitObject` hierarchy (`Blob`, `Tree`, `Commit`).
- **`index`** — the staging area: reading and writing the index, recursive `add`,
  and building nested tree objects from flat index paths.
- **`repository`** — repository creation, `HEAD` and ref resolution, `update_ref`,
  and the first-parent history walk behind `log`.

## Known deviations from Git

This is a learning project, and a few things are deliberately simplified:

- **The index is a custom text format** stored at `.git/mygit-index`, one
  `<mode> <sha> <path>` line per entry — not Git's binary `.git/index`. Real
  `git` will not see this client's staged files, and vice versa.
- **Commit identity is hardcoded** to `Jane Doe <janedoe@example.com>` with a
  fixed `-0400` offset; there is no config file yet.
- **Flags are not parsed.** `hash-object` always writes (as if `-w`) and
  `cat-file` always pretty-prints the payload (as if `-p`).
- **Only blobs and trees are handled by `ls-tree`**; symlink and submodule modes
  will throw.
- **No packfiles, branches, merges, remotes, or diffs** — only loose objects and
  a single linear history on `refs/heads/main`.
