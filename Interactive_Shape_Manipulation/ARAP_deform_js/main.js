// Three.js - Load .OBJ ?
// from https://threejsfundamentals.org/threejs/threejs-load-obj-no-materials.html

// https://github.com/scijs/cholesky-solve

import * as THREE from 'https://threejsfundamentals.org/threejs/resources/threejs/r115/build/three.module.js';
import {
OrbitControls
} from 'https://threejsfundamentals.org/threejs/resources/threejs/r115/examples/jsm/controls/OrbitControls.js';
import {
OBJLoader2
} from 'https://threejsfundamentals.org/threejs/resources/threejs/r115/examples/jsm/loaders/OBJLoader2.js';

// var choleskySolve = require('cholesky-solve')

function createStats() {
	var stats = new Stats();
	stats.setMode(0);

	stats.domElement.style.position = 'absolute';
	stats.domElement.style.left = '0';
	stats.domElement.style.top = '0';

	return stats;
}

function main() {
	// global vars
	const SELECT_FEATURE = 0;
	const MOVE_FEATURE = 1;
	var mode = SELECT_FEATURE;
	var deforming = false;
	var pos = null;
	var vertices = null;
	var eij = [];
	var eij_vec3 = [];
	var Ri = [];

	var solver = null;
	var neighbors = [];
	var selected_handle = -1;
	var handles = [];
	var is_constrain = [];
	var non_constrain_idx = [];

	window.move = function() {
		let d = 0.001;
		for(let i of handles[0]) {
			vertices[3*i+0] += d;
			vertices[3*i+1] += d;
			vertices[3*i+2] += d;
		}
	}

	const stats = createStats();
	document.body.appendChild( stats.domElement );

	const canvas = document.querySelector('#c');
	const renderer = new THREE.WebGLRenderer({
		canvas
	});

	const fov = 45;
	const aspect = 2; // the canvas default
	const near = 0.1;
	const far = 100;
	const camera = new THREE.PerspectiveCamera(fov, aspect, near, far);
	camera.position.set(0, 0, 5);

	const controls = new OrbitControls(camera, canvas);
	controls.target.set(0, 0, 0);
	controls.update();

	const checkbox = document.getElementById('enable_cam');

	checkbox.addEventListener('change', (event) => {
		controls.enabled = event.target.checked;
	})

	function get_xy(event) {
		let bound = canvas.getBoundingClientRect();
		let x = event.clientX - bound.left - canvas.clientLeft;
		let y = event.clientY - bound.top - canvas.clientTop;

		return {x: (x / bound.width) * 2 - 1, y: -((y / bound.height) * 2 - 1)};
	}

	function prepare_matrix() {
		console.log('preparing...')
		let p_to_idx = new Array(pos.count);
		non_constrain_idx = [];
		for(let i=0;i<pos.count;++i) {
			if (!is_constrain[i]) {
				p_to_idx[i] = non_constrain_idx.length;
				non_constrain_idx.push(i);
			}
		}

		let n = non_constrain_idx.length;
		var M = [];
		for (let i=0;i<n;++i) {
			let curidx = non_constrain_idx[i];
			M.push([i, i, neighbors[curidx].length]);
			for (let p of neighbors[curidx]) {
				if (!is_constrain[p]) {
					M.push([i, p_to_idx[p], -1]);
				}
			}
		}
		// let P = cuthillMckee(M, n);
		solver = prepare(M, n);
		console.log('prepare done')
	}

	function compute_Ri() {
		// compute epij
		var epij = [];
		for(let i=0;i<pos.count;++i) {
			let e = [];
			for(let nei of neighbors[i]) {
				e.push([
					vertices[3*i+0] - vertices[3*nei+0],
					vertices[3*i+1] - vertices[3*nei+1],
					vertices[3*i+2] - vertices[3*nei+2]
				])
			}
			epij.push(e);
		}
		for(let i=0;i<pos.count;++i) {
			let e = eij[i];
			let ep = epij[i];
			let a = [];
			for(let j=0;j<3;++j) {
				let r = [];
				for(let k=0;k<3;++k) {
					let s = 0;
					for(let t=0;t<neighbors[i].length;++t) {
						s += e[t][j] * ep[t][k];
					}
					r.push(s);
				}
				a.push(r);
			}
			let { u, v, q } = SVDJS.SVD(a);
			let _u = new THREE.Matrix3();
			_u.set(
				u[0][0], u[0][1], u[0][2],
				u[1][0], u[1][1], u[1][2],
				u[2][0], u[2][1], u[2][2],
			)
			_u.transpose();
			let _v = new THREE.Matrix3();
			_v.set(
				v[0][0], v[0][1], v[0][2],
				v[1][0], v[1][1], v[1][2],
				v[2][0], v[2][1], v[2][2],
			)
			Ri[i] = _v.clone().multiply(_u);
			let det = Ri[i].determinant()
			let flip = new THREE.Matrix3();
			flip.set(
				1, 0, 0,
				0, det, 0,
				0, 0, 1,
			);
			Ri[i] = _v.multiply(flip).multiply(_u);
		}
	}

	function compute_p_prime() {
		var b = [];
		for(let j=0;j<3;++j) {
			b.push(new Array(non_constrain_idx.length).fill(0));
		}
		for(let i=0;i<non_constrain_idx.length;++i) {
			let curidx = non_constrain_idx[i];
			let nidx = 0;
			for(let p of neighbors[curidx]) {
				// let e = eij[curidx][nidx++];
				let b1 = eij_vec3[curidx][nidx].clone().applyMatrix3(Ri[curidx]);
				let b2 = eij_vec3[curidx][nidx].clone().applyMatrix3(Ri[p]);
				let bv = b1.add(b2).multiplyScalar(0.5);
				b[0][i] += bv.x;
				b[1][i] += bv.y;
				b[2][i] += bv.z;
				// b[0][i] += e[0];
				// b[1][i] += e[1];
				// b[2][i] += e[2];
				if (is_constrain[p]) {
					b[0][i] += vertices[3*p+0];
					b[1][i] += vertices[3*p+1];
					b[2][i] += vertices[3*p+2];
				}
				nidx += 1;
			}
		}
		for(let j=0;j<3;++j) {
			let solved = solver(b[j]);
			for(let i=0;i<non_constrain_idx.length;++i) {
				let curidx = non_constrain_idx[i];
				vertices[3*curidx+j] = solved[i];
			}
		}
		pos.needsUpdate = true;
		// console.log('done')
	}

	canvas.addEventListener('keypress', function(e) {
		// console.log(e.keyCode)
		switch(e.keyCode) {
		case 49: // 1
			deforming = false;
			mode = SELECT_FEATURE;
			console.log('select feature');
			break;
		case 50: // 2
			mode = MOVE_FEATURE;
			console.log('move feature');
			prepare_matrix();
			deforming = true;
			break;
		case 100: // d
			prepare_matrix();
			break;
		case 102: // f
			compute_p_prime();
			break;
		case 32:
			let flip = new THREE.Matrix3();
			flip.set(
				1, 0, 0,
				0, 2, 0,
				0, 0, 1,
			);
			let flip2 = new THREE.Matrix3();
			flip2.set(
				1, 0, 0,
				0, 2, 0,
				0, 0, 1,
			);
			console.log(flip.multiply(flip2))
			console.log(flip.multiply(flip2))
			// compute_Ri();
			// compute_p_prime();
			break;
		}
	}, false)

	function dist(x1, y1, x2, y2) {
		let dx = (x1-x2);
		let dy = (y1-y2);
		return dx*dx+dy*dy;
	}

	var last_pos = null;
	var start_pos = null;
	var drag = false;
	canvas.addEventListener('mousedown', (e) => {
		start_pos = get_xy(e);
		if (controls.enabled == false) {
			if (mode == MOVE_FEATURE) {
				// find nearest handle
				last_pos = get_xy(e);
				let min_dist = 1e9;
				for(let h=0;h<handles.length;++h) {
					let handle = handles[h];
					for(let vid of handle) {
						let idx = 3 * vid;
						let p = (new THREE.Vector3(
							vertices[idx+0],vertices[idx+1],vertices[idx+2]
						)).project(camera);
						let d = dist(p.x, p.y, start_pos.x, start_pos.y);
						if (d < min_dist) {
							min_dist = d;
							selected_handle = h;
						}
					}
				}
				drag = true;
			}
		}
	}, false);
	canvas.addEventListener('mouseup', (e) => {
		let end_pos = get_xy(e);
		if (controls.enabled === false) {
			if (mode == SELECT_FEATURE) {
				let selected_idx = [];
				for(let i = 0; i < pos.count; ++i) {
					let idx = 3 * i;
					let p = (new THREE.Vector3(
						vertices[idx+0],vertices[idx+1],vertices[idx+2]
					)).project(camera);
					if (start_pos.x <= p.x && p.x <= end_pos.x && start_pos.y >= p.y && p.y >= end_pos.y) {
						selected_idx.push(i);
						is_constrain[i] = true;
					}
				}

				handles.push(selected_idx)
				console.log('#handles:', handles.length);
			} else if (mode == MOVE_FEATURE) {
				selected_handle = -1;
			}
		}
		drag = false;
	}, false);

	canvas.addEventListener('mousemove', function(e) {
		if (controls.enabled == false) {
			if (mode == MOVE_FEATURE && drag) {
				let xy = get_xy(e);
				let dx = (xy.x - last_pos.x) * 2;
				let dy = (xy.y - last_pos.y) * 2;
				let upxy = new THREE.Vector3(xy.x, xy.y, 0);
				let uplt = new THREE.Vector3(last_pos.x, last_pos.y, 0);
				upxy.unproject(camera);
				uplt.unproject(camera);
				let dir = upxy.sub(uplt);
				dir.multiplyScalar(20);
				for(let vidx of handles[selected_handle]) {
					let idx = 3*vidx;
					// vertices[idx+0] += dx;
					// vertices[idx+1] += dy;
					vertices[idx+0] += dir.x;
					vertices[idx+1] += dir.y;
					vertices[idx+2] += dir.z;
				}
				last_pos = xy;
				pos.needsUpdate = true;
			}
		}
	}, false);

	const scene = new THREE.Scene();
	scene.background = new THREE.Color('black');

	{
		const skyColor = 0xCCCCCC;
		const groundColor = 0x333333;
		const intensity = 1;
		const light = new THREE.HemisphereLight(skyColor, groundColor, intensity);
		scene.add(light);
	}

	{
		const color = 0xFFFFFF;
		const intensity = 0.3;
		const light = new THREE.DirectionalLight(color, intensity);
		light.position.set(0, 10, 0);
		light.target.position.set(-5, 0, 0);
		scene.add(light);
		scene.add(light.target);
	}

	{
		const objLoader = new OBJLoader2();
		objLoader.setUseIndices(true);
		objLoader.load('models/Armadillo.obj', (model) => {
			window.model = model;

			pos = model.children[0].geometry.attributes.position;
			vertices = pos['array'];

			Ri = new Array(pos.count);

			let index =  model.children[0].geometry.index.array;
			let num_tri = model.children[0].geometry.index.count / 3;

			// neighbors
			for(let i=0;i<pos.count;++i) {
				neighbors.push([]);
			}
			for(let i=0;i<num_tri;++i) {
				let idx = 3 * i;
				let a = index[idx+0];
				let b = index[idx+1];
				let c = index[idx+2];
				neighbors[a].push(b);
				neighbors[a].push(c);
				neighbors[b].push(a);
				neighbors[b].push(c);
				neighbors[c].push(a);
				neighbors[c].push(b);
			}

			is_constrain = new Array(pos.count).fill(false);

			// translate to origin and scale within unit cube
			let _min = vertices.slice(0, 3), _max = vertices.slice(0, 3);
			for(let i = 0; i < pos.count; ++i) {
				let idx = 3 * i;
				for(let j=0;j<3;++j) {
					_min[j] = Math.min(_min[j], vertices[idx + j])
					_max[j] = Math.max(_max[j], vertices[idx + j])
				}
			}

			let center = _min.map((v, i) => (v + _max[i]) / 2.0);
			let w = _max[0] - _min[0];
			let h = _max[1] - _min[1];
			let d = _max[2] - _min[2];
			let scale = 2.0 / Math.max(w, Math.max(h, d));

			for(let i = 0; i < pos.count; ++i) {
				let idx = 3 * i;
				vertices[idx + 0] -= center[0];
				vertices[idx + 1] -= center[1];
				vertices[idx + 2] -= center[2];
				vertices[idx + 0] *= scale;
				vertices[idx + 1] *= scale;
				vertices[idx + 2] *= scale;
			}

			// compute eij
			for(let i=0;i<pos.count;++i) {
				neighbors[i] = Array.from(new Set(neighbors[i]));
				let e = [];
				let e_vec3 = [];
				for(let nei of neighbors[i]) {
					e.push([
						vertices[3*i+0] - vertices[3*nei+0],
						vertices[3*i+1] - vertices[3*nei+1],
						vertices[3*i+2] - vertices[3*nei+2]
					]);
					e_vec3.push(new THREE.Vector3(
						vertices[3*i+0] - vertices[3*nei+0],
						vertices[3*i+1] - vertices[3*nei+1],
						vertices[3*i+2] - vertices[3*nei+2]
					));
				}
				eij.push(e);
				eij_vec3.push(e_vec3);
			}

			// draw bounding box
			let geometry = new THREE.BoxGeometry( 2, 2, 2);
			let wireframe = new THREE.WireframeGeometry( geometry );
			let line = new THREE.LineSegments( wireframe );
			line.material.depthTest = false;
			line.material.opacity = 0.25;
			line.material.transparent = true;

			setSmoothGeometry(model);
			scene.add(model);
			scene.add(line);
		});
	}

	function setSmoothGeometry(obj) {
		obj.traverse(node => {
			if ('geometry' in node) {
				node.geometry.computeVertexNormals();
			}
		})
	}

	function resizeRendererToDisplaySize(renderer) {
		const canvas = renderer.domElement;
		const width = canvas.clientWidth;
		const height = canvas.clientHeight;
		const needResize = canvas.width !== width || canvas.height !== height;
		if (needResize) {
			renderer.setSize(width, height, false);
		}
		return needResize;
	}

	function render() {

		if (resizeRendererToDisplaySize(renderer)) {
			const canvas = renderer.domElement;
			camera.aspect = canvas.clientWidth / canvas.clientHeight;
			camera.updateProjectionMatrix();
		}

		if (deforming && !controls.enabled) {
			compute_Ri();
			compute_p_prime();
		}

		renderer.render(scene, camera);
		stats.update();
		requestAnimationFrame(render);
	}

	requestAnimationFrame(render);
}

main();
