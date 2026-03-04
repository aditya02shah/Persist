## Persist

A log-structured key-value storage engine, written in C, inspired by Riak's [Bitcask](https://riak.com/assets/bitcask-intro.pdf) paper.

It is designed to handle variable-length key-value pairs.

It supports the following operations: `put`, `get`, `delete`

## How it works:

- Key-value pairs are represented as length-prefixed byte-arrays, and stored on disk for peristence
- Keys are also maintained in an in-memory hashtable, for quick lookups
- All writes are append-only operations, to minimize IOs
- Deletes create tombstones, which are cleanup by the `merge` operation
- Each persist directory contains a `.metadata` file to keep track of the latest file being written to

## Usage:

- `make persist` - Creates the executable
- `open` <dirname> - Opens a persist directory
- `merge`- Removes unused entries from the directory
- `exit` - Stop the storage engine
