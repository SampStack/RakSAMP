# Server scripting

The development server loads source `.lua` files named by the `scripts` configuration attribute from the adjacent `scripts/` directory.

```xml
<server scripts="basic" ... />
```

Multiple names use the format already supported by the legacy loader. Start with [the bundled example](../server/bin/scripts/basic.lua).

RakSAMP now embeds official Lua 5.4.8 and opens the full standard library. Scripts can access files, processes, native modules, and the host environment; only run scripts you trust.

Existing RakSAMP function and callback names remain the public scripting interface. Key callbacks include:

- `onScriptStart`, `onScriptExit`
- `onNewConnection`, `onNewQuery`
- `onPlayerJoin`, `onPlayerDisconnect`
- `onPlayerRequestClass`, `onPlayerSpawn`, `onPlayerDeath`
- `onPlayerMessage`, `onPlayerCommand`
- `onDialogResponse`, `onPlayerWeaponShot`

The complete registered function list is maintained in `server/src/ScrFunctions.cpp`.

Only source Lua is supported. Bytecode produced by older Lua versions is incompatible with Lua 5.4 and should be rebuilt from source.
