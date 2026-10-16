# D-pad

The navigation keys of a remote on one tile: up, down, left and right round an OK key, with back, home, menu and guide
in the corners. Each key performs the Home Assistant action you give it, so the same tile drives a streamer, a
Blu-ray player behind an IR remote, a set-top box, or anything a script can reach.

It works with anything: the keys are not tied to any one kind of device or integration. Each key sends exactly the
action you type, to the entity you choose, so it suits bespoke setups: an IR remote you taught your own commands, a
streamer behind a script, a projector on a serial bridge, a set-top box in a bar. That is where it differs from
Tessera's own remote card, which opens when you tap a `remote` tile and shows a ready-made keypad (arrows, OK, Back,
Home, Play/Pause and volume) for the integrations Tessera knows, such as Android TV, Apple TV and Roku. Use that where
it fits; use this tile where it does not (Harmony and Broadlink remotes, for example, have no built-in keypad), or when
you want the keys on the page itself, in one tap, with your own commands.

Give a key an action and it appears; leave it empty and it is left out, and its place stays empty so the other keys do
not move. The keys sit in a block of three by three in the middle of the tile. A key is never higher than 14 mm nor
wider than half again its height, so a bigger tile only gives them room. The tile is one cell at least and up to 3x3.

```
 back   up    home
 left   OK    right
 menu   down  guide
```

## Set up

1. In Tessera, open **Plugins**, choose **D-pad** and tick your screens. Each screen builds once with the plugin.
2. In Layout, place the **D-pad** tile from the library's Plugins group.
3. In the inspector, choose the entity the keys act on: usually a remote, but a media player, a script and so on work.
4. Type an action for each key you want, for example for an IR or Android TV remote:

   | Key | Remote |
   |---|---|
   | Up | `remote.send_command command=UP` |
   | Down | `remote.send_command command=DOWN` |
   | Left | `remote.send_command command=LEFT` |
   | Right | `remote.send_command command=RIGHT` |
   | OK | `remote.send_command command=OK` |
   | Back | `remote.send_command command=BACK` |
   | Home | `remote.send_command command=HOME` |
   | Menu | `remote.send_command command=MENU` |
   | Guide | `remote.send_command command=GUIDE` |

   The command names are whatever your integration or learned IR codes call them.

5. Make sure the screen may perform Home Assistant actions. Tessera turns this on for a screen it added to Home
   Assistant; for a screen added by hand, tick **Allow the device to perform Home Assistant actions** in the ESPHome
   integration's options for that device.

## What a key's action can say

A key's text is the action, then optionally an entity of its own, then optionally one field, separated by spaces:

- `button.press`: the action, on the tile's entity.
- `remote.send_command command=UP`: the action with one field, on the tile's entity. The value runs to the end of the
  text, so it may contain spaces.
- `script.turn_on script.open_menu`: the action on another entity than the tile's.
- `remote.send_command remote.hall command=up`: both.

A key whose text is not an action shows its icon in red, and a tap sends nothing. At most 64 characters per key, and
one field: for an action that needs more, write a script in Home Assistant and give the key `script.turn_on
script.<its name>`.

## On the screen

- A tap on a key lights it blue while the action goes out, or red when it could not be sent (no entity, no action, or
  the screen may not perform actions).
- The icons go grey while the entity is unavailable.
- Works on every board, the CYD included. On a single cell the keys get small; make the tile bigger to give them
  more room.

## Good to know

- A key runs whatever action you type for it, on whatever entity you name. The manifest lists the actions the tile is
  made for (remote, script, button, scene and switch actions), but the screen does not hold a key to that list: anyone
  who can edit the layout can make a key perform any action the screen is allowed to.
- Nothing leaves your home: the tile uses no internet service and no Home Assistant entity but the ones you choose.

## How it works

- `tessera-plugin.yaml`: one tile, `pad`, 1x1 to 3x3, that belongs to an entity of the domains it acts on, with one
  text option per key.
- `components/d_pad/dpad_command.h`: reads a key's text into an action, a target and a field (the same code as the
  Transport controls plugin).
- `components/d_pad/d_pad.cpp`: lays the keys out in a block of three by three in the middle of the tile, finds the
  key under the finger (`tessera::tap_point`) and sends its action (`tessera::action`).
