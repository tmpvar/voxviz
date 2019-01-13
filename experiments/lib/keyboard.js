module.exports = createKeyboard



function createKeyboard(target) {
  target = target || window

  var keys = {}

  target.addEventListener("keydown", (e) => {
    keys[e.key] = true
  })

  target.addEventListener("keyup", (e) => {
    keys[e.key] = false
  })

  return {
    keys: keys
  }
}
