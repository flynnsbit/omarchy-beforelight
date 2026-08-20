function parseList(raw) {
  try {
    var parsed = JSON.parse(String(raw || ""))
    if (!parsed || !Array.isArray(parsed.items))
      return { ok: false, error: "Could not read screensaver list.", selected: "", engine: "", items: [] }
    var items = []
    for (var i = 0; i < parsed.items.length; i++) {
      var item = parsed.items[i]
      if (!item || !item.id) continue
      items.push({
        id: String(item.id),
        name: String(item.name || item.id),
        emoji: String(item.emoji || ""),
        blurb: String(item.blurb || ""),
        installed: item.installed !== false,
        selected: item.selected === true,
        hasSettings: item.hasSettings === true
      })
    }
    return {
      ok: true,
      error: "",
      selected: String(parsed.selected || ""),
      engine: String(parsed.engine || ""),
      items: items
    }
  } catch (error) {
    return { ok: false, error: "Screensaver list was not valid JSON.", selected: "", engine: "", items: [] }
  }
}

function indexOf(items, id) {
  for (var i = 0; i < items.length; i++) if (items[i].id === id) return i
  return 0
}
