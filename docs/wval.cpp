V = V1 - V0

w0 = P.x * V.y - P.y * V.x
//(P.x + 1) * V.y - P.y * V.x = P.x * V.y - P.y * V.x + V.y
//P.x * V.y - (P.y + 1) * V.x = P.x * V.y - P.y * V.x - V.x

for (int y = 0; y < Y; y++) {
	float w = w0;
	for (int x = 0; x < X; x++) {
		w += V.y;
	}
	w0 += -V.x;
}