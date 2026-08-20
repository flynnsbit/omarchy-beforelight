# Before Light

Classic-style screensavers for [Omarchy](https://omarchy.org) 4 (Quattro): a bar picker, fullscreen preview, per-saver settings, and an idle hook.

Not affiliated with Berkeley Systems, Infinisys, or After Dark. Original engine and original pixel art (origami-winged toasters, aquarium fish).

Intended for the [Omarchy Plugins](https://omarchyplugins.com/) directory. After this repo is public, submit it with the [marketplace form](https://github.com/HANCORE-linux/omarchy-plugin-marketplace/issues/new?template=submit-plugin.yml).

## Install (Omarchy 4)

Omarchy clones the plugin into `~/.config/omarchy/plugins/` but does **not** run setup hooks. After adding the plugin, run setup once so the SDL savers and idle wrapper are installed.

```sh
# 1. Add the bar plugin
omarchy plugin add https://github.com/flynnsbit/omarchy-beforelight.git --enable

# 2. Build savers + idle wrapper (needs gcc, SDL2, SDL2_image, SDL2_ttf; worms also SDL2_mixer)
~/.config/omarchy/plugins/io.github.flynnsbit.beforelight/scripts/setup.sh

# 3. Place it on the bar if enable did not
omarchy bar add io.github.flynnsbit.beforelight --section right

# 4. Reload
hyprctl reload
omarchy restart shell
```

Dependencies on Arch / Omarchy:

```sh
sudo pacman -S --needed gcc sdl2 sdl2_image sdl2_ttf sdl2_mixer mesa glu
```

## Usage

- Click the bar icon to pick a screensaver.
- On the **ON** row: play previews fullscreen, stop ends preview, cog opens settings (hidden if that saver has none).
- Idle uses the ON saver via a user `omarchy-screensaver` wrapper (does not replace files under `/usr/share/omarchy`).

## Remove

```sh
omarchy plugin remove io.github.flynnsbit.beforelight
rm -f ~/.config/omarchy/bin/omarchy-beforelight \
      ~/.config/omarchy/bin/omarchy-beforelight-settings \
      ~/.config/omarchy/bin/omarchy-screensaver
# Optional: rm -rf ~/.config/omarchy/branding/screensaver
# Remove the Before Light block from ~/.config/hypr/hyprland.lua if you added it via setup.sh
```

## Develop

```sh
omarchy plugin validate ~/Projects/beforelight
```

## License

MIT. Original sprites and C engine by Shawn Henderson (flynnsbit).
