# Before Light

Classic-style screensavers for [Omarchy](https://omarchy.org) 4: bar picker, fullscreen preview, per-saver settings, idle hook.

Original engine and pixel art. Not affiliated with Berkeley Systems, Infinisys, or After Dark.

Listed for [Omarchy Plugins](https://omarchyplugins.com/).

## Install

```sh
omarchy plugin add https://github.com/flynnsbit/omarchy-beforelight.git --enable
```

That is the whole install. A plugin service copies the savers and idle wrapper on first load. Add the widget if enable did not place it:

```sh
omarchy bar add beforelight --section right
```

Then click the Before Light icon.

## Usage

- Click the bar icon to pick a screensaver.
- On the **ON** row: play previews fullscreen, stop ends preview, cog opens settings (hidden when that saver has none).
- Idle uses the ON saver. The SDL window is a topmost overlay, so the Omarchy bar stays running underneath and does not have to hide or come back.

## Remove

```sh
omarchy plugin remove beforelight
rm -f ~/.config/omarchy/bin/omarchy-beforelight \
      ~/.config/omarchy/bin/omarchy-beforelight-settings \
      ~/.config/omarchy/bin/omarchy-screensaver \
      ~/.config/environment.d/90-beforelight.conf
```

Optional: remove the Before Light block from `~/.config/hypr/hyprland.lua` and `~/.config/omarchy/branding/screensaver`.

## License

MIT.
