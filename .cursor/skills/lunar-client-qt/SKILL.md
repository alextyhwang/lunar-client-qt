---
name: lunar-client-qt
description: Domain knowledge for the ATW Client launcher (lunar-client-qt fork). Use when working on this repository, modifying launch behavior, account handling, or integrating with Lunar Client.
---

# ATW Client

C++/Qt launcher that bypasses the official Lunar Client launcher and starts Genesis directly. Rebranded as ATW Client.

## Key Paths

| Purpose | Path |
|---------|------|
| Lunar Client data | `~/.lunarclient` (FS::getLunarDirectory()) |
| **Accounts** | `~/.lunarclient/settings/game/accounts.json` |
| Launcher config | `%APPDATA%/lunar-client-qt/settings.json` |
| Game working dir | `~/.lunarclient/offline/multiver` |
| Game dir (Minecraft) | `~/.minecraft` (or custom) |

## Accounts

Accounts live in Lunar Client's `accounts.json`, **not** in lunar-client-qt config. The launcher reads from it to pass credentials to Genesis.

**Critical**: Passing `--accessToken "0"` puts Genesis in offline mode—accounts disappear, login does nothing, and Genesis may reset accounts.json. To use real accounts, read the active account from `accounts.json` and pass:
- `--accessToken` (real token)
- `--username` (from minecraftProfile.name)
- `--uuid` (from minecraftProfile.id, add hyphens: 8-4-4-4-12)
- `--userProperties` (JSON, often `[]` or `{}`)

**accounts.json structure**: `accounts[activeAccountLocalId]` has `accessToken`, `minecraftProfile` (id, name), `userProperties`. Use `activeAccountLocalId` to find the active account.

## Genesis Launch

Main class: `com.moonsworth.lunar.genesis.Genesis`

Required args: `--version`, `--accessToken`, `--assetIndex`, `--userProperties`, `--gameDir`, `--workingDirectory`, `--classpathDir`, `--ichorClassPath`, `--ichorExternalFiles`, `--launcherVersion`.

## Code Locations

- Launch logic: `src/launch/offlinelauncher.cpp`
- Path helpers: `src/util/fs.cpp`
- Config load/save: `src/config/config.cpp`

## Workflow

**Rebuild the project after every turn** when making code changes. Run from the build directory: `cmake --build .` or `mingw32-make`.
