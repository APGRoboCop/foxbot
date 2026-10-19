# Bot button activation and creation timing fixes

This change updates two bot behaviors in the beta3 source.

## Button activation

`BotFindItem()` distinguishes button behavior:

- Touch-only buttons (`SFLAG_PROXIMITY_BUTTON`) use `JOB_PUSH_BUTTON` and are activated by walking into them.
- Ordinary non-damageable buttons use `JOB_PUSH_BUTTON` and the existing `IN_USE` behavior.
- Non-touch buttons marked damageable use `JOB_ATTACK_BREAKABLE` and are shot.

`JobPushButton()` already handled touch and `IN_USE` activation; its comments now describe the distinction.

## Bot timing initialization

`BotCreate()` initializes `fUpdateTime` and `fLastRunPlayerMoveTime` after any new-record zeroing. This covers newly created bots and reused records after map changes. `BotThink()` uses `fLastRunPlayerMoveTime` to calculate the `msec` passed to `RunPlayerMove()`.

The movement timestamp is not reset in `BotSpawnInit()`, which also runs after ordinary deaths.
