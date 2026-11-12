# TinyKV-C

A lightweight, zero-dependency **Key-Value Store with Write-Ahead Log (WAL) Persistence** implemented in **C11**.

## Highlights
- **Open-Addressing / Chained Hash Table**: Custom FNV-1a hashing with dynamic resizing at >70% load factor.
- **Crash-Safe WAL Persistence**: Every `SET` and `DEL` operation is appended to a structured Write-Ahead Log (`SET`/`DEL` records) and replayed cleanly on startup.
- **Interactive CLI & Pipe Mode**: Supports `SET <key> <value>`, `GET <key>`, `DEL <key>`, `COUNT`, `COMPACT`, and `EXIT`.
- **Automated C Test Suite & CI**: Covers collisions, dynamic resizing, overwrites, deletions, WAL recovery, and log compaction via `make test` and GitHub Actions.

## Build & Run

```bash
make
./tinykv data.wal
make test
```
