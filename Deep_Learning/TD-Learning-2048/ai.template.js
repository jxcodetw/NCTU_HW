function Ai() {
	this.init = function() {
		this.w = 0;
		this.ntuple = [];

		this.mapping = [0, 2, 4, 8, 16, 32,
			64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768];
		// this.action_map = [0, 3, 2, 1];
		this.c = [];
		for(let i=0;i<7;++i) {
			this.c.push(0x1<<(i*4));
		}
		console.log('ai loaded');
	}

	this.restart = function() {
		// This method is called when the game is reset.
	}

	this.get_board = function(grid) {
		var arr = [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]];
		for(let i=0;i<4;++i) {
			for(let j=0;j<4;++j) {
				if (grid.cells[i][j] == null) {
					grid.cells[i][j] = 0;
				} else {
					grid.cells[i][j] = Math.log2(grid.cells[i][j].value);
				}
			}
		}

		for(let i=0;i<4;++i) {
			for(let j=0;j<4;++j) {
				arr[i][j] = grid.cells[j][i];
			}
		}

		return arr;
	}

	this.board_eq = function(c1, c2) {
		for(let i=0;i<4;++i) {
			for(let j=0;j<4;++j) {
				if (c1[i][j] != c2[i][j]) {
					return false;
				}
			}
		}
		return true;
	}

	this.clone_cells = function(cells) {
		return cells.map(function(arr) {
			return arr.slice();
		});
	}

	this.rotate_left = function(cells) {
		var s_ = cells;
		var s = this.clone_cells(cells);
		for(let r=0;r<4;++r) {
			for(let c=0;c<4;++c) {
				s[3-c][r] = s_[r][c];
			}
		}
		return s;
	}

	this.rotate_right = function(cells) {
		var s_ = cells;
		var s = this.clone_cells(cells);
		for(let r=0;r<4;++r) {
			for(let c=0;c<4;++c) {
				s[c][3-r] = s_[r][c];
			}
		}
		return s;
	}

	this.flip = function(cells) {
		var s_ = cells;
		var s = this.clone_cells(cells);
		for(let r=0;r<4;++r) {
			for(let c=0;c<4;++c) {
				s[r][3-c] = s_[r][c];
			}
		}
		return s;
	}

	this.move_up = function(cells) {
		var s = this.clone_cells(cells);
		var pr = this.move_left(this.rotate_left(s));
		return [this.rotate_right(pr[0]), pr[1]];
	}

	this.move_left = function(cells) {
		var s = this.clone_cells(cells);
		let reward = 0;
		for(let r=0;r<4;++r) {
			let top = 0;
			let tmp = 0;
			for(let c=0;c<4;++c) {
				let tile = s[r][c];
				if (tile == 0) {continue;}
				s[r][c] = 0;

				if (tmp != 0) {
					if (tile == tmp) {
						tile += 1;
						s[r][top++] = tile;
						reward += (1<<tile);
						tmp = 0;
					} else {
						s[r][top++] = tmp;
						tmp = tile;
					}
				} else {
					tmp = tile;
				}
			}
			if (tmp != 0) {
				s[r][top] = tmp;
			}
		}
		return [s, reward];
	}

	this.move_down = function(cells) {
		var s = this.clone_cells(cells);
		var pr = this.move_left(this.rotate_right(s));
		return [this.rotate_left(pr[0]), pr[1]];
	}

	this.move_right = function(cells) {
		var s = this.clone_cells(cells);
		var pr = this.move_down(this.rotate_right(s));
		return [this.rotate_left(pr[0]), pr[1]];
	}

	this.compute_afterstate = function(cells, a) {
		if (a == 0) {
			return this.move_up(cells);
		}
		if (a == 1) {
			return this.move_right(cells);
		}
		if (a == 2) {
			return this.move_down(cells);
		}
		if (a == 3) {
			return this.move_left(cells);
		}
	}

	this.V = function(cells) {
		s = cells;
		let res = 0.0;
		for(let i=0;i<this.ntuple.length;++i) {
			let tuple = this.ntuple[i];
			for(let feature of tuple) {
				let idx = 0;
				for(let j=0;j<feature.length;++j) {
					let p = feature[j];
					idx += s[p[0]][p[1]] * this.c[j];
				}
				res += this.w[i][idx];
			}
		}
		return res;
	}

	this.get_possible_actions = function(cells) {
		var actions = [];
		for(let a=0;a<4;++a) {
			let s = this.clone_cells(cells);
			let pr = this.compute_afterstate(s, a);
			if (!this.board_eq(s, pr[0])) {
				actions.push([a, pr[1] + this.V(pr[0]) ]);
			}
		}
		return actions;
	}

	this.step = function(grid) {
		var cells = this.get_board(grid);
		// console.log('cell', cells)
		// console.log('up', this.move_up(this.clone_cells(cells)));
		// console.log('right', this.move_right(this.clone_cells(cells)));
		// console.log('down', this.move_down(this.clone_cells(cells)));
		// console.log('left', this.move_left(this.clone_cells(cells)));
		var actions = this.get_possible_actions(cells);
		// console.log(actions)
		if (actions.length > 0) {
			var mx_r = actions[0][1];
			var best_action = actions[0][0];
			for(let i=1;i<actions.length;++i) {
				let action = actions[i];
				if (action[1] > mx_r) {
					mx_r = action[1];
					best_action = action[0];
				}
			}
			// console.log(mx_r);

			return best_action;
		} else {
			// game over
		}
	}
}
