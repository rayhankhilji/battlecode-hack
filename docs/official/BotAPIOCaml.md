# CAMBC Bot API — OCaml

Event: cambc2026a. Read alongside GameRules.md. Signatures and descriptions come from the same SDK documentation shown on the Bot API page.

Examples are fragments for an initialised bot. Check visibility and action preconditions. 

```ocaml
open Cambc
```

## Cambc

### Cambc_error

```ocaml
exception Cambc_error of Error_code.t * string
```

Raised by every failing call.

Public SDK type declaration.

```ocaml
exception Cambc_error of Error_code.t * string
```

### camp

```ocaml
type camp
```



Public SDK type declaration.

```ocaml
type camp
```

### champion

```ocaml
type champion
```



Public SDK type declaration.

```ocaml
type champion
```

### champions

```ocaml
val champions : ?team:Team.t -> unit -> Champion.t list
```

`champions ?team ()` is every roster champion, including hidden ones. @param team Team to include; both teams when omitted.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Use this value in your bot's draft or turn logic.

```ocaml
let result = champions ()
```

### constants

```ocaml
val constants : unit -> Constants.t
```

Game constants loaded at startup.

Use this value in your bot's draft or turn logic.

```ocaml
let result = constants ()
```

### debug_block

```ocaml
val debug_block : (unit -> unit) -> unit
```

`debug_block f` runs `f` without spending fuel when the bot is built in debug mode (`cambc run`, the default); skipped entirely in release mode (`--release`, and always on the platform). The free budget per block is 1000 turns, beyond which the rest is charged; ending the turn inside closes the free window. Usage: `debug_block (fun () -> expensive_debug_operation ())`. @param f Runs in debug mode only; never called in release mode.

Use this value in your bot's draft or turn logic.

Inputs:
- value1: (unit -> unit) (types from Cambc)

```ocaml
let result = debug_block value1
```

### end_turn

```ocaml
val end_turn : unit -> unit
```

End the current turn.

Stop optional planning when little fuel remains and yield the current turn.

```ocaml
let yield_if_low_on_fuel () =
  if fuel_remaining () < 1000 then end_turn ()
```

### fuel_remaining

```ocaml
val fuel_remaining : unit -> int
```

Fuel left in the current turn.

Stop optional planning when little fuel remains and yield the current turn.

```ocaml
let yield_if_low_on_fuel () =
  if fuel_remaining () < 1000 then end_turn ()
```

### global_submitted_gems

```ocaml
val global_submitted_gems : Team.t -> int
```

`global_submitted_gems team` is the total gems the team has tributed. @param team Team to count.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Use this value in your bot's draft or turn logic.

Inputs:
- value1: Team.t (types from Cambc)

```ocaml
let result = global_submitted_gems value1
```

### init

```ocaml
val init : unit -> unit
```

Load the SDK caches. This already runs at startup, before the bot's modules; calling it again does nothing.

Use this value in your bot's draft or turn logic.

```ocaml
let result = init ()
```

### is_draft

```ocaml
val is_draft : unit -> bool
```

True when this instance runs the draft rather than controlling a champion.

Use this value in your bot's draft or turn logic.

```ocaml
let is_draft = is_draft ()
```

### Missing_identity

```ocaml
exception Missing_identity
```

A champion design from `Champion.designs` used where a live champion is needed; Rust `Error::MissingIdentity`.

Public SDK type declaration.

```ocaml
exception Missing_identity
```

### No_range

```ocaml
exception No_range
```

An ability without a targeting range (attack reset); Rust `Error::NoRange`.

Public SDK type declaration.

```ocaml
exception No_range
```

### resign

```ocaml
val resign : unit -> unit
```

Resign the game for the bot's team.

Use this value in your bot's draft or turn logic.

```ocaml
let result = resign ()
```

### round_number

```ocaml
val round_number : unit -> int
```

The current round number.

Use this value in your bot's draft or turn logic.

```ocaml
let result = round_number ()
```

### self_champion

```ocaml
val self_champion : unit -> Champion.t
```

The champion this bot controls; raises during the draft.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Use this value in your bot's draft or turn logic.

```ocaml
let result = self_champion ()
```

### self_champion_opt

```ocaml
val self_champion_opt : unit -> Champion.t option
```

Like `self_champion`, but `None` during the draft.

Use this value in your bot's draft or turn logic.

```ocaml
let result = self_champion_opt ()
```

## Cambc.Ability

### can_target_allies

```ocaml
val can_target_allies : t -> bool
```

`can_target_allies a` is true when the ability can target allied champions. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let can_target_allies = Ability.can_target_allies value1
```

### can_target_camps

```ocaml
val can_target_camps : t -> bool
```

`can_target_camps a` is true when the ability can target camps. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let can_target_camps = Ability.can_target_camps value1
```

### can_target_enemies

```ocaml
val can_target_enemies : t -> bool
```

`can_target_enemies a` is true when the ability can target enemy champions. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let can_target_enemies = Ability.can_target_enemies value1
```

### can_target_ground

```ocaml
val can_target_ground : t -> bool
```

`can_target_ground a` is true when the ability can target a tile. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let can_target_ground = Ability.can_target_ground value1
```

### can_target_self

```ocaml
val can_target_self : t -> bool
```

`can_target_self a` is true when the ability can target its caster. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let can_target_self = Ability.can_target_self value1
```

### cast

```ocaml
val cast : ?target:[ `Champion of champion | `Camp of camp ] -> ?at:Vec2.t -> t -> bool
```

Pass at most one of `~target` and `~at`. Unit casts land on the target's tile; ground casts take a tile. Returns false for a newly revealed obstruction; known invalid destinations raise `Cambc_error`. @param target Champion or camp to cast on. @param at Ground tile to cast at. @param a Ability slot of the bot's own champion. Altar locks are not valid ability targets.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.cast value1
```

### cooldown

```ocaml
val cooldown : t -> int
```

`cooldown a` is the remaining cooldown; 0 means ready. @param a Ability slot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.cooldown value1
```

### damage_amount

```ocaml
val damage_amount : ?against:champion -> t -> float
```

`damage_amount ?against a` is the damage the ability would deal, after mitigation and shields when `against` is given. @param against Optional visible champion the amount is computed against. @param a Ability slot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.damage_amount value1
```

### heal_amount

```ocaml
val heal_amount : ?against:champion -> t -> float
```

`heal_amount ?against a` is the healing the ability would do, capped at the target's missing health when `against` is given. @param against Optional visible champion the amount is computed against. @param a Ability slot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.heal_amount value1
```

### info

```ocaml
type info = | Attack_reset of { properties : properties; enhanced_attack : bool } | Damage of { properties : properties; range : float; modifier : float } | Aoe_damage of { properties : properties; range : float; modifier : float; radius : float; duration : int; } | Cc of { properties : properties; range : float; modifier : float; duration : int } | Heal of { properties : properties; range : float; modifier : float } | Dash of { properties : properties; range : float } | Shield of { properties : properties; range : float; modifier : float; duration : int }
```



Public SDK type declaration.

```ocaml
type info = | Attack_reset of { properties : properties; enhanced_attack : bool } | Damage of { properties : properties; range : float; modifier : float } | Aoe_damage of { properties : properties; range : float; modifier : float; radius : float; duration : int; } | Cc of { properties : properties; range : float; modifier : float; duration : int } | Heal of { properties : properties; range : float; modifier : float } | Dash of { properties : properties; range : float } | Shield of { properties : properties; range : float; modifier : float; duration : int }
```

### info

```ocaml
val info : t -> info option
```

`info a` is the cached metadata, or `None` for an unused slot. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.info value1
```

### is

```ocaml
val is : t -> Ability_kind.t -> bool
```

`is a k` is true when the slot holds an ability of kind `k`. @param a Ability slot. @param k Ability kind.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)
- value2: Ability_kind.t (types from Cambc.Ability)

```ocaml
let result = Ability.is value1 value2
```

### is_aoe_damage

```ocaml
val is_aoe_damage : t -> bool
```

Check whether this is an area-damage ability. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_aoe_damage = Ability.is_aoe_damage value1
```

### is_attack_reset

```ocaml
val is_attack_reset : t -> bool
```

Check whether this is an attack-reset ability. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_attack_reset = Ability.is_attack_reset value1
```

### is_cc

```ocaml
val is_cc : t -> bool
```

Check whether this is a crowd-control ability. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_cc = Ability.is_cc value1
```

### is_damage

```ocaml
val is_damage : t -> bool
```

Check whether this is a damage ability. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_damage = Ability.is_damage value1
```

### is_dash

```ocaml
val is_dash : t -> bool
```

Check whether this is a dash ability. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_dash = Ability.is_dash value1
```

### is_heal

```ocaml
val is_heal : t -> bool
```

Check whether this is a healing ability. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_heal = Ability.is_heal value1
```

### is_on_cooldown

```ocaml
val is_on_cooldown : t -> bool
```

`is_on_cooldown a` is true while the cooldown is running. @param a Ability slot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_on_cooldown = Ability.is_on_cooldown value1
```

### is_ready

```ocaml
val is_ready : t -> bool
```

Check whether this ability is ready to cast. @param a Ability slot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_ready = Ability.is_ready value1
```

### is_shield

```ocaml
val is_shield : t -> bool
```

Check whether this is a shield ability. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let is_shield = Ability.is_shield value1
```

### kind

```ocaml
val kind : info -> Ability_kind.t
```

`kind i` is the ability's kind. @param i Ability metadata.

Evaluate an ability before choosing a spell.

Inputs:
- value1: info (types from Cambc.Ability)

```ocaml
let result = Ability.kind value1
```

### mask_into

```ocaml
val mask_into : t -> Bitboard.t -> unit
```

Like `valid_tiles`, but writes into `out`. Raises `No_range` for attack resets. @param a Ability slot. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)
- value2: Bitboard.t (types from Cambc.Ability)

```ocaml
let result = Ability.mask_into value1 value2
```

### properties

```ocaml
type properties = { max_cooldown : int; valid_targets : Ability_target.t; exclusive_with_movement : bool; }
```



Public SDK type declaration.

```ocaml
type properties = { max_cooldown : int; valid_targets : Ability_target.t; exclusive_with_movement : bool; }
```

### properties

```ocaml
val properties : info -> properties
```

`properties i` is the properties shared by every ability kind. @param i Ability metadata.

Evaluate an ability before choosing a spell.

Inputs:
- value1: info (types from Cambc.Ability)

```ocaml
let result = Ability.properties value1
```

### range

```ocaml
val range : info -> float option
```

`range i` is the casting range, or `None` for range-less abilities. @param i Ability metadata.

Evaluate an ability before choosing a spell.

Inputs:
- value1: info (types from Cambc.Ability)

```ocaml
let result = Ability.range value1
```

### shield_amount

```ocaml
val shield_amount : ?against:champion -> t -> float
```

`shield_amount ?against a` is the shield the ability would grant. @param against Optional visible champion the amount is computed against. @param a Ability slot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.shield_amount value1
```

### slot

```ocaml
val slot : t -> int
```

`slot a` is the slot index on its champion. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.slot value1
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

### valid_targets

```ocaml
val valid_targets : t -> Ability_target.t
```

`valid_targets a` is the categories of target the ability accepts. @param a Ability slot.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.valid_targets value1
```

### valid_tiles

```ocaml
val valid_tiles : t -> Bitboard.t
```

`valid_tiles a` is the tiles within casting range; visibility and occupancy are not checked. @param a Ability slot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate an ability before choosing a spell.

Inputs:
- value1: t (types from Cambc.Ability)

```ocaml
let result = Ability.valid_tiles value1
```

## Cambc.Ability_kind

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Evaluate an ability before choosing a spell.

```ocaml
let result = Ability_kind.all
```

### t

```ocaml
type t = | Attack_reset | Damage | Aoe_damage | Cc | Heal | Dash | Shield
```



Public SDK type declaration.

```ocaml
type t = | Attack_reset | Damage | Aoe_damage | Cc | Heal | Dash | Shield
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Ability_target

### add

```ocaml
val add : flag -> t -> t
```

`add f s` is `s` with `f` set. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Ability_target)
- value2: t (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.add value1 value2
```

### all

```ocaml
val all : flag list
```

Every named flag.

Combine or inspect flags before deciding which actions or targets are allowed.

```ocaml
let result = Ability_target.all
```

### diff

```ocaml
val diff : t -> t -> t
```

`diff a b` has the flags of `a` not in `b`. @param a First set. @param b Flags to remove.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Ability_target)
- value2: t (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.diff value1 value2
```

### empty

```ocaml
val empty : t
```

The set with no flags.

Combine or inspect flags before deciding which actions or targets are allowed.

```ocaml
let result = Ability_target.empty
```

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true for identical sets. @param a First set. @param b Second set.

### flag

```ocaml
type flag = Camps | Enemies | Allies | Self | Ground
```



Public SDK type declaration.

```ocaml
type flag = Camps | Enemies | Allies | Self | Ground
```

### hash

```ocaml
val hash : t -> int
```

Hash consistent with `equal`. @param s Flag set.

### inter

```ocaml
val inter : t -> t -> t
```

`inter a b` has the flags of both sets. @param a First set. @param b Second set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Ability_target)
- value2: t (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.inter value1 value2
```

### is_empty

```ocaml
val is_empty : t -> bool
```

`is_empty s` is true when no flag is set. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Ability_target)

```ocaml
let is_empty = Ability_target.is_empty value1
```

### mem

```ocaml
val mem : flag -> t -> bool
```

`mem f s` is true when `s` contains `f`. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Ability_target)
- value2: t (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.mem value1 value2
```

### of_list

```ocaml
val of_list : flag list -> t
```

`of_list fs` is the set of the listed flags. @param fs Flags to set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag list (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.of_list value1
```

### remove

```ocaml
val remove : flag -> t -> t
```

`remove f s` is `s` with `f` cleared. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Ability_target)
- value2: t (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.remove value1 value2
```

### subset

```ocaml
val subset : t -> t -> bool
```

`subset a b` is true when every flag of `a` is in `b`. @param a Candidate subset. @param b Superset.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Ability_target)
- value2: t (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.subset value1 value2
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

### to_list

```ocaml
val to_list : t -> flag list
```

`to_list s` lists the set flags. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.to_list value1
```

### union

```ocaml
val union : t -> t -> t
```

`union a b` has the flags of either set. @param a First set. @param b Second set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Ability_target)
- value2: t (types from Cambc.Ability_target)

```ocaml
let result = Ability_target.union value1 value2
```

## Cambc.Altar_lock

### is_ally

```ocaml
val is_ally : t -> bool
```

Check whether this altar lock belongs to the observing team. @param l Altar lock snapshot.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Altar_lock)

```ocaml
let is_ally = Altar_lock.is_ally value1
```

### is_enemy

```ocaml
val is_enemy : t -> bool
```

Check whether this altar lock belongs to the opposing team. @param l Altar lock snapshot.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Altar_lock)

```ocaml
let is_enemy = Altar_lock.is_enemy value1
```

### t

```ocaml
type t = { team : Team.t; position : Vec2.t; hits_remaining : int }
```



Public SDK type declaration.

```ocaml
type t = { team : Team.t; position : Vec2.t; hits_remaining : int }
```

## Cambc.Bitboard

### clear

```ocaml
val clear : t -> unit
```

`clear b` clears every tile. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.clear value1
```

### complement

```ocaml
val complement : t -> t
```

`complement b` sets exactly the map tiles `b` leaves clear. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.complement value1
```

### copy

```ocaml
val copy : t -> t
```

`copy b` is an independent copy. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.copy value1
```

### count

```ocaml
val count : t -> int
```

`count b` is the number of set tiles. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.count value1
```

### create

```ocaml
val create : unit -> t
```

A board with no tiles set.

Build a pathfinding or targeting mask.

```ocaml
let result = Bitboard.create ()
```

### draw_chebyshev

```ocaml
val draw_chebyshev : t -> Vec2.t -> float -> unit
```

`draw_chebyshev b center radius` sets every tile within a Chebyshev radius. @param b Bitboard. @param center Centre tile. @param radius Chebyshev radius in tiles.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)
- value3: float (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.draw_chebyshev value1 value2 value3
```

### draw_circle

```ocaml
val draw_circle : t -> Vec2.t -> float -> unit
```

`draw_circle b center radius` sets every tile within a Euclidean radius. @param b Bitboard. @param center Centre tile. @param radius Euclidean radius in tiles.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)
- value3: float (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.draw_circle value1 value2 value3
```

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true when both boards set the same tiles. @param a First board. @param b Second board.

### export_linear_buffer

```ocaml
val export_linear_buffer : t -> Bytes.t
```

`export_linear_buffer b` is the contents as a row-major buffer. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.export_linear_buffer value1
```

### export_linear_buffer_into

```ocaml
val export_linear_buffer_into : t -> Bytes.t -> unit
```

Like `export_linear_buffer`, but writes into `out`. @param b Bitboard. @param out Destination, exactly `linear_buffer_size` long.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Bytes.t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.export_linear_buffer_into value1 value2
```

### export_raw_buffer

```ocaml
val export_raw_buffer : t -> Bytes.t
```

`export_raw_buffer b` is the contents as an engine-chunked buffer. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.export_raw_buffer value1
```

### export_raw_buffer_into

```ocaml
val export_raw_buffer_into : t -> Bytes.t -> unit
```

Like `export_raw_buffer`, but writes into `out`. @param b Bitboard. @param out Destination, exactly `raw_buffer_size` long.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Bytes.t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.export_raw_buffer_into value1 value2
```

### fill

```ocaml
val fill : t -> unit
```

`fill b` sets every tile on the map. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.fill value1
```

### height

```ocaml
val height : t -> int
```

Map height in tiles. @param b Bitboard.

### import_linear_buffer

```ocaml
val import_linear_buffer : t -> Bytes.t -> unit
```

`import_linear_buffer b data` replaces the contents from a row-major buffer. @param b Bitboard. @param data One value per tile in row-major order.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Bytes.t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.import_linear_buffer value1 value2
```

### import_raw_buffer

```ocaml
val import_raw_buffer : t -> Bytes.t -> unit
```

`import_raw_buffer b data` replaces the contents from an engine-chunked buffer. @param b Bitboard. @param data Engine-chunked bytes, exactly `raw_buffer_size` long.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Bytes.t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.import_raw_buffer value1 value2
```

### intersect

```ocaml
val intersect : t -> t -> t
```

`intersect a b` is a new board with tiles set in both. @param a First board. @param b Second board; may be `a`.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.intersect value1 value2
```

### intersect_assign

```ocaml
val intersect_assign : t -> t -> unit
```

`intersect_assign a b` keeps only the tiles of `a` also in `b`. @param a First board. @param b Second board; may be `a`.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.intersect_assign value1 value2
```

### is_empty

```ocaml
val is_empty : t -> bool
```

`is_empty b` is true when no tile is set. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let is_empty = Bitboard.is_empty value1
```

### is_set

```ocaml
val is_set : t -> Vec2.t -> bool
```

`is_set b p` is true when `p` is set; raises outside the map. @param b Bitboard. @param p Tile.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)

```ocaml
let is_set = Bitboard.is_set value1 value2
```

### linear_buffer_size

```ocaml
val linear_buffer_size : t -> int
```

Size in bytes of the row-major buffer. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.linear_buffer_size value1
```

### positions

```ocaml
val positions : t -> Vec2.t list
```

`positions b` lists the set tiles in row-major order. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.positions value1
```

### print

```ocaml
val print : t -> unit
```

`print b` prints the board to stdout for debugging. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.print value1
```

### raw_buffer_size

```ocaml
val raw_buffer_size : t -> int
```

Size in bytes of the engine-chunked buffer. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.raw_buffer_size value1
```

### reachable_tiles

```ocaml
val reachable_tiles : ?scratch:t * t -> t -> Vec2.t -> distance:int -> t
```

`reachable_tiles ?scratch walls origin ~distance` is the tiles reachable from `origin` around `walls`. @param scratch Optional pair of working boards; their contents are overwritten. @param walls Impassable tiles. @param origin Start tile. @param distance Maximum number of 8-directional steps.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)
- distance: int (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.reachable_tiles value1 value2 ~distance
```

### reachable_tiles_into

```ocaml
val reachable_tiles_into : ?scratch:t * t -> t -> Vec2.t -> distance:int -> t -> unit
```

`reachable_tiles walls origin ~distance`; `?scratch` is reusable (frontier, scratch) storage. @param scratch Optional pair of working boards; their contents are overwritten. @param walls Impassable tiles. @param origin Start tile. @param distance Maximum number of 8-directional steps. @param out Destination; its previous contents are overwritten.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)
- distance: int (types from Cambc.Bitboard)
- value4: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.reachable_tiles_into value1 value2 ~distance value4
```

### reflect

```ocaml
val reflect : t -> Symmetry.t -> t
```

`reflect b s` is a copy transformed by a map symmetry. @param b Bitboard. @param s Map symmetry.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Symmetry.t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.reflect value1 value2
```

### reflect_horizontal

```ocaml
val reflect_horizontal : t -> t
```

`reflect_horizontal b` is a copy reflected horizontally, reversing x. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.reflect_horizontal value1
```

### reflect_horizontal_into

```ocaml
val reflect_horizontal_into : t -> t -> unit
```

Like `reflect_horizontal`, but writes into `out`. @param b Bitboard. @param out Destination; its previous contents are overwritten.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.reflect_horizontal_into value1 value2
```

### reflect_into

```ocaml
val reflect_into : t -> Symmetry.t -> t -> unit
```

Like `reflect`, but writes into `out`. @param b Bitboard. @param s Map symmetry. @param out Destination; its previous contents are overwritten.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Symmetry.t (types from Cambc.Bitboard)
- value3: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.reflect_into value1 value2 value3
```

### reflect_vertical

```ocaml
val reflect_vertical : t -> t
```

`reflect_vertical b` is a copy reflected vertically, reversing y. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.reflect_vertical value1
```

### reflect_vertical_into

```ocaml
val reflect_vertical_into : t -> t -> unit
```

Like `reflect_vertical`, but writes into `out`. @param b Bitboard. @param out Destination; its previous contents are overwritten.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.reflect_vertical_into value1 value2
```

### rotate_180

```ocaml
val rotate_180 : t -> t
```

`rotate_180 b` is a copy rotated 180 degrees within the map. @param b Bitboard.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.rotate_180 value1
```

### rotate_180_into

```ocaml
val rotate_180_into : t -> t -> unit
```

Like `rotate_180`, but writes into `out`. @param b Bitboard. @param out Destination; its previous contents are overwritten.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.rotate_180_into value1 value2
```

### set

```ocaml
val set : t -> Vec2.t -> bool -> unit
```

`set b p value` sets or clears `p`; raises outside the map. @param b Bitboard. @param p Tile. @param value `true` to set, `false` to clear.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)
- value3: bool (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.set value1 value2 value3
```

### shadow_cast

```ocaml
val shadow_cast : t -> Vec2.t -> distance:int -> t
```

`shadow_cast walls origin ~distance` is the tiles visible from `origin` past opaque `walls`. @param walls Opaque tiles. @param origin Viewer tile. @param distance Maximum sight radius in tiles.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)
- distance: int (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.shadow_cast value1 value2 ~distance
```

### shadow_cast_into

```ocaml
val shadow_cast_into : t -> Vec2.t -> distance:int -> t -> unit
```

Like `shadow_cast`, but writes into `out`. @param walls Opaque tiles. @param origin Viewer tile. @param distance Maximum sight radius in tiles. @param out Destination; its previous contents are overwritten.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)
- distance: int (types from Cambc.Bitboard)
- value4: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.shadow_cast_into value1 value2 ~distance value4
```

### shift_right

```ocaml
val shift_right : t -> int -> t
```

`shift_right b count` is a copy shifted horizontally; tiles shifted off the map are dropped. @param b Bitboard. @param count Columns to shift; negative values shift left.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: int (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.shift_right value1 value2
```

### shift_right_into

```ocaml
val shift_right_into : t -> int -> t -> unit
```

`_into` functions overwrite the whole output. Unary bitboard operations may write in place. @param b Bitboard. @param count Columns to shift; negative values shift left. @param out Destination; its previous contents are overwritten.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: int (types from Cambc.Bitboard)
- value3: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.shift_right_into value1 value2 value3
```

### shift_up

```ocaml
val shift_up : t -> int -> t
```

`shift_up b count` is a copy shifted vertically; tiles shifted off the map are dropped. @param b Bitboard. @param count Rows to shift; negative values shift down.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: int (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.shift_up value1 value2
```

### shift_up_into

```ocaml
val shift_up_into : t -> int -> t -> unit
```

Like `shift_up`, but writes into `out`. @param b Bitboard. @param count Rows to shift; negative values shift down. @param out Destination; its previous contents are overwritten.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: int (types from Cambc.Bitboard)
- value3: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.shift_up_into value1 value2 value3
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

### toggle

```ocaml
val toggle : t -> Vec2.t -> unit
```

`toggle b p` flips `p`; raises outside the map. @param b Bitboard. @param p Tile.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: Vec2.t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.toggle value1 value2
```

### union

```ocaml
val union : t -> t -> t
```

`union a b` is a new board with tiles set in either. @param a First board. @param b Second board; may be `a`.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.union value1 value2
```

### union_assign

```ocaml
val union_assign : t -> t -> unit
```

`union_assign a b` adds the tiles of `b` to `a`. @param a First board. @param b Second board; may be `a`.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.union_assign value1 value2
```

### width

```ocaml
val width : t -> int
```

Map width in tiles. @param b Bitboard.

### xor

```ocaml
val xor : t -> t -> t
```

`xor a b` is a new board with tiles set in exactly one. @param a First board. @param b Second board; may be `a`.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.xor value1 value2
```

### xor_assign

```ocaml
val xor_assign : t -> t -> unit
```

`xor_assign a b` flips the tiles of `a` set in `b`. @param a First board. @param b Second board; may be `a`.

Build a pathfinding or targeting mask.

Inputs:
- value1: t (types from Cambc.Bitboard)
- value2: t (types from Cambc.Bitboard)

```ocaml
let result = Bitboard.xor_assign value1 value2
```

## Cambc.Buff

### duration

```ocaml
val duration : t -> int
```

`duration b` is the remaining time in rounds. @param b Buff snapshot.

Inspect a visible unit's active effects when evaluating a fight.

Inputs:
- value1: t (types from Cambc.Buff)

```ocaml
let result = Buff.duration value1
```

### get

```ocaml
val get : t list -> Buff_kind.t -> t option
```

`get buffs k` is the buff of kind `k`, if present. @param buffs Buff snapshots. @param k Buff kind.

Inspect a visible unit's active effects when evaluating a fight.

Inputs:
- value1: t list (types from Cambc.Buff)
- value2: Buff_kind.t (types from Cambc.Buff)

```ocaml
let result = Buff.get value1 value2
```

### has

```ocaml
val has : t list -> Buff_kind.t -> bool
```

`has buffs k` is true when a buff of kind `k` is present. @param buffs Buff snapshots. @param k Buff kind.

Inspect a visible unit's active effects when evaluating a fight.

Inputs:
- value1: t list (types from Cambc.Buff)
- value2: Buff_kind.t (types from Cambc.Buff)

```ocaml
let result = Buff.has value1 value2
```

### has_enhanced_attack

```ocaml
val has_enhanced_attack : t list -> bool
```

Check whether the collection has an enhanced-attack buff. @param buffs Buff snapshots.

Inspect a visible unit's active effects when evaluating a fight.

Inputs:
- value1: t list (types from Cambc.Buff)

```ocaml
let has_enhanced_attack = Buff.has_enhanced_attack value1
```

### has_shield

```ocaml
val has_shield : t list -> bool
```

Check whether the collection has a shield buff. @param buffs Buff snapshots.

Inspect a visible unit's active effects when evaluating a fight.

Inputs:
- value1: t list (types from Cambc.Buff)

```ocaml
let has_shield = Buff.has_shield value1
```

### has_stun

```ocaml
val has_stun : t list -> bool
```

Check whether the collection has a stun buff. @param buffs Buff snapshots.

Inspect a visible unit's active effects when evaluating a fight.

Inputs:
- value1: t list (types from Cambc.Buff)

```ocaml
let has_stun = Buff.has_stun value1
```

### kind

```ocaml
val kind : t -> Buff_kind.t
```

`kind b` is the buff's kind. @param b Buff snapshot.

Inspect a visible unit's active effects when evaluating a fight.

Inputs:
- value1: t (types from Cambc.Buff)

```ocaml
let result = Buff.kind value1
```

### shield_value

```ocaml
val shield_value : t -> float option
```

`shield_value b` is the remaining shield, or `None` for other buffs. @param b Buff snapshot.

Inspect a visible unit's active effects when evaluating a fight.

Inputs:
- value1: t (types from Cambc.Buff)

```ocaml
let result = Buff.shield_value value1
```

### t

```ocaml
type t = | Shield of { duration : int; value : float } | Stun of { duration : int } | Enhanced_attack of { duration : int }
```



Public SDK type declaration.

```ocaml
type t = | Shield of { duration : int; value : float } | Stun of { duration : int } | Enhanced_attack of { duration : int }
```

## Cambc.Buff_kind

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Inspect a visible unit's active effects when evaluating a fight.

```ocaml
let result = Buff_kind.all
```

### t

```ocaml
type t = | Shield | Stun | Enhanced_attack
```



Public SDK type declaration.

```ocaml
type t = | Shield | Stun | Enhanced_attack
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Buff_restrictions

### add

```ocaml
val add : flag -> t -> t
```

`add f s` is `s` with `f` set. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Buff_restrictions)
- value2: t (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.add value1 value2
```

### all

```ocaml
val all : flag list
```

Every named flag.

Combine or inspect flags before deciding which actions or targets are allowed.

```ocaml
let result = Buff_restrictions.all
```

### diff

```ocaml
val diff : t -> t -> t
```

`diff a b` has the flags of `a` not in `b`. @param a First set. @param b Flags to remove.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Buff_restrictions)
- value2: t (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.diff value1 value2
```

### empty

```ocaml
val empty : t
```

The set with no flags.

Combine or inspect flags before deciding which actions or targets are allowed.

```ocaml
let result = Buff_restrictions.empty
```

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true for identical sets. @param a First set. @param b Second set.

### flag

```ocaml
type flag = Move | Attack | Cast | Shop | Consumable | Ward
```



Public SDK type declaration.

```ocaml
type flag = Move | Attack | Cast | Shop | Consumable | Ward
```

### hash

```ocaml
val hash : t -> int
```

Hash consistent with `equal`. @param s Flag set.

### inter

```ocaml
val inter : t -> t -> t
```

`inter a b` has the flags of both sets. @param a First set. @param b Second set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Buff_restrictions)
- value2: t (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.inter value1 value2
```

### is_empty

```ocaml
val is_empty : t -> bool
```

`is_empty s` is true when no flag is set. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Buff_restrictions)

```ocaml
let is_empty = Buff_restrictions.is_empty value1
```

### mem

```ocaml
val mem : flag -> t -> bool
```

`mem f s` is true when `s` contains `f`. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Buff_restrictions)
- value2: t (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.mem value1 value2
```

### of_list

```ocaml
val of_list : flag list -> t
```

`of_list fs` is the set of the listed flags. @param fs Flags to set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag list (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.of_list value1
```

### remove

```ocaml
val remove : flag -> t -> t
```

`remove f s` is `s` with `f` cleared. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Buff_restrictions)
- value2: t (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.remove value1 value2
```

### subset

```ocaml
val subset : t -> t -> bool
```

`subset a b` is true when every flag of `a` is in `b`. @param a Candidate subset. @param b Superset.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Buff_restrictions)
- value2: t (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.subset value1 value2
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

### to_list

```ocaml
val to_list : t -> flag list
```

`to_list s` lists the set flags. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.to_list value1
```

### union

```ocaml
val union : t -> t -> t
```

`union a b` has the flags of either set. @param a First set. @param b Second set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Buff_restrictions)
- value2: t (types from Cambc.Buff_restrictions)

```ocaml
let result = Buff_restrictions.union value1 value2
```

## Cambc.Camp

### attack_range

```ocaml
val attack_range : t -> int
```

`attack_range c` is the attack range in Chebyshev tiles; melee units have range 1. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.attack_range value1
```

### attack_tiles

```ocaml
val attack_tiles : t -> Bitboard.t
```

`attack_tiles c` is the tiles within attack range of `c`'s position. @param c Camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.attack_tiles value1
```

### attack_tiles_into

```ocaml
val attack_tiles_into : t -> Bitboard.t -> unit
```

Like `attack_tiles`, but writes into `out`. @param c Camp. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)
- value2: Bitboard.t (types from Cambc.Camp)

```ocaml
let result = Camp.attack_tiles_into value1 value2
```

### base_stats

```ocaml
type base_stats = { vision_radius : float; base_move_speed : int; base_attack_range : int; base_health : float; health_growth : float; base_strength : float; strength_growth : float; base_focus : float; focus_growth : float; }
```



Public SDK type declaration.

```ocaml
type base_stats = { vision_radius : float; base_move_speed : int; base_attack_range : int; base_health : float; health_growth : float; base_strength : float; strength_growth : float; base_focus : float; focus_growth : float; }
```

### base_stats

```ocaml
val base_stats : t -> base_stats
```

`base_stats c` is the camp type's design stats. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.base_stats value1
```

### buffs

```ocaml
val buffs : t -> Buff.t list
```

`buffs c` is the current buffs; raises if `c` is not visible. @param c Camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.buffs value1
```

### camp_group

```ocaml
val camp_group : t -> Camp_group.t
```

`camp_group c` is the group the camp belongs to. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.camp_group value1
```

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true for the same camp. @param a First camp. @param b Second camp.

### focus

```ocaml
val focus : t -> float
```

`focus c` is current focus; raises if `c` is not visible. @param c Camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.focus value1
```

### hash

```ocaml
val hash : t -> int
```

Hash consistent with `equal`. @param c Camp.

### health

```ocaml
val health : t -> float
```

`health c` is current health; raises if `c` is not visible. @param c Camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.health value1
```

### is_alive

```ocaml
val is_alive : t -> bool
```

Query whether this camp is living (awake or asleep); uses the visibility checks of `state`.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_alive = Camp.is_alive value1
```

### is_asleep

```ocaml
val is_asleep : t -> bool
```

Query whether this camp is living but dormant; uses the visibility checks of `state`.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_asleep = Camp.is_asleep value1
```

### is_awake

```ocaml
val is_awake : t -> bool
```

Query whether this camp is awake; uses the same visibility checks as `state`.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_awake = Camp.is_awake value1
```

### is_dead

```ocaml
val is_dead : t -> bool
```

Query whether this camp is dead; uses the same visibility checks as `state`.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_dead = Camp.is_dead value1
```

### is_elk

```ocaml
val is_elk : t -> bool
```

Check whether this camp is an elk. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_elk = Camp.is_elk value1
```

### is_gem_cluster

```ocaml
val is_gem_cluster : t -> bool
```

Check whether this camp is a gem cluster. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_gem_cluster = Camp.is_gem_cluster value1
```

### is_gold_cluster

```ocaml
val is_gold_cluster : t -> bool
```

Check whether this camp is a gold cluster. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_gold_cluster = Camp.is_gold_cluster value1
```

### is_monolith

```ocaml
val is_monolith : t -> bool
```

Check whether this camp is a monolith. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_monolith = Camp.is_monolith value1
```

### is_visible

```ocaml
val is_visible : t -> bool
```

`is_visible c` is true when `c` is currently visible. @param c Camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_visible = Camp.is_visible value1
```

### is_wolf

```ocaml
val is_wolf : t -> bool
```

Check whether this camp is a wolf. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let is_wolf = Camp.is_wolf value1
```

### kind

```ocaml
val kind : t -> Camp_type.t
```

`kind c` is the camp's type. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.kind value1
```

### max_health

```ocaml
val max_health : t -> float
```

`max_health c` is current maximum health; raises if `c` is not visible. @param c Camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.max_health value1
```

### position

```ocaml
val position : t -> Vec2.t
```

`position c` is the current tile; raises if `c` is not visible. @param c Camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.position value1
```

### spawn_position

```ocaml
val spawn_position : t -> Vec2.t
```

`spawn_position c` is the camp's spawn tile, not its live position. @param c Camp.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.spawn_position value1
```

### state

```ocaml
type state = Camp_state.t = Dead | Alive | Asleep
```

Individual camp lifecycle. `Alive` means awake; `Asleep` means living but dormant.

Public SDK type declaration.

```ocaml
type state = Camp_state.t = Dead | Alive | Asleep
```

### state

```ocaml
val state : t -> state
```

Query `Dead`, `Alive` (awake), or `Asleep`. The current tile, or last occupied tile when dead, must be in vision; hidden camps raise `Api_not_in_vision`. This is separate from the camp-group state. @param c Camp to inspect.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.state value1
```

### strength

```ocaml
val strength : t -> float
```

`strength c` is current strength; raises if `c` is not visible. @param c Camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp)

```ocaml
let result = Camp.strength value1
```

### t

```ocaml
type t = camp
```



Public SDK type declaration.

```ocaml
type t = camp
```

## Cambc.Camp_group

### camps

```ocaml
val camps : t -> camp list
```

`camps g` is the camps in the group. @param g Camp group.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp_group)

```ocaml
let result = Camp_group.camps value1
```

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true for the same group. @param a First group. @param b Second group.

### gem_value

```ocaml
val gem_value : t -> int
```

`gem_value g` is the gems awarded for clearing the group. @param g Camp group.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp_group)

```ocaml
let result = Camp_group.gem_value value1
```

### gold_value

```ocaml
val gold_value : t -> int
```

`gold_value g` is the gold awarded for clearing the group. @param g Camp group.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp_group)

```ocaml
let result = Camp_group.gold_value value1
```

### hash

```ocaml
val hash : t -> int
```

Hash consistent with `equal`. @param g Camp group.

### max_respawn_timer

```ocaml
val max_respawn_timer : t -> int
```

`max_respawn_timer g` is the rounds the group takes to respawn after being cleared. @param g Camp group.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp_group)

```ocaml
let result = Camp_group.max_respawn_timer value1
```

### spawn_tiles

```ocaml
val spawn_tiles : t -> Bitboard.t
```

`spawn_tiles g` is the spawn tiles of the group's camps. The board is shared and cached: copy before mutating. @param g Camp group.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp_group)

```ocaml
let result = Camp_group.spawn_tiles value1
```

### spawn_tiles_into

```ocaml
val spawn_tiles_into : t -> Bitboard.t -> unit
```

Like `spawn_tiles`, but writes into `out`. @param g Camp group. @param out Destination; its previous contents are overwritten.

Evaluate a visible camp before choosing where to farm.

Inputs:
- value1: t (types from Cambc.Camp_group)
- value2: Bitboard.t (types from Cambc.Camp_group)

```ocaml
let result = Camp_group.spawn_tiles_into value1 value2
```

### state

```ocaml
type state = Alive | Reward_available | Respawning of int
```

Returned by Camp_group.state group. Alive has living camps; Reward_available is defeated but unclaimed; Respawning timer carries the remaining round countdown. Zero means respawning is blocked by occupied spawn tiles. Reading state requires at least one original camp spawn tile in current vision; otherwise the SDK reports a visibility error. max_respawn_timer gives the configured full duration, not the remaining countdown.

Read the remaining respawn rounds for a group with an original spawn tile in current vision. Alive and unclaimed-reward states have no countdown.

Inputs:
- group: a camp group currently in vision

```ocaml
match Camp_group.state group with
| Camp_group.Respawning timer -> Printf.printf "%d\n" timer
| _ -> ()
```

### state

```ocaml
val state : t -> state
```

`state g` is the group's current state. @param g Camp group.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Read the remaining respawn rounds for a group with an original spawn tile in current vision. Alive and unclaimed-reward states have no countdown.

Inputs:
- group: a camp group currently in vision

```ocaml
match Camp_group.state group with
| Camp_group.Respawning timer -> Printf.printf "%d\n" timer
| _ -> ()
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

## Cambc.Camp_state

### is_alive

```ocaml
val is_alive : t -> bool
```

Check whether the captured state is living, including asleep camps.

Inspect a camp's captured life and wake state.

Inputs:
- value1: t (types from Cambc.Camp_state)

```ocaml
let is_alive = Camp_state.is_alive value1
```

### is_asleep

```ocaml
val is_asleep : t -> bool
```

Check whether the captured state is living but dormant.

Inspect a camp's captured life and wake state.

Inputs:
- value1: t (types from Cambc.Camp_state)

```ocaml
let is_asleep = Camp_state.is_asleep value1
```

### is_awake

```ocaml
val is_awake : t -> bool
```

Check whether the captured state is awake (the `Alive` enum value).

Inspect a camp's captured life and wake state.

Inputs:
- value1: t (types from Cambc.Camp_state)

```ocaml
let is_awake = Camp_state.is_awake value1
```

### is_dead

```ocaml
val is_dead : t -> bool
```

Check whether the captured state is dead.

Inspect a camp's captured life and wake state.

Inputs:
- value1: t (types from Cambc.Camp_state)

```ocaml
let is_dead = Camp_state.is_dead value1
```

### t

```ocaml
type t = Dead | Alive | Asleep
```

An individual camp's captured lifecycle; states are mutually exclusive, not flags.

Public SDK type declaration.

```ocaml
type t = Dead | Alive | Asleep
```

## Cambc.Camp_type

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Evaluate a visible camp before choosing where to farm.

```ocaml
let result = Camp_type.all
```

### t

```ocaml
type t = | Wolf | Elk | Gem_cluster | Gold_cluster | Monolith
```



Public SDK type declaration.

```ocaml
type t = | Wolf | Elk | Gem_cluster | Gold_cluster | Monolith
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Champion

### abilities

```ocaml
val abilities : t -> Ability.t array
```

`abilities c` is the champion's ability slots. @param c Champion.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.abilities value1
```

### attack

```ocaml
val attack : t -> [ `Champion of champion | `Camp of camp | `Altar_lock of Altar_lock.t ] -> unit
```

`attack c target` basic-attacks a visible champion, camp or altar lock in range. @param c The bot's own champion. @param target Unit or altar lock to attack.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: [ `Champion of champion | `Camp of camp | `Altar_lock of Altar_lock.t ] (types from Cambc.Champion)

```ocaml
let result = Champion.attack value1 value2
```

### attack_range

```ocaml
val attack_range : t -> int
```

`attack_range c` is the attack range in Chebyshev tiles; melee champions have range 1. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.attack_range value1
```

### attack_tiles

```ocaml
val attack_tiles : t -> Bitboard.t
```

`attack_tiles c` is the tiles within attack range of `c`'s position. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.attack_tiles value1
```

### attack_tiles_into

```ocaml
val attack_tiles_into : t -> Bitboard.t -> unit
```

Like `attack_tiles`, but writes into `out`. @param c Champion. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Bitboard.t (types from Cambc.Champion)

```ocaml
let result = Champion.attack_tiles_into value1 value2
```

### base_stats

```ocaml
type base_stats = { vision_radius : int; base_move_speed : int; base_attack_range : int; base_health : float; health_growth : float; base_strength : float; strength_growth : float; base_focus : float; focus_growth : float; camp_damage_multiplier : float; }
```



Public SDK type declaration.

```ocaml
type base_stats = { vision_radius : int; base_move_speed : int; base_attack_range : int; base_health : float; health_growth : float; base_strength : float; strength_growth : float; base_focus : float; focus_growth : float; camp_damage_multiplier : float; }
```

### base_stats

```ocaml
val base_stats : t -> base_stats
```

`base_stats c` is the champion type's design stats. @param c Champion.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.base_stats value1
```

### buffs

```ocaml
val buffs : t -> Buff.t list
```

`buffs c` is the champion's current buffs; raises if `c` is not visible. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.buffs value1
```

### buy

```ocaml
val buy : ?count:int -> t -> Item.t -> unit
```

`buy ?count c item` buys items; the engine validates location, gold and recipe. @param count Number of copies; defaults to 1. @param c The bot's own champion. @param item Item type.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Item.t (types from Cambc.Champion)

```ocaml
let result = Champion.buy value1 value2
```

### buy_consumable

```ocaml
val buy_consumable : t -> Consumable.t -> unit
```

`buy_consumable c k` buys one consumable; the engine validates gold and carry limit. @param c The bot's own champion. @param k Consumable type.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Consumable.t (types from Cambc.Champion)

```ocaml
let result = Champion.buy_consumable value1 value2
```

### can_attack

```ocaml
val can_attack : t -> bool
```

Check whether this champion can attack. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check the action allowance before spending fuel to choose an attack target.

```ocaml
let action_available =
  let me = self_champion () in
  Champion.can_attack me
```

### can_buy

```ocaml
val can_buy : ?count:int -> t -> Item.t -> bool
```

`can_buy ?count c item` is true when `c` can buy `item` in its current state. @param count Number of copies; defaults to 1. @param c Champion. @param item Item type.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Item.t (types from Cambc.Champion)

```ocaml
let can_buy = Champion.can_buy value1 value2
```

### can_buy_consumable

```ocaml
val can_buy_consumable : t -> Consumable.t -> bool
```

`can_buy_consumable c k` is true when `c` can buy a consumable now. @param c Champion. @param k Consumable type.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Consumable.t (types from Cambc.Champion)

```ocaml
let can_buy_consumable = Champion.can_buy_consumable value1 value2
```

### can_cast

```ocaml
val can_cast : t -> bool
```

Check whether this champion can cast. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check the action allowance before spending fuel to evaluate ready abilities.

```ocaml
let action_available =
  let me = self_champion () in
  Champion.can_cast me
```

### can_move

```ocaml
val can_move : t -> bool
```

Check whether this champion can move. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check the action allowance before spending fuel to plan a path.

```ocaml
let action_available =
  let me = self_champion () in
  Champion.can_move me
```

### can_sell

```ocaml
val can_sell : t -> bool
```

`can_sell c` is true when `c` can sell items in its current state. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let can_sell = Champion.can_sell value1
```

### cancel_recall

```ocaml
val cancel_recall : t -> unit
```

`cancel_recall c` cancels the current recall. @param c The bot's own champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.cancel_recall value1
```

### cancel_submit_gems

```ocaml
val cancel_submit_gems : t -> unit
```

`cancel_submit_gems c` cancels the current tribute. @param c The bot's own champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.cancel_submit_gems value1
```

### champion_name

```ocaml
val champion_name : t -> Champion_type.t
```

`champion_name c` is the champion's type. @param c Champion.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.champion_name value1
```

### consumable_count

```ocaml
val consumable_count : t -> Consumable.t -> int
```

`consumable_count c k` is how many of a consumable `c` carries. @param c Champion. @param k Consumable type.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Consumable.t (types from Cambc.Champion)

```ocaml
let result = Champion.consumable_count value1 value2
```

### consumable_counts

```ocaml
val consumable_counts : t -> Consumable_counts.t
```

`consumable_counts c` is `c`'s consumable inventory. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.consumable_counts value1
```

### designs

```ocaml
val designs : unit -> t array
```

Static designs of every champion type, as used during the draft.

Evaluate your champion's options during its turn.

```ocaml
let result = Champion.designs ()
```

### end_turn

```ocaml
val end_turn : t -> unit
```

`end_turn c` ends the turn now. @param c The bot's own champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.end_turn value1
```

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true for the same champion. @param a First champion. @param b Second champion.

### focus

```ocaml
val focus : t -> float
```

`focus c` is current focus; raises if `c` is not visible. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.focus value1
```

### gem_count

```ocaml
val gem_count : t -> int
```

`gem_count c` is the gems `c` is carrying. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.gem_count value1
```

### gold_count

```ocaml
val gold_count : t -> int
```

`gold_count c` is the gold `c` is carrying. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.gold_count value1
```

### has_completed_turn

```ocaml
val has_completed_turn : t -> bool
```

Check whether this champion has completed its turn. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let has_completed_turn = Champion.has_completed_turn value1
```

### hash

```ocaml
val hash : t -> int
```

Hash consistent with `equal`. @param c Champion.

### health

```ocaml
val health : t -> float
```

`health c` is current health; raises if `c` is not visible. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Recall when badly injured and safe from interruption.

Inputs:
- safe_to_recall: your bot's assessment that recalling here is safe and legal

```ocaml
let retreat_if_needed safe_to_recall =
  let me = self_champion () in
  if safe_to_recall && Champion.health me < Champion.max_health me *. 0.3 then
    Champion.recall me
```

### is_alive

```ocaml
val is_alive : t -> bool
```

`is_alive c` is true while the champion is alive. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let is_alive = Champion.is_alive value1
```

### is_ally

```ocaml
val is_ally : t -> bool
```

Check whether this champion belongs to the observing team. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let is_ally = Champion.is_ally value1
```

### is_dead

```ocaml
val is_dead : t -> bool
```

`is_dead c` is `not (is_alive c)`. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let is_dead = Champion.is_dead value1
```

### is_enemy

```ocaml
val is_enemy : t -> bool
```

Check whether this champion belongs to the opposing team. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let is_enemy = Champion.is_enemy value1
```

### is_recalling

```ocaml
val is_recalling : t -> bool
```

Check whether this champion is recalling. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let is_recalling = Champion.is_recalling value1
```

### is_submitting_gems

```ocaml
val is_submitting_gems : t -> bool
```

Check whether this champion is submitting gems. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let is_submitting_gems = Champion.is_submitting_gems value1
```

### is_teleporting

```ocaml
val is_teleporting : t -> bool
```

Check whether this champion is teleporting. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let is_teleporting = Champion.is_teleporting value1
```

### is_visible

```ocaml
val is_visible : t -> bool
```

`is_visible c` is true when the champion is currently visible. @param c Champion.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let is_visible = Champion.is_visible value1
```

### item_count

```ocaml
val item_count : t -> Item.t -> int
```

`item_count c item` is how many of `item` `c` holds. @param c Champion. @param item Item type.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Item.t (types from Cambc.Champion)

```ocaml
let result = Champion.item_count value1 value2
```

### item_counts

```ocaml
val item_counts : t -> Item_counts.t
```

`item_counts c` is `c`'s item inventory. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.item_counts value1
```

### max_health

```ocaml
val max_health : t -> float
```

`max_health c` is current maximum health; raises if `c` is not visible. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Recall when badly injured and safe from interruption.

Inputs:
- safe_to_recall: your bot's assessment that recalling here is safe and legal

```ocaml
let retreat_if_needed safe_to_recall =
  let me = self_champion () in
  if safe_to_recall && Champion.health me < Champion.max_health me *. 0.3 then
    Champion.recall me
```

### move

```ocaml
val move : t -> Vec2.t -> bool
```

Returns false for a newly revealed obstruction; known invalid destinations raise `Cambc_error`. @param c The bot's own champion. @param delta Displacement from the current position.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Take a planned step; false means no movement occurred.

Inputs:
- direction: a legal unit direction, such as east

```ocaml
let me = self_champion () in
if Champion.can_move me then
  let moved = Champion.move me direction in
  ignore moved
```

### move_absolute

```ocaml
val move_absolute : t -> Vec2.t -> bool
```

Returns false for a newly revealed obstruction; known invalid destinations raise `Cambc_error`. @param c The bot's own champion. @param destination Map tile to move to.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Take a planned step; false means no movement occurred.

Inputs:
- destination: a destination selected by your pathfinder

```ocaml
let me = self_champion () in
if Champion.can_move me then
  let moved = Champion.move_absolute me destination in
  ignore moved
```

### move_towards

```ocaml
val move_towards : t -> Vec2.t -> bool
```

Move to the reachable tile on a shortest path to `target`, avoiding walls, holes, altars, the enemy spawn and visible units. Returns false without moving when already there, unable to move, or `target` is blocked or unreachable, and for a newly revealed obstruction. @param c The bot's own champion. @param target Map tile to approach.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Take a planned step; false means no movement occurred.

Inputs:
- destination: a destination selected by your pathfinder

```ocaml
let me = self_champion () in
if Champion.can_move me then
  let moved = Champion.move_towards me destination in
  ignore moved
```

### passive

```ocaml
val passive : t -> Passive.t
```

`passive c` is the champion's passive. @param c Champion.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.passive value1
```

### place_ward

```ocaml
val place_ward : t -> Vec2.t -> bool
```

Returns false for a newly revealed obstruction; known invalid destinations raise `Cambc_error`. @param c The bot's own champion. @param destination Tile to place the ward on.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Vec2.t (types from Cambc.Champion)

```ocaml
let result = Champion.place_ward value1 value2
```

### position

```ocaml
val position : t -> Vec2.t
```

`position c` is the current tile; raises if `c` is not visible. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.position value1
```

### prev_priority

```ocaml
val prev_priority : t -> int
```

`prev_priority c` is the priority used for the current round. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.prev_priority value1
```

### priority

```ocaml
val priority : t -> int
```

`priority c` is the priority that will apply from the next round. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.priority value1
```

### recall

```ocaml
val recall : t -> unit
```

`recall c` starts channelling a recall to the team spawn. @param c The bot's own champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Recall when badly injured and safe from interruption.

Inputs:
- safe_to_recall: your bot's assessment that recalling here is safe and legal

```ocaml
let retreat_if_needed safe_to_recall =
  let me = self_champion () in
  if safe_to_recall && Champion.health me < Champion.max_health me *. 0.3 then
    Champion.recall me
```

### remaining_channel_time

```ocaml
val remaining_channel_time : t -> int
```

`remaining_channel_time c` is the rounds left on the current recall, tribute or teleport channel. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.remaining_channel_time value1
```

### restrictions

```ocaml
val restrictions : t -> Buff_restrictions.t
```

`restrictions c` is the actions the champion's buffs currently prohibit. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.restrictions value1
```

### sell

```ocaml
val sell : ?count:int -> t -> Item.t -> unit
```

`sell ?count c item` sells items; the engine validates inventory and location. @param count Number of copies; defaults to 1. @param c The bot's own champion. @param item Item type.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Item.t (types from Cambc.Champion)

```ocaml
let result = Champion.sell value1 value2
```

### set_priority

```ocaml
val set_priority : t -> int -> unit
```

`set_priority c priority` sets the scheduling priority. @param c The bot's own champion. @param priority New priority; takes effect from the next round and persists.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: int (types from Cambc.Champion)

```ocaml
let result = Champion.set_priority value1 value2
```

### state

```ocaml
val state : t -> Champion_state.t
```

`state c` is the champion's life, channel and action flags. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.state value1
```

### strength

```ocaml
val strength : t -> float
```

`strength c` is current strength; raises if `c` is not visible. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.strength value1
```

### submit_gems

```ocaml
val submit_gems : t -> Vec2.t -> unit
```

`submit_gems c altar` starts tributing carried gems at an altar. @param c The bot's own champion. @param altar Altar tile to tribute at.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Vec2.t (types from Cambc.Champion)

```ocaml
let result = Champion.submit_gems value1 value2
```

### t

```ocaml
type t = champion
```



Public SDK type declaration.

```ocaml
type t = champion
```

### team

```ocaml
val team : t -> Team.t
```

`team c` is the champion's team. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.team value1
```

### use_consumable

```ocaml
val use_consumable : ?at:Vec2.t -> t -> Consumable.t -> bool
```

Returns false for a newly revealed obstruction; known invalid destinations raise `Cambc_error`. @param at Tile to use the consumable at, for targeted consumables. @param c The bot's own champion. @param k Consumable to use.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Consumable.t (types from Cambc.Champion)

```ocaml
let result = Champion.use_consumable value1 value2
```

### visible_tiles

```ocaml
val visible_tiles : t -> Bitboard.t
```

`visible_tiles c` is the tiles `c` currently sees. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.visible_tiles value1
```

### visible_tiles_from

```ocaml
val visible_tiles_from : Vec2.t -> t -> Bitboard.t
```

`visible_tiles_from p c` is the vision `c` would have at `p`. @param p Hypothetical tile to see from. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: Vec2.t (types from Cambc.Champion)
- value2: t (types from Cambc.Champion)

```ocaml
let result = Champion.visible_tiles_from value1 value2
```

### visible_tiles_from_into

```ocaml
val visible_tiles_from_into : Vec2.t -> t -> Bitboard.t -> unit
```

Like `visible_tiles_from`, but writes into `out`. @param p Hypothetical tile to see from. @param c Champion. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: Vec2.t (types from Cambc.Champion)
- value2: t (types from Cambc.Champion)
- value3: Bitboard.t (types from Cambc.Champion)

```ocaml
let result = Champion.visible_tiles_from_into value1 value2 value3
```

### visible_tiles_into

```ocaml
val visible_tiles_into : t -> Bitboard.t -> unit
```

Like `visible_tiles`, but writes into `out`. @param c Champion. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Bitboard.t (types from Cambc.Champion)

```ocaml
let result = Champion.visible_tiles_into value1 value2
```

### ward_cooldown

```ocaml
val ward_cooldown : t -> int
```

`ward_cooldown c` is the rounds until `c` can place another ward. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.ward_cooldown value1
```

### ward_placement_tiles

```ocaml
val ward_placement_tiles : t -> Bitboard.t
```

`ward_placement_tiles c` is the tiles where `c` could place a ward now. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.ward_placement_tiles value1
```

### ward_placement_tiles_into

```ocaml
val ward_placement_tiles_into : t -> Bitboard.t -> unit
```

Like `ward_placement_tiles`, but writes into `out`. @param c Champion. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)
- value2: Bitboard.t (types from Cambc.Champion)

```ocaml
let result = Champion.ward_placement_tiles_into value1 value2
```

### ward_positions

```ocaml
val ward_positions : t -> Vec2.t list
```

`ward_positions c` is the tiles of `c`'s placed wards. @param c Champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Champion)

```ocaml
let result = Champion.ward_positions value1
```

## Cambc.Champion_draft

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Choose your team's roster during the draft phase.

```ocaml
let result = Champion_draft.all
```

### t

```ocaml
type t = | Ryth | Vesper_vale | Vendetta | Cassi_and_issac | Stellar | Prism | Marcella | Willow | Ren | Varg | Tavi
```



Public SDK type declaration.

```ocaml
type t = | Ryth | Vesper_vale | Vendetta | Cassi_and_issac | Stellar | Prism | Marcella | Willow | Ren | Varg | Tavi
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Champion_state

### add

```ocaml
val add : flag -> t -> t
```

`add f s` is `s` with `f` set. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Champion_state)
- value2: t (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.add value1 value2
```

### all

```ocaml
val all : flag list
```

Every named flag.

Combine or inspect flags before deciding which actions or targets are allowed.

```ocaml
let result = Champion_state.all
```

### diff

```ocaml
val diff : t -> t -> t
```

`diff a b` has the flags of `a` not in `b`. @param a First set. @param b Flags to remove.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Champion_state)
- value2: t (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.diff value1 value2
```

### empty

```ocaml
val empty : t
```

The set with no flags.

Combine or inspect flags before deciding which actions or targets are allowed.

```ocaml
let result = Champion_state.empty
```

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true for identical sets. @param a First set. @param b Second set.

### flag

```ocaml
type flag = | Alive | Recalling | Submitting_gems | Can_move | Can_attack | Completed_turn | Recall_started | Submit_gems_started | Recall_cancelled | Submit_gems_cancelled | Can_cast | Teleporting
```



Public SDK type declaration.

```ocaml
type flag = | Alive | Recalling | Submitting_gems | Can_move | Can_attack | Completed_turn | Recall_started | Submit_gems_started | Recall_cancelled | Submit_gems_cancelled | Can_cast | Teleporting
```

### hash

```ocaml
val hash : t -> int
```

Hash consistent with `equal`. @param s Flag set.

### inter

```ocaml
val inter : t -> t -> t
```

`inter a b` has the flags of both sets. @param a First set. @param b Second set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Champion_state)
- value2: t (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.inter value1 value2
```

### is_dead

```ocaml
val is_dead : t -> bool
```

`is_dead s` is true when the alive flag is clear. @param s Champion state.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Champion_state)

```ocaml
let is_dead = Champion_state.is_dead value1
```

### is_empty

```ocaml
val is_empty : t -> bool
```

`is_empty s` is true when no flag is set. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Champion_state)

```ocaml
let is_empty = Champion_state.is_empty value1
```

### mem

```ocaml
val mem : flag -> t -> bool
```

`mem f s` is true when `s` contains `f`. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Champion_state)
- value2: t (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.mem value1 value2
```

### of_list

```ocaml
val of_list : flag list -> t
```

`of_list fs` is the set of the listed flags. @param fs Flags to set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag list (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.of_list value1
```

### remove

```ocaml
val remove : flag -> t -> t
```

`remove f s` is `s` with `f` cleared. @param f Flag. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: flag (types from Cambc.Champion_state)
- value2: t (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.remove value1 value2
```

### subset

```ocaml
val subset : t -> t -> bool
```

`subset a b` is true when every flag of `a` is in `b`. @param a Candidate subset. @param b Superset.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Champion_state)
- value2: t (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.subset value1 value2
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

### to_list

```ocaml
val to_list : t -> flag list
```

`to_list s` lists the set flags. @param s Flag set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.to_list value1
```

### union

```ocaml
val union : t -> t -> t
```

`union a b` has the flags of either set. @param a First set. @param b Second set.

Combine or inspect flags before deciding which actions or targets are allowed.

Inputs:
- value1: t (types from Cambc.Champion_state)
- value2: t (types from Cambc.Champion_state)

```ocaml
let result = Champion_state.union value1 value2
```

## Cambc.Champion_type

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Evaluate your champion's options during its turn.

```ocaml
let result = Champion_type.all
```

### t

```ocaml
type t = | Ryth | Vesper_vale | Vendetta | Cassi | Issac | Stellar | Prism | Marcella | Willow | Ren | Varg | Tavi
```



Public SDK type declaration.

```ocaml
type t = | Ryth | Vesper_vale | Vendetta | Cassi | Issac | Stellar | Prism | Marcella | Willow | Ren | Varg | Tavi
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Constants

### get

```ocaml
val get : unit -> t
```

Game constants loaded at startup.

Use this value in your bot's draft or turn logic.

```ocaml
let result = Constants.get ()
```

### t

```ocaml
type t = { capture_time : int; camp_sleep_time : int; respawn_time : int; recall_time : int; submit_gem_duration_multiplier : float; submit_gem_base_duration : int; submit_gem_minimum_duration : int; submit_gems_range : int; max_rounds : int; winning_gems : int; kill_gold : int; assist_gold : int; passive_gold : int; spawn_healing : float; active_wards : int; shared_memory_bytes : int; draft_memory_bytes : int; tile_reveal_rounds : int; ward_cooldown : int; ward_lifetime : int; ward_vision_radius : float; ward_placement_radius : float; altar_lock_hits : int; tp_crystal_time : int; }
```



Public SDK type declaration.

```ocaml
type t = { capture_time : int; camp_sleep_time : int; respawn_time : int; recall_time : int; submit_gem_duration_multiplier : float; submit_gem_base_duration : int; submit_gem_minimum_duration : int; submit_gems_range : int; max_rounds : int; winning_gems : int; kill_gold : int; assist_gold : int; passive_gold : int; spawn_healing : float; active_wards : int; shared_memory_bytes : int; draft_memory_bytes : int; tile_reveal_rounds : int; ward_cooldown : int; ward_lifetime : int; ward_vision_radius : float; ward_placement_radius : float; altar_lock_hits : int; tp_crystal_time : int; }
```

## Cambc.Constants.Passive_abilities

### prism_focus_per_gem

```ocaml
val prism_focus_per_gem : float
```

Focus Prism gains per carried gem.

Use this value in your bot's draft or turn logic.

```ocaml
let result = Constants.Passive_abilities.prism_focus_per_gem
```

### ren_retaliation_focus_ratio

```ocaml
val ren_retaliation_focus_ratio : float
```

Fraction of focus Ren deals back as retaliation damage.

Use this value in your bot's draft or turn logic.

```ocaml
let result = Constants.Passive_abilities.ren_retaliation_focus_ratio
```

### ryth_health_strength_ratio

```ocaml
val ryth_health_strength_ratio : float
```

Strength Ryth gains per point of bonus health.

Use this value in your bot's draft or turn logic.

```ocaml
let result = Constants.Passive_abilities.ryth_health_strength_ratio
```

### varg_splash_radius

```ocaml
val varg_splash_radius : float
```

Radius of Varg's splash damage, in tiles.

Use this value in your bot's draft or turn logic.

```ocaml
let result = Constants.Passive_abilities.varg_splash_radius
```

### vesper_vale_bonus_gold_percent

```ocaml
val vesper_vale_bonus_gold_percent : int
```

Extra gold Vesper Vale gains, in percent.

Use this value in your bot's draft or turn logic.

```ocaml
let result = Constants.Passive_abilities.vesper_vale_bonus_gold_percent
```

## Cambc.Consumable

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Plan a purchase or use an owned consumable.

```ocaml
let result = Consumable.all
```

### carry_limit

```ocaml
val carry_limit : t -> int
```

`carry_limit c` is how many a champion can carry. @param c Consumable type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: t (types from Cambc.Consumable)

```ocaml
let result = Consumable.carry_limit value1
```

### cost

```ocaml
val cost : t -> int
```

`cost c` is the gold cost of one. @param c Consumable type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: t (types from Cambc.Consumable)

```ocaml
let result = Consumable.cost value1
```

### info

```ocaml
type info = { cost : int; carry_limit : int }
```



Public SDK type declaration.

```ocaml
type info = { cost : int; carry_limit : int }
```

### info

```ocaml
val info : t -> info
```

`info c` is the consumable's static design data. @param c Consumable type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: t (types from Cambc.Consumable)

```ocaml
let result = Consumable.info value1
```

### t

```ocaml
type t = Tp_crystal | Altar_lock
```



Public SDK type declaration.

```ocaml
type t = Tp_crystal | Altar_lock
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Consumable_counts

### fold

```ocaml
val fold : (Consumable.t -> int -> 'a -> 'a) -> t -> 'a -> 'a
```

`fold fn c init` folds `fn` over each held type and its count. @param fn Folding function. @param c Inventory counts. @param init Initial accumulator.

Plan a purchase or use an owned consumable.

Inputs:
- value1: (Consumable.t -> int -> 'a -> 'a) (types from Cambc.Consumable_counts)
- value2: t (types from Cambc.Consumable_counts)
- value3: 'a (types from Cambc.Consumable_counts)

```ocaml
let result = Consumable_counts.fold value1 value2 value3
```

### get

```ocaml
val get : t -> Consumable.t -> int
```

`get c k` is the number of `k` in the inventory. @param c Inventory counts. @param k Consumable type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: t (types from Cambc.Consumable_counts)
- value2: Consumable.t (types from Cambc.Consumable_counts)

```ocaml
let result = Consumable_counts.get value1 value2
```

### iter

```ocaml
val iter : (Consumable.t -> int -> unit) -> t -> unit
```

`iter fn c` calls `fn` on each held type and its count. @param fn Callback. @param c Inventory counts.

Plan a purchase or use an owned consumable.

Inputs:
- value1: (Consumable.t -> int -> unit) (types from Cambc.Consumable_counts)
- value2: t (types from Cambc.Consumable_counts)

```ocaml
let result = Consumable_counts.iter value1 value2
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

### to_list

```ocaml
val to_list : t -> (Consumable.t * int) list
```

`to_list c` lists each held type with its count. @param c Inventory counts.

Plan a purchase or use an owned consumable.

Inputs:
- value1: t (types from Cambc.Consumable_counts)

```ocaml
let result = Consumable_counts.to_list value1
```

## Cambc.Debug

### draw_dot

```ocaml
val draw_dot : Vec2.t -> int -> unit
```

`draw_dot p rgb` draws a debug dot in the replay. @param p Tile to mark. @param rgb Packed `0xRRGGBB` colour.

Mark a decision in your replay debug overlay to understand the bot's route or target.

Inputs:
- value1: Vec2.t (types from Cambc.Debug)
- value2: int (types from Cambc.Debug)

```ocaml
let result = Debug.draw_dot value1 value2
```

### draw_line

```ocaml
val draw_line : Vec2.t -> Vec2.t -> int -> unit
```

`draw_line a b rgb` draws a debug line in the replay. @param a Start tile. @param b End tile. @param rgb Packed `0xRRGGBB` colour.

Mark a decision in your replay debug overlay to understand the bot's route or target.

Inputs:
- value1: Vec2.t (types from Cambc.Debug)
- value2: Vec2.t (types from Cambc.Debug)
- value3: int (types from Cambc.Debug)

```ocaml
let result = Debug.draw_line value1 value2 value3
```

## Cambc.Distance_field

### compute

```ocaml
val compute : ?destinations:Bitboard.t -> t -> walls:Bitboard.t -> roots:Bitboard.t -> visited:Bitboard.t -> max_distance:int -> unit
```

BFS from `roots`; with `?destinations` stops once all are reached.

Find a step towards the goal tiles. The result is a direction for move.

Inputs:
- blocked: impassable/occupied tiles, excluding your own tile
- goals: legal destination tiles

```ocaml
let field = Distance_field.create () in
let visited = Bitboard.create () in
Distance_field.compute field ~walls:blocked ~roots:goals ~visited
  ~max_distance:Distance_field.unreachable;
let direction = Distance_field.next_move_from field (Champion.position (self_champion ())) in
direction
```

### compute_with_expansion_count

```ocaml
val compute_with_expansion_count : ?destinations:Bitboard.t -> t -> walls:Bitboard.t -> roots:Bitboard.t -> visited:Bitboard.t -> max_distance:int -> int
```

Like `compute`, but returns the number of expansion steps. @param destinations Optional target tiles; stop once all are reached. @param walls Impassable tiles. @param roots Tiles at distance zero. @param visited Receives the tiles the search reached. @param max_distance Tiles farther than this are left unreached.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)
- walls: Bitboard.t (types from Cambc.Distance_field)
- roots: Bitboard.t (types from Cambc.Distance_field)
- visited: Bitboard.t (types from Cambc.Distance_field)
- max_distance: int (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.compute_with_expansion_count value1 ~walls ~roots ~visited ~max_distance
```

### copy

```ocaml
val copy : t -> t
```

`copy d` is an independent copy. @param d Distance field.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.copy value1
```

### create

```ocaml
val create : unit -> t
```

A field with every tile unreachable.

Choose a route around blocked tiles.

```ocaml
let result = Distance_field.create ()
```

### export_linear_buffer

```ocaml
val export_linear_buffer : t -> int array
```

`export_linear_buffer d` is the row-major distances. @param d Distance field.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.export_linear_buffer value1
```

### export_linear_buffer_into

```ocaml
val export_linear_buffer_into : t -> int array -> unit
```

Like `export_linear_buffer`, but writes into `out`. @param d Distance field. @param out Destination with one entry per tile.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)
- value2: int array (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.export_linear_buffer_into value1 value2
```

### export_raw_buffer

```ocaml
val export_raw_buffer : t -> Bytes.t
```

`export_raw_buffer d` is the contents as an engine-chunked buffer. @param d Distance field.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.export_raw_buffer value1
```

### export_raw_buffer_into

```ocaml
val export_raw_buffer_into : t -> Bytes.t -> unit
```

Like `export_raw_buffer`, but writes into `out`. @param d Distance field. @param out Destination, exactly `raw_buffer_size` long.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)
- value2: Bytes.t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.export_raw_buffer_into value1 value2
```

### get

```ocaml
val get : t -> Vec2.t -> int
```

`get d p` is the distance at `p`; raises outside the map. @param d Distance field. @param p Tile.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)
- value2: Vec2.t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.get value1 value2
```

### height

```ocaml
val height : t -> int
```

Map height in tiles. @param d Distance field.

### import_linear_buffer

```ocaml
val import_linear_buffer : t -> int array -> unit
```

`import_linear_buffer d values` replaces the contents from row-major distances. @param d Distance field. @param values One distance per tile in row-major order.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)
- value2: int array (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.import_linear_buffer value1 value2
```

### import_raw_buffer

```ocaml
val import_raw_buffer : t -> Bytes.t -> unit
```

`import_raw_buffer d data` replaces the contents from an engine-chunked buffer. @param d Distance field. @param data Engine-chunked bytes, exactly `raw_buffer_size` long.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)
- value2: Bytes.t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.import_raw_buffer value1 value2
```

### linear_buffer_size

```ocaml
val linear_buffer_size : t -> int
```

Number of row-major values. @param d Distance field.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.linear_buffer_size value1
```

### next_move_from

```ocaml
val next_move_from : t -> Vec2.t -> Vec2.t option
```

`next_move_from d p` is the first optimal step towards a root as a direction delta, or `None` at a root or unreached tile. @param d Distance field. @param p Tile to step from. First optimal step from a tile towards a root, as a direction delta in compass order from north; `None` at a root or unreached tile.

Find a step towards the goal tiles. The result is a direction for move.

Inputs:
- blocked: impassable/occupied tiles, excluding your own tile
- goals: legal destination tiles

```ocaml
let field = Distance_field.create () in
let visited = Bitboard.create () in
Distance_field.compute field ~walls:blocked ~roots:goals ~visited
  ~max_distance:Distance_field.unreachable;
let direction = Distance_field.next_move_from field (Champion.position (self_champion ())) in
direction
```

### next_moves_from

```ocaml
val next_moves_from : t -> Vec2.t -> Vec2.t list
```

Every optimal step from a tile towards a root, as direction deltas in compass order.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)
- value2: Vec2.t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.next_moves_from value1 value2
```

### print

```ocaml
val print : t -> unit
```

`print d` prints the field to stdout for debugging. @param d Distance field.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.print value1
```

### raw_buffer_size

```ocaml
val raw_buffer_size : t -> int
```

Size in bytes of the engine-chunked buffer. @param d Distance field.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.raw_buffer_size value1
```

### set

```ocaml
val set : t -> Vec2.t -> int -> unit
```

`set d p value` stores a distance at `p`; raises outside the map. @param d Distance field. @param p Tile. @param value Distance to store.

Choose a route around blocked tiles.

Inputs:
- value1: t (types from Cambc.Distance_field)
- value2: Vec2.t (types from Cambc.Distance_field)
- value3: int (types from Cambc.Distance_field)

```ocaml
let result = Distance_field.set value1 value2 value3
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

### unreachable

```ocaml
val unreachable : int
```

Distance stored for tiles the search did not reach.

Choose a route around blocked tiles.

```ocaml
let result = Distance_field.unreachable
```

### width

```ocaml
val width : t -> int
```

Map width in tiles. @param d Distance field.

## Cambc.Draft

### current

```ocaml
val current : Team.t -> Champion_draft.t list
```

`current team` is the team's picks so far, in order. @param team Team whose picks to return.

Choose your team's roster during the draft phase.

Inputs:
- value1: Team.t (types from Cambc.Draft)

```ocaml
let result = Draft.current value1
```

### pick

```ocaml
val pick : Champion_draft.t -> unit
```

`pick champion` picks a champion for the current draft round; only valid during the draft. @param champion Champion to pick.

**draft**: This function can only be used during draft phase, and not by bots during gameplay

Choose your team's roster during the draft phase.

Inputs:
- value1: Champion_draft.t (types from Cambc.Draft)

```ocaml
let result = Draft.pick value1
```

### team_size

```ocaml
val team_size : unit -> int
```

Number of champions each team drafts.

Choose your team's roster during the draft phase.

```ocaml
let result = Draft.team_size ()
```

## Cambc.Draft_memory

### bytes

```ocaml
val bytes : unit -> Bytes.t
```

Copy of the draft memory snapshot.

Pass your drafting decisions to the bot's later turns using draft memory.

```ocaml
let result = Draft_memory.bytes ()
```

### get

```ocaml
val get : int -> int
```

`get offset` reads one byte of the snapshot. @param offset Byte offset into the snapshot.

Pass your drafting decisions to the bot's later turns using draft memory.

Inputs:
- value1: int (types from Cambc.Draft_memory)

```ocaml
let result = Draft_memory.get value1
```

### length

```ocaml
val length : unit -> int
```

Length of the draft memory snapshot in bytes.

### submit

```ocaml
val submit : Bytes.t -> unit
```

`submit data` sets the draft memory passed to this team's gameplay bots; only valid during the draft. @param data Bytes to submit.

**draft**: This function can only be used during draft phase, and not by bots during gameplay

Pass your drafting decisions to the bot's later turns using draft memory.

Inputs:
- value1: Bytes.t (types from Cambc.Draft_memory)

```ocaml
let result = Draft_memory.submit value1
```

## Cambc.Error_code

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Inspect a caught SDK error when diagnosing a rejected action.

```ocaml
let result = Error_code.all
```

### t

```ocaml
type t = | Success | Invalid_handle | Out_of_memory | File_not_found | Null_pointer | Invalid_argument | Map_parse_error | Invalid_map_file | Invalid_map_version | Replay_parse_error | Invalid_replay_file | Invalid_replay_version | Invalid_draft | Draft_not_drafting | Draft_duplicate_champion | Draft_invalid_round | Draft_unavailable | Already_initialized | Invalid_action | Invalid_action_memory_bad_range | Invalid_action_memory_draft_frozen | Invalid_action_move_cant_move | Invalid_action_move_bad_destination | Invalid_action_spell_cast_invalid_source | Invalid_action_spell_cast_invalid_slot | Invalid_action_spell_cast_no_ability | Invalid_action_spell_cast_cant_cast | Invalid_action_spell_cast_already_cast | Invalid_action_spell_cast_on_cooldown | Invalid_action_spell_cast_no_charges | Invalid_action_spell_cast_bad_target | Invalid_action_spell_cast_out_of_range | Invalid_action_spell_cast_bad_destination | Invalid_action_cant_act | Invalid_action_is_recalling | Invalid_action_is_submitting_gems | Invalid_action_move_already_moved | Invalid_action_move_out_of_range | Invalid_action_move_unreachable | Invalid_action_attack_invalid_source | Invalid_action_attack_cant_attack | Invalid_action_attack_already_attacked | Invalid_action_attack_bad_target | Invalid_action_attack_out_of_range | Invalid_action_recall_cant_channel | Invalid_action_submit_gems_cant_channel | Invalid_action_submit_gems_no_gems | Invalid_action_submit_gems_bad_altar | Invalid_action_submit_gems_out_of_range | Invalid_action_buy_item_not_on_spawn | Invalid_action_buy_item_invalid_item | Invalid_action_buy_item_invalid_count | Invalid_action_buy_item_not_enough_gold | Invalid_action_buy_item_inventory_full | Invalid_action_buy_item_stats_overflow | Invalid_action_buy_consumable_not_on_spawn | Invalid_action_buy_consumable_invalid_type | Invalid_action_buy_consumable_invalid_count | Invalid_action_buy_consumable_inventory_full | Invalid_action_buy_consumable_not_enough_gold | Invalid_action_use_consumable_invalid_type | Invalid_action_use_consumable_not_owned | Invalid_action_place_ward_no_charges | Invalid_action_place_ward_cant_place | Invalid_action_place_ward_bad_destination | Invalid_action_place_ward_out_of_range | Map_set_team_aspawn_area_bad_position | Map_add_bush_bad_position | Map_remove_bush_bad_position | Map_add_wall_bad_position | Map_remove_wall_bad_position | Map_add_hole_bad_position | Map_remove_hole_bad_position | Map_add_altar_bad_position | Map_remove_altar_bad_position | Map_get_bush_bad_position | Map_get_wall_bad_position | Map_get_hole_bad_position | Map_get_altar_bad_position | Map_get_camp_handle_at_bad_position | Map_add_camp_bad_position | Map_set_camp_bad_position | Invalid_action_champion_teleport_bad_destination | Invalid_action_respawn_group_blocked_spawn | Invalid_action_camp_teleport_bad_destination | Timeline_bad_seek_position | Timeline_bad_seek_substep | Api_no_map | Managed_resources_bad_position | Managed_resources_bad_radius | Managed_resources_aliasing | Managed_resources_metadata_mismatch | Managed_resources_opaque_origin | Api_invalid_team | Api_invalid_perspective | Api_not_initialized | Api_bad_position | Api_not_in_vision | Api_not_found | Api_invalid_camp_group | Api_invalid_camp_info | Api_invalid_camp | Api_invalid_champion | Api_invalid_ward | Api_invalid_ability | Api_not_teammate | Api_camp_group_not_waiting_to_respawn | Api_champion_alive | Api_invalid_champion_type | Api_invalid_camp_type | Api_value_out_of_range | Api_buffer_too_small | Api_not_owner | Invalid_action_recall_already_used | Invalid_action_submit_gems_already_used | Invalid_action_cancel_channel_not_channeling | Invalid_action_cancel_channel_already_used | Invalid_action_place_ward_ally_ward_present | Invalid_action_altar_lock_bad_altar | Invalid_action_altar_lock_out_of_range | Invalid_action_altar_lock_already_locked | Invalid_action_is_teleporting | Invalid_action_cancel_channel_not_cancellable | Invalid_action_submit_gems_altar_locked | Invalid_action_spell_cast_already_moved | Invalid_action_move_exclusive_spell_cast | Invalid_action_sell_not_tavi | Invalid_action_sell_invalid_item | Invalid_action_sell_not_owned | Invalid_action_sell_already_sold | Invalid_action_sell_gold_overflow | Invalid_action_sell_not_on_spawn | Draft_already_picked | Draft_no_champion_selected | Unknown of int
```



Public SDK type declaration.

```ocaml
type t = | Success | Invalid_handle | Out_of_memory | File_not_found | Null_pointer | Invalid_argument | Map_parse_error | Invalid_map_file | Invalid_map_version | Replay_parse_error | Invalid_replay_file | Invalid_replay_version | Invalid_draft | Draft_not_drafting | Draft_duplicate_champion | Draft_invalid_round | Draft_unavailable | Already_initialized | Invalid_action | Invalid_action_memory_bad_range | Invalid_action_memory_draft_frozen | Invalid_action_move_cant_move | Invalid_action_move_bad_destination | Invalid_action_spell_cast_invalid_source | Invalid_action_spell_cast_invalid_slot | Invalid_action_spell_cast_no_ability | Invalid_action_spell_cast_cant_cast | Invalid_action_spell_cast_already_cast | Invalid_action_spell_cast_on_cooldown | Invalid_action_spell_cast_no_charges | Invalid_action_spell_cast_bad_target | Invalid_action_spell_cast_out_of_range | Invalid_action_spell_cast_bad_destination | Invalid_action_cant_act | Invalid_action_is_recalling | Invalid_action_is_submitting_gems | Invalid_action_move_already_moved | Invalid_action_move_out_of_range | Invalid_action_move_unreachable | Invalid_action_attack_invalid_source | Invalid_action_attack_cant_attack | Invalid_action_attack_already_attacked | Invalid_action_attack_bad_target | Invalid_action_attack_out_of_range | Invalid_action_recall_cant_channel | Invalid_action_submit_gems_cant_channel | Invalid_action_submit_gems_no_gems | Invalid_action_submit_gems_bad_altar | Invalid_action_submit_gems_out_of_range | Invalid_action_buy_item_not_on_spawn | Invalid_action_buy_item_invalid_item | Invalid_action_buy_item_invalid_count | Invalid_action_buy_item_not_enough_gold | Invalid_action_buy_item_inventory_full | Invalid_action_buy_item_stats_overflow | Invalid_action_buy_consumable_not_on_spawn | Invalid_action_buy_consumable_invalid_type | Invalid_action_buy_consumable_invalid_count | Invalid_action_buy_consumable_inventory_full | Invalid_action_buy_consumable_not_enough_gold | Invalid_action_use_consumable_invalid_type | Invalid_action_use_consumable_not_owned | Invalid_action_place_ward_no_charges | Invalid_action_place_ward_cant_place | Invalid_action_place_ward_bad_destination | Invalid_action_place_ward_out_of_range | Map_set_team_aspawn_area_bad_position | Map_add_bush_bad_position | Map_remove_bush_bad_position | Map_add_wall_bad_position | Map_remove_wall_bad_position | Map_add_hole_bad_position | Map_remove_hole_bad_position | Map_add_altar_bad_position | Map_remove_altar_bad_position | Map_get_bush_bad_position | Map_get_wall_bad_position | Map_get_hole_bad_position | Map_get_altar_bad_position | Map_get_camp_handle_at_bad_position | Map_add_camp_bad_position | Map_set_camp_bad_position | Invalid_action_champion_teleport_bad_destination | Invalid_action_respawn_group_blocked_spawn | Invalid_action_camp_teleport_bad_destination | Timeline_bad_seek_position | Timeline_bad_seek_substep | Api_no_map | Managed_resources_bad_position | Managed_resources_bad_radius | Managed_resources_aliasing | Managed_resources_metadata_mismatch | Managed_resources_opaque_origin | Api_invalid_team | Api_invalid_perspective | Api_not_initialized | Api_bad_position | Api_not_in_vision | Api_not_found | Api_invalid_camp_group | Api_invalid_camp_info | Api_invalid_camp | Api_invalid_champion | Api_invalid_ward | Api_invalid_ability | Api_not_teammate | Api_camp_group_not_waiting_to_respawn | Api_champion_alive | Api_invalid_champion_type | Api_invalid_camp_type | Api_value_out_of_range | Api_buffer_too_small | Api_not_owner | Invalid_action_recall_already_used | Invalid_action_submit_gems_already_used | Invalid_action_cancel_channel_not_channeling | Invalid_action_cancel_channel_already_used | Invalid_action_place_ward_ally_ward_present | Invalid_action_altar_lock_bad_altar | Invalid_action_altar_lock_out_of_range | Invalid_action_altar_lock_already_locked | Invalid_action_is_teleporting | Invalid_action_cancel_channel_not_cancellable | Invalid_action_submit_gems_altar_locked | Invalid_action_spell_cast_already_moved | Invalid_action_move_exclusive_spell_cast | Invalid_action_sell_not_tavi | Invalid_action_sell_invalid_item | Invalid_action_sell_not_owned | Invalid_action_sell_already_sold | Invalid_action_sell_gold_overflow | Invalid_action_sell_not_on_spawn | Draft_already_picked | Draft_no_champion_selected | Unknown of int
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Game_map

### altars

```ocaml
val altars : unit -> Bitboard.t
```

Altar tiles, where gems are tributed.

Build a terrain-only walking mask. Also exclude occupied tiles and the enemy spawn when selecting a legal step.

```ocaml
let blocked =
  Bitboard.union (Game_map.walls ())
    (Bitboard.union (Game_map.holes ()) (Game_map.altars ()))
let walkable = Bitboard.complement blocked
```

### bushes

```ocaml
val bushes : unit -> Bitboard.t
```

Bush tiles, which hide champions from outside vision.

Inspect map geometry when choosing a route or objective.

```ocaml
let result = Game_map.bushes ()
```

### camp_at

```ocaml
val camp_at : Vec2.t -> Camp.t
```

`camp_at p` is the camp that spawns at `p`; raises if none does. @param p Spawn tile.

Inspect map geometry when choosing a route or objective.

Inputs:
- value1: Vec2.t (types from Cambc.Game_map)

```ocaml
let result = Game_map.camp_at value1
```

### camp_at_opt

```ocaml
val camp_at_opt : Vec2.t -> Camp.t option
```

Like `camp_at`, but `None` if no camp spawns at `p`. @param p Spawn tile.

Inspect map geometry when choosing a route or objective.

Inputs:
- value1: Vec2.t (types from Cambc.Game_map)

```ocaml
let result = Game_map.camp_at_opt value1
```

### camp_groups

```ocaml
val camp_groups : unit -> Camp_group.t list
```

Every camp group on the map, also available in draft.

Inspect map geometry when choosing a route or objective.

```ocaml
let result = Game_map.camp_groups ()
```

### height

```ocaml
val height : unit -> int
```

Map height in tiles.

### holes

```ocaml
val holes : unit -> Bitboard.t
```

Hole tiles, which block movement but not vision.

Build a terrain-only walking mask. Also exclude occupied tiles and the enemy spawn when selecting a legal step.

```ocaml
let blocked =
  Bitboard.union (Game_map.walls ())
    (Bitboard.union (Game_map.holes ()) (Game_map.altars ()))
let walkable = Bitboard.complement blocked
```

### is_in_bounds

```ocaml
val is_in_bounds : Vec2.t -> bool
```

`is_in_bounds p` is true when `p` lies on the map. @param p Position to check.

Inspect map geometry when choosing a route or objective.

Inputs:
- value1: Vec2.t (types from Cambc.Game_map)

```ocaml
let is_in_bounds = Game_map.is_in_bounds value1
```

### reflect_horizontal

```ocaml
val reflect_horizontal : Vec2.t -> Vec2.t
```

`reflect_horizontal p` reflects `p` horizontally, reversing x. @param p Map position.

Inspect map geometry when choosing a route or objective.

Inputs:
- value1: Vec2.t (types from Cambc.Game_map)

```ocaml
let result = Game_map.reflect_horizontal value1
```

### reflect_vertical

```ocaml
val reflect_vertical : Vec2.t -> Vec2.t
```

`reflect_vertical p` reflects `p` vertically, reversing y. @param p Map position.

Inspect map geometry when choosing a route or objective.

Inputs:
- value1: Vec2.t (types from Cambc.Game_map)

```ocaml
let result = Game_map.reflect_vertical value1
```

### rotate_180

```ocaml
val rotate_180 : Vec2.t -> Vec2.t
```

`rotate_180 p` rotates `p` 180 degrees within the map. @param p Map position.

Inspect map geometry when choosing a route or objective.

Inputs:
- value1: Vec2.t (types from Cambc.Game_map)

```ocaml
let result = Game_map.rotate_180 value1
```

### spawn_region

```ocaml
val spawn_region : Team.t -> Bitboard.t
```

`spawn_region team` is the team's spawn region. @param team Team whose spawn region to return.

Inspect map geometry when choosing a route or objective.

Inputs:
- value1: Team.t (types from Cambc.Game_map)

```ocaml
let result = Game_map.spawn_region value1
```

### symmetry

```ocaml
val symmetry : unit -> Symmetry.t
```

The symmetry the map was generated with.

Inspect map geometry when choosing a route or objective.

```ocaml
let result = Game_map.symmetry ()
```

### walls

```ocaml
val walls : unit -> Bitboard.t
```

Wall tiles. Terrain and spawn boards are shared cached values: copy before mutating.

Build a terrain-only walking mask. Also exclude occupied tiles and the enemy spawn when selecting a legal step.

```ocaml
let blocked =
  Bitboard.union (Game_map.walls ())
    (Bitboard.union (Game_map.holes ()) (Game_map.altars ()))
let walkable = Bitboard.complement blocked
```

### width

```ocaml
val width : unit -> int
```

Map width in tiles.

## Cambc.Game_unit

### attack_range

```ocaml
val attack_range : t -> int
```

`attack_range u` is the attack range in Chebyshev tiles; melee units have range 1. @param u Unit.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.attack_range value1
```

### attack_tiles

```ocaml
val attack_tiles : t -> Bitboard.t
```

`attack_tiles u` is the tiles within attack range of `u`'s position. @param u Unit.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.attack_tiles value1
```

### attack_tiles_into

```ocaml
val attack_tiles_into : t -> Bitboard.t -> unit
```

Like `attack_tiles`, but writes into `out`. @param u Unit. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)
- value2: Bitboard.t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.attack_tiles_into value1 value2
```

### buffs

```ocaml
val buffs : t -> Buff.t list
```

`buffs u` is the current buffs; raises if `u` is not visible. @param u Unit.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.buffs value1
```

### focus

```ocaml
val focus : t -> float
```

`focus u` is current focus; raises if `u` is not visible. @param u Unit.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.focus value1
```

### health

```ocaml
val health : t -> float
```

`health u` is current health; raises if `u` is not visible. @param u Unit.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.health value1
```

### is_visible

```ocaml
val is_visible : t -> bool
```

`is_visible u` is true when `u` is currently visible. @param u Unit.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let is_visible = Game_unit.is_visible value1
```

### max_health

```ocaml
val max_health : t -> float
```

`max_health u` is current maximum health; raises if `u` is not visible. @param u Unit.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.max_health value1
```

### position

```ocaml
val position : t -> Vec2.t
```

`position u` is the current tile; raises if `u` is not visible. @param u Unit.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.position value1
```

### strength

```ocaml
val strength : t -> float
```

`strength u` is current strength; raises if `u` is not visible. @param u Unit.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Evaluate your champion's options during its turn.

Inputs:
- value1: t (types from Cambc.Game_unit)

```ocaml
let result = Game_unit.strength value1
```

### t

```ocaml
type t = [ `Champion of champion | `Camp of camp ]
```



Public SDK type declaration.

```ocaml
type t = [ `Champion of champion | `Camp of camp ]
```

## Cambc.Item

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Plan a purchase or use an owned consumable.

```ocaml
let result = Item.all
```

### t

```ocaml
type t = | Monster_shard_of_strength | Monster_emblem_of_strength | Monster_shard_of_focus | Monster_emblem_of_focus | Monster_shard_of_health | Monster_emblem_of_health | Power_shard_of_strength | Power_emblem_of_strength | Power_shard_of_focus | Power_emblem_of_focus | Power_shard_of_health | Power_emblem_of_health | Durability_shard_of_strength | Durability_emblem_of_strength | Durability_shard_of_focus | Durability_emblem_of_focus | Durability_shard_of_health | Durability_emblem_of_health | Treasure_shard_of_strength | Treasure_emblem_of_strength | Treasure_shard_of_focus | Treasure_emblem_of_focus | Treasure_shard_of_health | Treasure_emblem_of_health | Jungle_shard_of_strength | Jungle_emblem_of_strength | Jungle_shard_of_focus | Jungle_emblem_of_focus | Jungle_shard_of_health | Jungle_emblem_of_health | Home_shard_of_strength | Home_emblem_of_strength | Home_shard_of_focus | Home_emblem_of_focus | Home_shard_of_health | Home_emblem_of_health | Bruiser_shard_of_strength | Bruiser_emblem_of_strength | Bruiser_shard_of_focus | Bruiser_emblem_of_focus | Bruiser_shard_of_health | Bruiser_emblem_of_health
```



Public SDK type declaration.

```ocaml
type t = | Monster_shard_of_strength | Monster_emblem_of_strength | Monster_shard_of_focus | Monster_emblem_of_focus | Monster_shard_of_health | Monster_emblem_of_health | Power_shard_of_strength | Power_emblem_of_strength | Power_shard_of_focus | Power_emblem_of_focus | Power_shard_of_health | Power_emblem_of_health | Durability_shard_of_strength | Durability_emblem_of_strength | Durability_shard_of_focus | Durability_emblem_of_focus | Durability_shard_of_health | Durability_emblem_of_health | Treasure_shard_of_strength | Treasure_emblem_of_strength | Treasure_shard_of_focus | Treasure_emblem_of_focus | Treasure_shard_of_health | Treasure_emblem_of_health | Jungle_shard_of_strength | Jungle_emblem_of_strength | Jungle_shard_of_focus | Jungle_emblem_of_focus | Jungle_shard_of_health | Jungle_emblem_of_health | Home_shard_of_strength | Home_emblem_of_strength | Home_shard_of_focus | Home_emblem_of_focus | Home_shard_of_health | Home_emblem_of_health | Bruiser_shard_of_strength | Bruiser_emblem_of_strength | Bruiser_shard_of_focus | Bruiser_emblem_of_focus | Bruiser_shard_of_health | Bruiser_emblem_of_health
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Item_counts

### fold

```ocaml
val fold : (Item.t -> int -> 'a -> 'a) -> t -> 'a -> 'a
```

`fold fn c init` folds `fn` over each held type and its count. @param fn Folding function. @param c Inventory counts. @param init Initial accumulator.

Plan a purchase or use an owned consumable.

Inputs:
- value1: (Item.t -> int -> 'a -> 'a) (types from Cambc.Item_counts)
- value2: t (types from Cambc.Item_counts)
- value3: 'a (types from Cambc.Item_counts)

```ocaml
let result = Item_counts.fold value1 value2 value3
```

### get

```ocaml
val get : t -> Item.t -> int
```

`get c k` is the number of `k` in the inventory. @param c Inventory counts. @param k Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: t (types from Cambc.Item_counts)
- value2: Item.t (types from Cambc.Item_counts)

```ocaml
let result = Item_counts.get value1 value2
```

### iter

```ocaml
val iter : (Item.t -> int -> unit) -> t -> unit
```

`iter fn c` calls `fn` on each held type and its count. @param fn Callback. @param c Inventory counts.

Plan a purchase or use an owned consumable.

Inputs:
- value1: (Item.t -> int -> unit) (types from Cambc.Item_counts)
- value2: t (types from Cambc.Item_counts)

```ocaml
let result = Item_counts.iter value1 value2
```

### t

```ocaml
type t
```



Public SDK type declaration.

```ocaml
type t
```

### to_list

```ocaml
val to_list : t -> (Item.t * int) list
```

`to_list c` lists each held type with its count. @param c Inventory counts.

Plan a purchase or use an owned consumable.

Inputs:
- value1: t (types from Cambc.Item_counts)

```ocaml
let result = Item_counts.to_list value1
```

## Cambc.Item_info

### combine_count

```ocaml
val combine_count : Item.t -> int
```

`combine_count item` is how many copies combine into the result item. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.combine_count value1
```

### cost

```ocaml
val cost : Item.t -> int
```

`cost item` is the gold cost to buy the item. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.cost value1
```

### focus_points

```ocaml
val focus_points : Item.t -> int
```

Focus the item grants. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.focus_points value1
```

### get

```ocaml
val get : Item.t -> t
```

`get item` is the item's static design data. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.get value1
```

### health_points

```ocaml
val health_points : Item.t -> int
```

Maximum health the item grants. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.health_points value1
```

### is_purchasable

```ocaml
val is_purchasable : Item.t -> bool
```

`is_purchasable item` is true when the shop sells the item directly. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let is_purchasable = Item_info.is_purchasable value1
```

### passive

```ocaml
val passive : Item.t -> Item_passive.t option
```

Passive effect the item grants, if any. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.passive value1
```

### passive_magnitude

```ocaml
val passive_magnitude : Item.t -> float
```

Strength of the item's passive effect. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.passive_magnitude value1
```

### result

```ocaml
val result : Item.t -> Item.t option
```

`result item` is the item these copies combine into, if any. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.result value1
```

### strength_points

```ocaml
val strength_points : Item.t -> int
```

Strength the item grants. @param item Item type.

Plan a purchase or use an owned consumable.

Inputs:
- value1: Item.t (types from Cambc.Item_info)

```ocaml
let result = Item_info.strength_points value1
```

### t

```ocaml
type t = { cost : int; combine_count : int; result : Item.t option; health_points : int; strength_points : int; focus_points : int; passive : Item_passive.t option; passive_magnitude : float; purchasable : bool; }
```



Public SDK type declaration.

```ocaml
type t = { cost : int; combine_count : int; result : Item.t option; health_points : int; strength_points : int; focus_points : int; passive : Item_passive.t option; passive_magnitude : float; purchasable : bool; }
```

## Cambc.Item_passive

### t

```ocaml
type t = Monster | Durability | Treasure | Jungle | Home
```



Public SDK type declaration.

```ocaml
type t = Monster | Durability | Treasure | Jungle | Home
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string p` is the name of an item passive. @param p Item passive.

## Cambc.Passive

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Use this value in your bot's draft or turn logic.

```ocaml
let result = Passive.all
```

### t

```ocaml
type t = | None | Bonus_gold | Remote_shop | Gem_focus | Splash_attack | Health_strength | Reset_cooldowns_on_kill | Retaliation | Stun_immunity
```



Public SDK type declaration.

```ocaml
type t = | None | Bonus_gold | Remote_shop | Gem_focus | Splash_attack | Health_strength | Reset_cooldowns_on_kill | Retaliation | Stun_immunity
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Shared_memory

### get

```ocaml
val get : int -> int
```

`get offset` reads one byte. @param offset Byte offset into shared memory.

Share an objective using your team's agreed memory layout.

Inputs:
- value1: int (types from Cambc.Shared_memory)

```ocaml
let result = Shared_memory.get value1
```

### kind

```ocaml
type _ kind = | U8 : int kind | I8 : int kind | U16 : int kind | I16 : int kind | U32 : int kind | I32 : int kind | I64 : int64 kind | F32 : float kind | F64 : float kind
```

Element types for typed reads/writes (little-endian, like the Rust/C++ SDKs).

Public SDK type declaration.

```ocaml
type _ kind = | U8 : int kind | I8 : int kind | U16 : int kind | I16 : int kind | U32 : int kind | I32 : int kind | I64 : int64 kind | F32 : float kind | F64 : float kind
```

### read

```ocaml
val read : offset:int -> int -> Bytes.t
```

`read ~offset len` reads `len` bytes; raises if the range does not fit. @param offset Byte offset into shared memory. @param len Number of bytes to read.

Reserve byte zero in your team's memory layout for a shared objective ID. All allied bots must agree on its meaning.

```ocaml
let read_objective () =
  let data = Shared_memory.read ~offset:0 1 in
  Char.code (Bytes.get data 0)
```

### read_into

```ocaml
val read_into : offset:int -> Bytes.t -> unit
```

`read_into ~offset out` fills `out` from shared memory. @param offset Byte offset into shared memory. @param out Destination; its length sets how many bytes are read.

Share an objective using your team's agreed memory layout.

Inputs:
- offset: int (types from Cambc.Shared_memory)
- value2: Bytes.t (types from Cambc.Shared_memory)

```ocaml
let result = Shared_memory.read_into ~offset value2
```

### read_values

```ocaml
val read_values : offset:int -> 'a kind -> int -> 'a array
```

`read_values ~offset kind n` reads `n` typed values in little-endian layout. @param offset Byte offset into shared memory. @param kind Element type. @param n Number of values to read.

Share an objective using your team's agreed memory layout.

Inputs:
- offset: int (types from Cambc.Shared_memory)
- value2: 'a kind (types from Cambc.Shared_memory)
- value3: int (types from Cambc.Shared_memory)

```ocaml
let result = Shared_memory.read_values ~offset value2 value3
```

### set

```ocaml
val set : int -> int -> unit
```

`set offset value` writes one byte. @param offset Byte offset into shared memory. @param value Byte to store, 0 to 255.

Share an objective using your team's agreed memory layout.

Inputs:
- value1: int (types from Cambc.Shared_memory)
- value2: int (types from Cambc.Shared_memory)

```ocaml
let result = Shared_memory.set value1 value2
```

### size

```ocaml
val size : unit -> int
```

Shared memory size in bytes.

### write

```ocaml
val write : offset:int -> Bytes.t -> unit
```

`write ~offset data` writes bytes; raises if the range does not fit. @param offset Byte offset into shared memory. @param data Bytes to write.

Reserve byte zero in your team's memory layout for a shared objective ID. All allied bots must agree on its meaning.

```ocaml
let share_objective () =
  Shared_memory.write ~offset:0 (Bytes.make 1 (Char.chr 2))
```

### write_values

```ocaml
val write_values : offset:int -> 'a kind -> 'a array -> unit
```

`write_values ~offset kind values` writes typed values in little-endian layout. @param offset Byte offset into shared memory. @param kind Element type. @param values Values to write.

Share an objective using your team's agreed memory layout.

Inputs:
- offset: int (types from Cambc.Shared_memory)
- value2: 'a kind (types from Cambc.Shared_memory)
- value3: 'a array (types from Cambc.Shared_memory)

```ocaml
let result = Shared_memory.write_values ~offset value2 value3
```

## Cambc.Symmetry

### all

```ocaml
val all : t list
```

Every value, in declaration order.

Use this value in your bot's draft or turn logic.

```ocaml
let result = Symmetry.all
```

### t

```ocaml
type t = | Horizontal | Vertical | Rotational180
```



Public SDK type declaration.

```ocaml
type t = | Horizontal | Vertical | Rotational180
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string x` is the name of `x`. @param x Value to name.

## Cambc.Team

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true for the same team. @param a First team. @param b Second team.

### opponent

```ocaml
val opponent : t -> t
```

`opponent t` is the other team. @param t Team.

Use this value in your bot's draft or turn logic.

Inputs:
- value1: t (types from Cambc.Team)

```ocaml
let result = Team.opponent value1
```

### t

```ocaml
type t = Ally | Enemy
```



Public SDK type declaration.

```ocaml
type t = Ally | Enemy
```

### to_string

```ocaml
val to_string : t -> string
```

Name of a team. @param t Team.

## Cambc.Teleport

### is_ally

```ocaml
val is_ally : t -> bool
```

Check whether this teleport belongs to the observing team. @param t Teleport snapshot.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Teleport)

```ocaml
let is_ally = Teleport.is_ally value1
```

### is_enemy

```ocaml
val is_enemy : t -> bool
```

Check whether this teleport belongs to the opposing team. @param t Teleport snapshot.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Teleport)

```ocaml
let is_enemy = Teleport.is_enemy value1
```

### t

```ocaml
type t = { champion : champion; team : Team.t; position : Vec2.t; remaining_time : int; }
```



Public SDK type declaration.

```ocaml
type t = { champion : champion; team : Team.t; position : Vec2.t; remaining_time : int; }
```

### visible_tiles

```ocaml
val visible_tiles : t -> Bitboard.t
```

`visible_tiles t` is the tiles visible from the reservation. @param t Teleport snapshot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Teleport)

```ocaml
let result = Teleport.visible_tiles value1
```

### visible_tiles_into

```ocaml
val visible_tiles_into : t -> Bitboard.t -> unit
```

Like `visible_tiles`, but writes into `out`. @param t Teleport snapshot. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Teleport)
- value2: Bitboard.t (types from Cambc.Teleport)

```ocaml
let result = Teleport.visible_tiles_into value1 value2
```

## Cambc.Vec2

### ( - )

```ocaml
val ( - ) : t -> t -> t
```

Same as `sub`.

### ( * )

```ocaml
val ( * ) : int -> t -> t
```

Same as `scale`.

### ( + )

```ocaml
val ( + ) : t -> t -> t
```

Same as `add`.

### ( ~- )

```ocaml
val ( ~- ) : t -> t
```

Same as `neg`.

### add

```ocaml
val add : t -> t -> t
```

`add a b` is the component-wise sum. @param a First vector. @param b Second vector.

### chebyshev_distance

```ocaml
val chebyshev_distance : t -> t -> int
```

`chebyshev_distance a b` is the number of king moves between the tiles. @param a First vector. @param b Second vector.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)
- value2: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.chebyshev_distance value1 value2
```

### compare

```ocaml
val compare : t -> t -> int
```

Total order on vectors, by `x` then `y`. @param a First vector. @param b Second vector.

### cross

```ocaml
val cross : t -> t -> int
```

`cross a b` is the signed 2D cross product. @param a First vector. @param b Second vector.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)
- value2: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.cross value1 value2
```

### div

```ocaml
val div : t -> int -> t
```

`div v k` divides both components by `k`, truncating; raises on zero. @param v Vector. @param k Divisor.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)
- value2: int (types from Cambc.Vec2)

```ocaml
let result = Vec2.div value1 value2
```

### dot

```ocaml
val dot : t -> t -> int
```

`dot a b` is the dot product. @param a First vector. @param b Second vector.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)
- value2: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.dot value1 value2
```

### east

```ocaml
val east : t
```

`(1, 0)`.

### equal

```ocaml
val equal : t -> t -> bool
```

`equal a b` is true when both coordinates match. @param a First vector. @param b Second vector.

### hash

```ocaml
val hash : t -> int
```

Hash consistent with `equal`. @param v Vector.

### length_squared

```ocaml
val length_squared : t -> int
```

`length_squared v` is the squared Euclidean length. @param v Vector.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.length_squared value1
```

### make

```ocaml
val make : int -> int -> t
```

`make x y` is the vector `(x, y)`. @param x Horizontal coordinate. @param y Vertical coordinate.

### neg

```ocaml
val neg : t -> t
```

`neg v` negates both components. @param v Vector.

### north

```ocaml
val north : t
```

`(0, 1)`; +y is north, matching the engine and `rotate_cw`/`rotate_ccw`.

### north_east

```ocaml
val north_east : t
```

`(1, 1)`.

### north_west

```ocaml
val north_west : t
```

`(-1, 1)`.

### reflect_horizontal

```ocaml
val reflect_horizontal : t -> t
```

`reflect_horizontal p` reflects a map position horizontally, reversing x. @param p Map position.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.reflect_horizontal value1
```

### reflect_vertical

```ocaml
val reflect_vertical : t -> t
```

`reflect_vertical p` reflects a map position vertically, reversing y. @param p Map position.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.reflect_vertical value1
```

### rotate_180

```ocaml
val rotate_180 : t -> t
```

`rotate_180 p` rotates a map position 180 degrees within the map. @param p Map position.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.rotate_180 value1
```

### rotate_ccw

```ocaml
val rotate_ccw : t -> t
```

`rotate_ccw v` rotates a direction 45 degrees counter-clockwise. @param v Vector.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.rotate_ccw value1
```

### rotate_cw

```ocaml
val rotate_cw : t -> t
```

`rotate_cw v` rotates a direction 45 degrees clockwise. @param v Vector.

Compare positions or adjust a movement direction.

Inputs:
- value1: t (types from Cambc.Vec2)

```ocaml
let result = Vec2.rotate_cw value1
```

### scale

```ocaml
val scale : int -> t -> t
```

`scale k v` multiplies both components by `k`. @param k Scale factor. @param v Vector.

### south

```ocaml
val south : t
```

`(0, -1)`.

### south_east

```ocaml
val south_east : t
```

`(1, -1)`.

### south_west

```ocaml
val south_west : t
```

`(-1, -1)`.

### sub

```ocaml
val sub : t -> t -> t
```

`sub a b` is the component-wise difference `a - b`. @param a First vector. @param b Second vector.

### t

```ocaml
type t = { x : int; y : int }
```



Public SDK type declaration.

```ocaml
type t = { x : int; y : int }
```

### to_string

```ocaml
val to_string : t -> string
```

`to_string v` formats the vector as `(x, y)`. @param v Vector.

### west

```ocaml
val west : t
```

`(-1, 0)`.

### zero

```ocaml
val zero : t
```

`(0, 0)`.

## Cambc.Vision

### altar_lock_at

```ocaml
val altar_lock_at : Vec2.t -> Altar_lock.t
```

`altar_lock_at p` is the visible altar lock on `p`, ally first; raises if there is none. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.altar_lock_at value1
```

### altar_lock_at_opt

```ocaml
val altar_lock_at_opt : Vec2.t -> Altar_lock.t option
```

Like `altar_lock_at`, but `None` instead of raising. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.altar_lock_at_opt value1
```

### altar_lock_tiles

```ocaml
val altar_lock_tiles : ?team:Team.t -> unit -> Bitboard.t
```

`altar_lock_tiles ?team ()` is the tiles of visible altar locks. @param team Team to include; both teams when omitted.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.altar_lock_tiles ()
```

### altar_lock_tiles_into

```ocaml
val altar_lock_tiles_into : ?team:Team.t -> Bitboard.t -> unit
```

Like `altar_lock_tiles`, but writes into `out`. @param team Team to include; both teams when omitted. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Bitboard.t (types from Cambc.Vision)

```ocaml
let result = Vision.altar_lock_tiles_into value1
```

### altar_locks

```ocaml
val altar_locks : ?team:Team.t -> unit -> Altar_lock.t list
```

`altar_locks ?team ()` is snapshots of visible altar locks. @param team Team to include; both teams when omitted.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.altar_locks ()
```

### asleep_camp_tiles

```ocaml
val asleep_camp_tiles : unit -> Bitboard.t
```

Tiles of visible asleep camps. Dead camps are excluded.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.asleep_camp_tiles ()
```

### asleep_camp_tiles_into

```ocaml
val asleep_camp_tiles_into : Bitboard.t -> unit
```

Like `asleep_camp_tiles`, but writes into `out`. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Bitboard.t (types from Cambc.Vision)

```ocaml
let result = Vision.asleep_camp_tiles_into value1
```

### awake_camp_tiles

```ocaml
val awake_camp_tiles : unit -> Bitboard.t
```

Tiles of visible awake camps. Dead camps are excluded.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.awake_camp_tiles ()
```

### awake_camp_tiles_into

```ocaml
val awake_camp_tiles_into : Bitboard.t -> unit
```

Like `awake_camp_tiles`, but writes into `out`. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Bitboard.t (types from Cambc.Vision)

```ocaml
let result = Vision.awake_camp_tiles_into value1
```

### camp_at

```ocaml
val camp_at : Vec2.t -> Camp.t
```

`camp_at p` is the visible camp currently on `p`; raises if there is none or the tile is hidden. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.camp_at value1
```

### camp_at_opt

```ocaml
val camp_at_opt : Vec2.t -> Camp.t option
```

Like `camp_at`, but `None` instead of raising. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.camp_at_opt value1
```

### camp_groups

```ocaml
val camp_groups : unit -> Camp_group.t list
```

Camp groups with at least one visible camp.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.camp_groups ()
```

### camp_tiles

```ocaml
val camp_tiles : unit -> Bitboard.t
```

Tiles of visible camps.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.camp_tiles ()
```

### camp_tiles_into

```ocaml
val camp_tiles_into : Bitboard.t -> unit
```

Like `camp_tiles`, but writes into `out`. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Bitboard.t (types from Cambc.Vision)

```ocaml
let result = Vision.camp_tiles_into value1
```

### camps

```ocaml
val camps : unit -> Camp.t list
```

Visible camps.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.camps ()
```

### champion_at

```ocaml
val champion_at : Vec2.t -> Champion.t
```

`champion_at p` is the visible champion on `p`; raises if there is none or the tile is hidden. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.champion_at value1
```

### champion_at_opt

```ocaml
val champion_at_opt : Vec2.t -> Champion.t option
```

Like `champion_at`, but `None` instead of raising. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.champion_at_opt value1
```

### champion_tiles

```ocaml
val champion_tiles : ?team:Team.t -> unit -> Bitboard.t
```

`champion_tiles ?team ()` is the tiles of visible champions. @param team Team to include; both teams when omitted.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.champion_tiles ()
```

### champion_tiles_into

```ocaml
val champion_tiles_into : ?team:Team.t -> Bitboard.t -> unit
```

Like `champion_tiles`, but writes into `out`. @param team Team to include; both teams when omitted. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Bitboard.t (types from Cambc.Vision)

```ocaml
let result = Vision.champion_tiles_into value1
```

### champions

```ocaml
val champions : ?team:Team.t -> unit -> Champion.t list
```

`champions ?team ()` is the visible champions. @param team Team to include; both teams when omitted.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.champions ()
```

### has_altar_lock

```ocaml
val has_altar_lock : Vec2.t -> bool
```

`has_altar_lock p` is true when a visible altar lock of either team is on `p`. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let has_altar_lock = Vision.has_altar_lock value1
```

### has_camp

```ocaml
val has_camp : Vec2.t -> bool
```

`has_camp p` is true when a visible camp is on `p`. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let has_camp = Vision.has_camp value1
```

### has_champion

```ocaml
val has_champion : Vec2.t -> bool
```

`has_champion p` is true when a visible champion is on `p`. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let has_champion = Vision.has_champion value1
```

### has_teleport

```ocaml
val has_teleport : Vec2.t -> bool
```

`has_teleport p` is true when a visible teleport reservation is on `p`. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let has_teleport = Vision.has_teleport value1
```

### has_ward

```ocaml
val has_ward : ?team:Team.t -> Vec2.t -> bool
```

`has_ward ?team p` is true when a visible ward is on `p`; hidden wards give `false`. @param team Team to restrict to; either team when omitted. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let has_ward = Vision.has_ward value1
```

### teleport_at

```ocaml
val teleport_at : Vec2.t -> Teleport.t
```

`teleport_at p` is the visible teleport reservation on `p`; raises if there is none. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.teleport_at value1
```

### teleport_at_opt

```ocaml
val teleport_at_opt : Vec2.t -> Teleport.t option
```

Like `teleport_at`, but `None` instead of raising. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.teleport_at_opt value1
```

### teleport_tiles

```ocaml
val teleport_tiles : unit -> Bitboard.t
```

Tiles of visible teleport reservations.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.teleport_tiles ()
```

### teleport_tiles_into

```ocaml
val teleport_tiles_into : Bitboard.t -> unit
```

Like `teleport_tiles`, but writes into `out`. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Bitboard.t (types from Cambc.Vision)

```ocaml
let result = Vision.teleport_tiles_into value1
```

### teleports

```ocaml
val teleports : unit -> Teleport.t list
```

Snapshots of visible teleport reservations.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.teleports ()
```

### visible_tiles

```ocaml
val visible_tiles : unit -> Bitboard.t
```

Every tile visible to the bot's champion.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.visible_tiles ()
```

### visible_tiles_into

```ocaml
val visible_tiles_into : Bitboard.t -> unit
```

Like `visible_tiles`, but writes into `out`. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Bitboard.t (types from Cambc.Vision)

```ocaml
let result = Vision.visible_tiles_into value1
```

### ward_at

```ocaml
val ward_at : ?team:Team.t -> Vec2.t -> Ward.t
```

`ward_at ?team p` is the visible ward on `p`; raises if there is none. @param team Team to restrict to; either team when omitted. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.ward_at value1
```

### ward_at_opt

```ocaml
val ward_at_opt : ?team:Team.t -> Vec2.t -> Ward.t option
```

Like `ward_at`, but `None` instead of raising. @param team Team to restrict to; either team when omitted. @param p Tile to query.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

**vision**: This function requires you to have the subject in vision.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Vision)

```ocaml
let result = Vision.ward_at_opt value1
```

### ward_tiles

```ocaml
val ward_tiles : ?team:Team.t -> unit -> Bitboard.t
```

`ward_tiles ?team ()` is the tiles of visible wards. @param team Team to include; both teams when omitted.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.ward_tiles ()
```

### ward_tiles_into

```ocaml
val ward_tiles_into : ?team:Team.t -> Bitboard.t -> unit
```

Like `ward_tiles`, but writes into `out`. @param team Team to include; both teams when omitted. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Bitboard.t (types from Cambc.Vision)

```ocaml
let result = Vision.ward_tiles_into value1
```

### wards

```ocaml
val wards : ?team:Team.t -> unit -> Ward.t list
```

`wards ?team ()` is snapshots of visible wards. @param team Team to include; both teams when omitted.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

```ocaml
let result = Vision.wards ()
```

## Cambc.Ward

### is_ally

```ocaml
val is_ally : t -> bool
```

Check whether this ward belongs to the observing team. @param w Ward snapshot.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Ward)

```ocaml
let is_ally = Ward.is_ally value1
```

### is_enemy

```ocaml
val is_enemy : t -> bool
```

Check whether this ward belongs to the opposing team. @param w Ward snapshot.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Ward)

```ocaml
let is_enemy = Ward.is_enemy value1
```

### owner

```ocaml
val owner : t -> champion
```

Return the owning champion captured when this ward was visible, even if the owner is hidden. Live champion state still requires the champion to be in vision. @param w Ward snapshot.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Ward)

```ocaml
let result = Ward.owner value1
```

### t

```ocaml
type t = { position : Vec2.t; team : Team.t; remaining_time : int option; owner : champion }
```



Public SDK type declaration.

```ocaml
type t = { position : Vec2.t; team : Team.t; remaining_time : int option; owner : champion }
```

### visible_tiles

```ocaml
val visible_tiles : t -> Bitboard.t
```

`visible_tiles w` is the tiles the ward sees. @param w Ward snapshot.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Ward)

```ocaml
let result = Ward.visible_tiles value1
```

### visible_tiles_from

```ocaml
val visible_tiles_from : Vec2.t -> Bitboard.t
```

`visible_tiles_from p` is the vision an ordinary ward would have at `p`. @param p Hypothetical ward tile.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Ward)

```ocaml
let result = Ward.visible_tiles_from value1
```

### visible_tiles_from_into

```ocaml
val visible_tiles_from_into : Vec2.t -> Bitboard.t -> unit
```

Like `visible_tiles_from`, but writes into `out`. @param p Hypothetical ward tile. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: Vec2.t (types from Cambc.Ward)
- value2: Bitboard.t (types from Cambc.Ward)

```ocaml
let result = Ward.visible_tiles_from_into value1 value2
```

### visible_tiles_into

```ocaml
val visible_tiles_into : t -> Bitboard.t -> unit
```

Like `visible_tiles`, but writes into `out`. @param w Ward snapshot. @param out Destination; its previous contents are overwritten.

**gameplay**: This function can only be used by champion bots and is not usable during draft phase.

Check visible objects before choosing a destination.

Inputs:
- value1: t (types from Cambc.Ward)
- value2: Bitboard.t (types from Cambc.Ward)

```ocaml
let result = Ward.visible_tiles_into value1 value2
```
