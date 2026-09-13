# Nintendo DS on SJGAM M21 / M22 Pro

Pak: `Emus/m21/NDS.pak`
ROMs: `Roms/Nintendo DS (NDS)/*.nds`
Core: `desmume2015_libretro.so`

## Controls

| Handheld button | NDS action     |
| --------------- | -------------- |
| D-pad           | D-pad          |
| A / B / X / Y   | A / B / X / Y  |
| Start / Select  | Start / Select |
| L1 / R1         | L / R          |
| L2              | Close/open lid |
| R2 (hold)       | Stylus touch   |
| Menu            | MinArch menu   |

### Stylus mode

Hold **L1** to show the cursor. While held, L1, D-pad, and ABXY control the
cursor and are not sent to the game.

- D-pad: move the cursor
- Y: up, A: down, X: left, B: right
- R2: hold to touch; release to lift the stylus
- Release L1: hide the cursor and restore normal game controls

## Screen Layout

Set a layout per game: `Menu > Options > Screen layout > Save for Game`.

| Layout         | Use                                                |
| -------------- | -------------------------------------------------- |
| `left/right`   | Default; both screens side by side                 |
| `top only`     | Best for games that do not need touch              |
| `bottom only`  | Full-screen touch-focused games                    |
| `quick switch` | Switch between full-screen top and bottom displays |
| `top/bottom`   | Traditional vertical DS layout                     |

Use `Aspect` scaling to preserve the image ratio.

## Performance

Defaults use the ARM JIT, native `256x192` resolution, frameskip `2`, one
rasterizer core, disabled advanced timing, and ROM preloading.

- Use `top only` or `bottom only` when possible.
- Keep CPU speed at `Max`.
- Compare `Thread video` on and off per game.
- Try `CPU cores = 2` only if a game is rasterizer-limited.
- Use `CPU mode = interpreter` only for games that crash with JIT.

The dual Cortex-A7 H133 cannot run demanding 3D DS games at full speed.
Use the Debug HUD to compare settings per game.
