function Ai() {
	this.init = function() {
		this.w = 0;

		this.ntuple = [
			[ [0, 0], [0, 1], [0, 2], [0, 3] ],
			[ [1, 0], [1, 1], [1, 2], [1, 3] ],
			[ [2, 0], [2, 1], [2, 2], [2, 3] ],
			[ [3, 0], [3, 1], [3, 2], [3, 3] ],
	
			[ [0, 0], [1, 0], [2, 0], [3, 0] ],
			[ [0, 1], [1, 1], [2, 1], [3, 1] ],
			[ [0, 2], [1, 2], [2, 2], [3, 2] ],
			[ [0, 3], [1, 3], [2, 3], [3, 3] ],
	
			[ [0, 0], [0, 1], [1, 1], [1, 0] ],
			[ [0, 1], [0, 2], [1, 2], [1, 1] ],
			[ [0, 2], [0, 3], [1, 3], [1, 2] ],
	
			[ [1, 0], [1, 1], [2, 1], [2, 0] ],
			[ [1, 1], [1, 2], [2, 2], [2, 1] ],
			[ [1, 2], [1, 3], [2, 3], [2, 2] ],
	
			[ [2, 0], [2, 1], [3, 1], [3, 0] ],
			[ [2, 1], [2, 2], [3, 2], [3, 1] ],
			[ [2, 2], [2, 3], [3, 3], [3, 2] ]
		];

		this.mapping = [0, 2, 4, 8, 16, 32,
			64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768];
		this.action_map = [0, 3, 2, 1];
		this.c = [0x1, 0x10, 0x100, 0x1000];
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
				s[3-r][c] = s_[r][c];
			}
		}
		return s;
	}

	this.move_up = function(cells) {
		var s = this.clone_cells(cells);
		var reward = 0;
		for(let c=0;c<4;++c) {
			for(let r=0;r<4;++r) {
				if (s[r][c] == 0) {
					continue;
				}
				for(let j=r+1;j<4;++j) {
					if (s[j][c] != 0) {
						if (s[r][c] == s[j][c]) {
							s[r][c] += 1;
							s[j][c] = 0;
							reward += this.mapping[s[r][c]];
						}
						break;
					}
				}
			}
		}
		for(let c=0;c<4;++c) {
			for(let r=1;r<4;++r) {
				if (s[r][c] == 0) {
					continue;
				}
				let j = r;
				while(j-1 >= 0 && s[j-1][c] == 0) {
					j--;
				}
				if (s[j][c] == 0) {
	
					s[j][c] = s[r][c];
					s[r][c] = 0;
				}
			}
		}
		return [s, reward];
	}

	this.move_left = function(cells) {
		var s = this.clone_cells(cells);
		s = this.rotate_right(s);
		var pr = this.move_up(s);
		s = this.rotate_left(pr[0])
		return [s, pr[1]];
	}

	this.move_down = function(cells) {
		var s = this.clone_cells(cells);
		s = this.flip(s);
		var pr = this.move_up(s);
		s = this.flip(pr[0])
		return [s, pr[1]];
	}
	
	this.move_right = function(cells) {
		var s = this.clone_cells(cells);
		s = this.rotate_left(s);
		var pr = this.move_up(s);
		s = this.rotate_right(pr[0])
		return [s, pr[1]];
	}

	this.compute_afterstate = function(cells, a) {
		if (a == 0) {
			return this.move_up(cells);
		}
		if (a == 1) {
			return this.move_left(cells);
		}
		if (a == 2) {
			return this.move_down(cells);
		}
		if (a == 3) {
			return this.move_right(cells);
		}
	}

	this.V = function(cells) {
		s = cells;
		let res = 0;
		for(let i=0;i<this.ntuple.length;++i) {
			let tuple = this.ntuple[i];
			let idx = 0;
			for(let j=0;j<tuple.length;++j) {
				let p = tuple[j];
				idx += s[p[0]][p[1]] * this.c[j];
			}
			res += this.w[i][idx];
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
		var actions = this.get_possible_actions(cells);

		var mx_r = actions[0][1];
		var best_action = actions[0][0];
		for(let i=1;i<actions.length;++i) {
			let action = actions[i];
			if (action[1] > mx_r) {
				mx_r = action[1];
				best_action = action[0];
			}
		}

		return this.action_map[best_action];
	}
}
