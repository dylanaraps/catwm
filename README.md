
# sowm

<a href="https://user-images.githubusercontent.com/6799467/66687576-9747c200-ec72-11e9-947d-5b96753eab03.jpg"><img src="https://user-images.githubusercontent.com/6799467/66687576-9747c200-ec72-11e9-947d-5b96753eab03.jpg" width="43%" align="right"></a>

An itsy bitsy floating window manager with roots in `catwm`.

- Floating only.
- Maximize toggle.
- Window centering.
- Mix of mouse and keyboard workflow.
- Focus with cursor.
- All windows die on exit.

<a href="https://user-images.githubusercontent.com/6799467/66687814-8cd9f800-ec73-11e9-97b8-6ae77876bd1b.jpg"><img src="https://user-images.githubusercontent.com/6799467/66687814-8cd9f800-ec73-11e9-97b8-6ae77876bd1b.jpg" width="43%" align="right"></a>

- No borders.
- No bar support.
- No ICCM.
- No EMWH.
- Alt-Tab window focusing.
- etc etc etc


<br>

## Default Keybindings

**Window Management**

| combo                   | action                 |
| ----------------------- | -----------------------|
| `Mouse`                 | Focus under cursor     |
| `MOD4` + `Left Mouse`   | move window            |
| `MOD4` + `Right Mouse`  | resize window          |
| `MOD4` + `f`            | maximize toggle        |
| `MOD4` + `c`            | center window          |
| `MOD4` + `q`            | kill window            |
| `MOD4` + `1-9`          | desktop swap           |
| `MOD4` + `Shift` +`1-9` | send window to desktop |
| `MOD1` + `TAB`          | focus cycle            |

**Programs**

| combo                   | action           | program        |
| ----------------------- | ---------------- | -------------- |
| `MOD4` + `Return`       | terminal         | `st`           |
| `MOD4` + `d`            | dmenu            | `dmenu_run`    |
| `MOD4` + `p`            | scrot            | `scr`          |
| `MOD4` + `w`            | wallpaper cycler | `bud`          |
| `XF86_AudioLowerVolume` | volume down      | `amixer`       |
| `XF86_AudioRaiseVolume` | volume up        | `amixer`       |
| `XF86XK_AudioMute`      | volume toggle    | `amixer`       |


## Dependencies

- `xlib` (*usually `libX11`*).

## Installation

1) Modify `config.h` to suit your needs.
2) Run `make` to build `sowm`.
3) Copy it to your path or run `make install`.
    - `DESTDIR` and `PREFIX` are supported.
