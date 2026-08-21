function catalog() {
  return [
    { id: "omarchy", name: "Omarchy TTE", emoji: "🅰️", blurb: "Stock Omarchy terminal effects", hasSettings: false },
    { id: "toastersaver", name: "Paper Toasters", emoji: "🍞", blurb: "Origami-winged toasters on the breeze", hasSettings: true },
    { id: "fishsaver", name: "Aquarium", emoji: "🐟", blurb: "Pixel fish swimming a home tank", hasSettings: true },
    { id: "globe", name: "Globe", emoji: "🌍", blurb: "Earth spinning in space", hasSettings: true },
    { id: "spotlight", name: "Spotlight", emoji: "🔦", blurb: "City night with a moving beam", hasSettings: true },
    { id: "starrynight", name: "Starry Night", emoji: "✨", blurb: "Starfield over a city silhouette", hasSettings: true },
    { id: "matrix", name: "Code Rain", emoji: "🟢", blurb: "Falling green code rain", hasSettings: true },
    { id: "warp", name: "Warp", emoji: "💫", blurb: "Liquid distortion field", hasSettings: true },
    { id: "worms", name: "Worms", emoji: "🪱", blurb: "Crawling type worms", hasSettings: true },
    { id: "bouncingball", name: "Bouncing Balls", emoji: "⚽", blurb: "Physics balls and collisions", hasSettings: true },
    { id: "fadeout", name: "Fade Out", emoji: "🌫️", blurb: "Soft cloud fade", hasSettings: true },
    { id: "hardrain", name: "Hard Rain", emoji: "🌧️", blurb: "Heavy rain drops", hasSettings: true },
    { id: "rainstorm", name: "Rainstorm", emoji: "⛈️", blurb: "Layered storm rain", hasSettings: true },
    { id: "paperfire", name: "Paper Fire", emoji: "🔥", blurb: "Burning paper embers", hasSettings: true },
    { id: "lifeforms", name: "Life Forms", emoji: "🦠", blurb: "Conway's Game of Life", hasSettings: true },
    { id: "lifeforms_new", name: "Life Forms+", emoji: "🧬", blurb: "Alternate Game of Life", hasSettings: true },
    { id: "logo", name: "Logo", emoji: "🏷️", blurb: "Morphing Omarchy logo", hasSettings: true },
    { id: "messages", name: "Messages", emoji: "💬", blurb: "Scrolling text", hasSettings: true },
    { id: "messages2", name: "Messages 2", emoji: "💭", blurb: "Alternate scrolling text", hasSettings: true },
    { id: "randomizer", name: "Randomizer", emoji: "🎲", blurb: "Cycles every BeforeLight saver", hasSettings: true }
  ]
}

function parseConfig(raw) {
  try {
    var parsed = JSON.parse(String(raw || "{}"))
    if (!parsed || typeof parsed !== "object") parsed = {}
    return {
      selected: String(parsed.selected || "omarchy"),
      engine: String(parsed.engine || "omarchy"),
      previewSeconds: Number(parsed.previewSeconds || 12)
    }
  } catch (error) {
    return { selected: "omarchy", engine: "omarchy", previewSeconds: 12 }
  }
}

function parseScan(raw) {
  var installed = {}
  var lines = String(raw || "").split(/\r?\n/)
  for (var i = 0; i < lines.length; i++) {
    var name = String(lines[i] || "").trim()
    if (name) installed[name] = true
  }
  return installed
}

function buildItems(installed, selectedId, engine) {
  var cat = catalog()
  var items = []
  var onId = (engine === "omarchy") ? "omarchy" : String(selectedId || "omarchy")
  for (var i = 0; i < cat.length; i++) {
    var item = cat[i]
    if (item.id !== "omarchy" && !installed[item.id]) continue
    items.push({
      id: item.id,
      name: item.name,
      emoji: item.emoji,
      blurb: item.blurb,
      installed: true,
      selected: item.id === onId,
      hasSettings: item.hasSettings === true
    })
  }
  return items
}

function indexOf(items, id) {
  for (var i = 0; i < items.length; i++) if (items[i].id === id) return i
  return -1
}
