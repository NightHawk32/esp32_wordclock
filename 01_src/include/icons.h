int heart[11][11] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0},
  { 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}
};

int wifi[11][11] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
  { 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
  { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}
};

int ok[11][11] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0},
  { 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0},
  { 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0},
  { 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 0},
  { 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0},
  { 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

int error[11][11] = {
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  { 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
  { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
  { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
  { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0},
  { 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0},
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0},
  { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
};