const ctx = require('fc')(render, 1)
const center = require('ctx-translate-center')
const { vec2 } = require('gl-matrix')
const renderGrid = require('ctx-render-grid-lines')
const camera = require('ctx-camera')(ctx, window, {})
const Volume = require('./lib/volume')
const { BRICK_DIAMETER } = require('./lib/brick')
const tx = require('./lib/tx')

const fill = require('ndarray-fill')
const stock = new Volume([5, 0])
stock.name = "stock"

const tool = new Volume([0, 0])
tool.name = "tool"
const toolBrick = tool.addBrick([0, 0])
fill(toolBrick.grid, (x, y) => {
	const v = 1.0 + (x + y * BRICK_DIAMETER) / Math.pow(BRICK_DIAMETER, 2)
	return v
})
toolBrick.empty = false


tool.scale[0] = 1
tool.scale[1] = 1.0


const volumes = [
	stock,
	tool
]

var keys = {}

window.addEventListener("keydown", (e) => {
	keys[e.keyCode] = true
	ctx.start()
})

window.addEventListener("keyup", (e) => {
	keys[e.keyCode] = false


	if (!Object.keys(keys).filter((key) => keys[key]).length) {
		ctx.stop()
	}
})


function render() {

	if (keys[65]) {
		tool.pos[0] -= 0.1
	}

	if (keys[68]) {
		tool.pos[0] += 0.1
	}

	if (keys[87]) {
		tool.pos[1] += 0.1
	}

	if (keys[83]) {
		tool.pos[1] -= 0.1
	}


	//tool.pos[1] += 0.01;//Math.abs(Math.sin(Date.now() / 10000) * 0.01)
	//tool.pos[0] += 0.02;//Math.abs(Math.sin(Date.now() / 10000) * 0.01)
	//tool.rotation += 0.1
	stock.rotation += 0.002

	ctx.clear()
	camera.begin()
	center(ctx)
	ctx.scale(BRICK_DIAMETER * 5, -BRICK_DIAMETER * 5)
	ctx.lineWidth = 1.0/BRICK_DIAMETER * 0.1
	ctx.lineCap = "round"
	ctx.font = "1px monospace"

	// clear and rebuild the volume bricks
	//stock.bricks.clear()
	stock.opAdd(ctx, tool)

	volumes.forEach((v) => {
	  v.render(ctx)

		const offset = 0.05

	  const zero = [0, 0]
	  const left = [1, 0]
	  const up = [0, 1]
	  const model = v.modelMatrix

	  vec2.transformMat3(zero, zero, model)
	  vec2.transformMat3(left, left, model)
	  vec2.transformMat3(up, up, model)


	  ctx.lineWidth = 0.025
		ctx.beginPath()
			ctx.moveTo(zero[0], zero[1])
			ctx.lineTo(left[0], left[1])
			ctx.strokeStyle = "green"
			ctx.stroke()
		ctx.beginPath()
			ctx.moveTo(zero[0], zero[1])
			ctx.lineTo(up[0], up[1])
			ctx.strokeStyle = "red"
			ctx.stroke()
	})
	camera.end()
}

