#include <stdio.h>

void test1() {
	float Px = 850.5f, Py = 695.5f;
	float Lsx = 1027.28271f, Lsy = 434.700897f, Lex = 836.817627f, Ley = 715.686218f;
	float Rsx = 1027.28271f, Rsy = 434.700897f, Rex = 836.817627f, Rey = 715.686218f;

	//Ax*By - Ay*Bx
	//A=Le-Ls; B=P-Ls
	//(Ax)*(By) - (Ay)*(Bx)
	float W0 = (Lex-Lsx)*(Py-Lsy) - (Ley-Lsy)*(Px-Lsx);

	//w0 = ((le - ls) ^ (vt - ls)).z
	//w1 = ((vt - rs) ^ (re - rs)).z

	float W1 = (Px-Rsx)*(Rey-Rsy) - (Py-Rsy)*(Rex-Rsx);

	printf("%f\n%f\n", W0, W1);
}

struct v2 {
	float x,y;
	v2() {}
	v2(float a, float b):x(a),y(b) {}
	v2(v2 const& v):x(v.x),y(v.y) {}
	v2 operator + (v2 const& v) const { return v2(x+v.x, y+v.y); }
	v2 operator - (v2 const& v) const { return v2(x-v.x, y-v.y); }
	v2 operator * (v2 const& v) const { return v2(x*v.x, y*v.y); }
	float operator | (v2 const& v) const { return x*v.x+y*v.y; }
	float operator ^ (v2 const& v) const { return x*v.y-y*v.x; }
};

void test2() {
	float sy1 = 684.500000f, ey1 = 717.641113f;
	float sx1 = 825.500000f, ex1 = 1063.50000f;

	float sy2 = 480.500000f, ey2 = 717.641113f;
	float sx2 = 791.500000f, ex2 = 1029.50000f;

	v2 pt{842.500061f, 694.500061f};
	v2 ls{1029.23853f, 446.433655f}, le{825.080078f, 717.641113f};
	v2 rs{1029.23853f, 446.433655f}, re{825.080078f, 717.641113f};

	float lc = (le-ls)^(pt-ls);
	float rc = (pt-rs)^(re-rs);
	printf("lc=%f\nrc=%f\n", lc, rc);

	float w0i0 = (le-ls)^(v2(sx1,sy1)-ls);
	float w1i0 = (v2(sx2,sy2)-rs)^(re-rs);
	float w0dx = ls.y - le.y, w0dy = le.x - ls.x;
	float w1dx = re.y - rs.y, w1dy = rs.x - re.x;

	float rw0 = 0.f, w0i = w0i0;
	for (int qy=int(sy1), cy=1; qy<int(ey1); qy++, cy++) {
		float w0 = w0i;
		for (int qx=int(sx1), cx=0; qx<int(ex1); qx++, cx++) {
			w0 = w0i + w0dx*cx;
			if (qx == int(pt.x) && qy == int(pt.y))
				rw0 = w0;
			//w0 += w0dx;
		}
		//w0i += w0dy;
		w0i = w0i0+w0dy*cy;
	}

	float rw1 = 0.f, w1i = w1i0;
	for (int qy=int(sy2), cy=1; qy<int(ey2); qy++, cy++) {
		float w1 = w1i;
		for (int qx=int(sx2), cx=0; qx<int(ex2); qx++, cx++) {
			w1 = w1i + w1dx*cx;
			if (qx == int(pt.x) && qy == int(pt.y))
				rw1 = w1;
			//w1 += w1dx;
		}
		//w1i += w1dy;
		w1i = w1i0+w1dy*cy;
	}

	float rw0m = w0i0 + w0dy*(int(pt.y) - int(sy1)) + w0dx*(int(pt.x)-int(sx1));
	float rw1m = w1i0 + w1dy*(int(pt.y) - int(sy2)) + w1dx*(int(pt.x)-int(sx2));
	printf("inc: w0 =%f\ninc: w1 =%f\n", rw0, rw1);
	printf("inc: w0m=%f\ninc: w1m=%f\n", rw0m, rw1m);

	float Px = 842.5f, Py = 694.5f;
	float Lsx = 1029.23853f, Lsy = 446.433655f, Lex = 825.080078f, Ley = 717.641113f;
	float Rsx = 1029.23853f, Rsy = 446.433655f, Rex = 825.080078f, Rey = 717.641113f;

	float W0 = (Lex-Lsx)*(Py-Lsy) - (Ley-Lsy)*(Px-Lsx);
	float W1 = (Px-Rsx)*(Rey-Rsy) - (Py-Rsy)*(Rex-Rsx);
	printf("W0=%f\nW1=%f\n", W0, W1);
}

int main() {
	test2();
}