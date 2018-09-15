const argv = require('yargs').argv
const out = require('fs').createWriteStream("out.vzd")
const ndarray = require('ndarray')
const fill = require('ndarray-fill')
const volumeCount = 1
const BRICK_DIAMETER = 16

function uint32(val, stream) {
	const b = Buffer.alloc(4)
	b.writeUInt32LE(val, 0)
	stream.write(b)
}

function int32(val, stream) {
	const b = Buffer.alloc(4)
	b.writeInt32LE(val, 0)
	stream.write(b)
}


function uvec3(val, stream) {
	uint32(val[0], stream)
	uint32(val[1], stream)
	uint32(val[2], stream)
}

function float(val, stream) {
	const b = Buffer.alloc(4)
	b.writeFloatLE(val, 0)
	stream.write(b)
}

function mat4(val, stream) {
	for (var i = 0; i<val.length; i++) {
		float(val[i], stream)
	}
}

function vec3(val, stream) {
	float(val[0], stream)
	float(val[1], stream)
	float(val[2], stream)
}

function uint8(val, stream) {
	const b = Buffer.alloc(1)
	b.writeUInt8(val)
	stream.write(b)
}

function string(val, stream) {
	stream.write(val)
}

/*

FORMAT
HEADER
string "VZD"
uint32 volumeCount

VOLUME
mat4 transform (length: 64 bytes)
uint32 materialLength
void *material (length: materialLength bytes)
uint32 voxelSize (bytes)
uint32 brickCount

BRICKS
int32 x-index
int32 y-index
int32 z-index
void *brickVoxels (length: voxelSize * BRICK_DIAMETER^3)

repeat VOLUME
*/

string('VZD', out)

// begin volume
// specify volume dimensions 3x32bit uint
const dim = 512
const dims = [dim, dim, dim]
const hd = [dims[0] / 2, dims[1] / 2, dims[2] / 2]
const brickDims = [BRICK_DIAMETER, BRICK_DIAMETER, BRICK_DIAMETER]

// volume transform
const tx = [
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
]

const volumes = [
// cylinder
{
	dims: [dim/2, dim * 2, dim/2],
	hd: [dim/4, dim, dim/4],
	material:[255, 0, 0],
	fillFn: function (x, y, z) {
		const dx = x - this.hd[0]
		const dz = z - this.hd[2]
		const d = Math.sqrt(dx*dx + dz*dz) - this.hd[0]
		return d;
	}
},
// sphere
{
	dims: [dim, dim, dim],
	hd: [dim/2, dim/2, dim/2],
	material:[255, 255, 0],
	fillFn: function (x, y, z) {
		const dx = x - this.hd[0]
		const dy = y - this.hd[1]
		const dz = z - this.hd[1]

		const d = Math.sqrt(dx*dx + dy*dy + dz*dz) - this.hd[0]
		return d;
	}
}
]

uint32(volumes.length, out)

volumes.forEach((v) => {
	dumpVolume(v, out)
})
out.end()
console.log('\ndone')

function dumpVolume(vol, out) {
	mat4(tx, out)

	// material properties
	const materialLength = 3
	uint32(materialLength, out)
	// write a simple color for now in the first 3 bytes
	uint8(vol.material[0], out)
	uint8(vol.material[1], out)
	uint8(vol.material[2], out)

	// write out the size of a voxel, in bytes
	uint32(1, out);

	// TODO: handle negative
	const brickIndices = [
		vol.dims[0] / BRICK_DIAMETER|0,
		vol.dims[1] / BRICK_DIAMETER|0,
		vol.dims[2] / BRICK_DIAMETER|0
	]

	var bricks = []

	for (var x = 0; x < brickIndices[0]; x++) {
		process.stdout.write('.')
		for (var y = 0; y < brickIndices[1]; y++) {
			for (var z = 0; z < brickIndices[2]; z++) {
				// output the brick index
				var data = Buffer.alloc(Math.pow(BRICK_DIAMETER, 3))
				var brick = ndarray(data, brickDims).transpose(2, 1, 0)
				var completelyInside = true
				var completelyEmpty = true
				fill(brick, (lx, ly, lz) => {
					 var val = vol.fillFn(
						lx + x * BRICK_DIAMETER,
						ly + y * BRICK_DIAMETER,
						lz + z * BRICK_DIAMETER
					)

					if (val > -BRICK_DIAMETER/2.0) {
						completelyInside = false
					}

					var ret = val <= 0 ? 1 : 0;

					if (ret) {
						completelyEmpty = false
					}
					return ret
				})

				if (!completelyInside && !completelyEmpty) {
					bricks.push({
						x: x,
						y: y,
						z: z,
						data: data
					})
				}
			}
		}
	}

	// output brick count
	console.log("brick count:", bricks.length)
	uint32(bricks.length, out)
	bricks.forEach((b) => {
		int32(b.x, out)
		int32(b.y, out)
		int32(b.z, out)
		console.log("wrote", out.write(b.data), b.data.length)
	})
}
