-- Before Light: put ~/.config/omarchy/bin ahead of packaged Omarchy binaries
-- so idle uses the Before Light screensaver wrapper.
do
  local home = os.getenv("HOME") or ""
  local user_bin = home .. "/.config/omarchy/bin"
  local omarchy_bin = (os.getenv("OMARCHY_PATH") or "/usr/share/omarchy") .. "/bin"
  local kept = { user_bin, omarchy_bin }
  local seen = { [user_bin] = true, [omarchy_bin] = true }
  for entry in (os.getenv("PATH") or "/usr/bin"):gmatch("[^:]+") do
    if not seen[entry] then
      seen[entry] = true
      table.insert(kept, entry)
    end
  end
  hl.env("PATH", table.concat(kept, ":"))
end

o.window(
  "^(toastersaver|fishsaver|globe|spotlight|starrynight|matrix|warp|worms|bouncingball|fadeout|hardrain|rainstorm|paperfire|lifeforms|lifeforms_new|logo|messages|messages2|randomizer|org.omarchy.screensaver)$",
  { fullscreen = true, float = true, animation = "none", opacity = "1 1", tag = "-default-opacity" }
)
