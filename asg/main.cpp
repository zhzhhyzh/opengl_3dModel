
#include <Windows.h>
#include <windowsx.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <map>



#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "glu32.lib")
#define WINDOW_TITLE "Poseidon"

//Mouse Move
static int lastX = -1;
static int lastY = -1;
//projection
bool isOrtho = true;
float ty = 0, tx = 0, tz = 0, tSpeed = 1;
//float ONear = -3, OFar = 3;
float ptx = 0, pty = 0, ptSpeed = 0.1;
float prx = 0, pry = 0, prSpeed = 1;
float zoom = -7;
float rotateX = 0;
float rotateY = 0;
int textureChange = 1;
bool changing = false;
float OLeft = -10.0, ORight = 10.0, ODown = -10.0, OUp = 10.0, ONear = -10.0, OFar = 10.0;
float xPosition = 0.0f, yPosition = 0.0f, zPosition = 0.05f;

//Left leg
float ry = 1, rx = 0, rp = 0; // rotate checking purpose
float txwhole = 0, tywhole = 0, tzwhole = 0; // translate whole body

//Model Line
// top of file with other globals
bool showModelLines = false;   // press V to toggle (debug only)


//Lighting
float amb[3] = { 1.0,0,0 }; // red color ambient light
float posA[3] = { 0.8,0.0,0.0 }; // amb light pos(x,y,z)
float dif[4] = { 1, 1.0, 1.0, 1.0 }; // Green diffuse light (RGBA)
float posD[4] = { 0.0, 0.0, 0.0, 1.0 }; // Diffuse light position (x, y, z, w)
float ambM[3] = { 1.0,0.0,0 }; // blue color ambient material
float difM[3] = { 1.0,1.0,1.0 }; // blue color diffuse material
bool isLightOn = false;
float angle = 0.0;

// --- Arm pose controls (degrees) ---
float L_shoulder_yaw = 8.0f;   // + = swing out to the side
float L_shoulder_pitch = -10.0f;  // + = swing forward
float L_shoulder_roll = 0.0f;   // twist
float L_elbow_flex = 10.0f;  // + = bend elbow

float R_shoulder_yaw = -8.0f;
float R_shoulder_pitch = -10.0f;
float R_shoulder_roll = 0.0f;
float R_elbow_flex = 10.0f;


// ---- Reusable GLU quadric ----
static GLUquadric* gQuadric = nullptr;

static void initQuadric() {
	if (!gQuadric) {
		gQuadric = gluNewQuadric();
		gluQuadricDrawStyle(gQuadric, GLU_FILL);
		gluQuadricDrawStyle(gQuadric, GLU_FILL);
		gluQuadricNormals(gQuadric, GLU_SMOOTH);
		gluQuadricTexture(gQuadric, GL_FALSE);
	}
}

static void destroyQuadric() {
	if (gQuadric) {
		gluDeleteQuadric(gQuadric);
		gQuadric = nullptr;
	}
}


//Bitmap Texture
BITMAP BMP;  //bitmap structure
HBITMAP hBMP = NULL;   ///bitmap handle
int change = 0;

GLuint loadTexture(LPCSTR filename) {

	GLuint texture = 0; //texture name
	//step 3
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	HBITMAP hBMP = (HBITMAP)LoadImage(GetModuleHandle(NULL),
		filename, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION |
		LR_LOADFROMFILE);

	GetObject(hBMP, sizeof(BMP), &BMP);


	//step 4
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
		GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BMP.bmWidth,
		BMP.bmHeight, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, BMP.bmBits);

	DeleteObject(hBMP);

	return texture;
}


LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_LBUTTONDOWN: {
		// Capture the initial X position when the drag starts
		lastX = GET_X_LPARAM(lParam);
		lastY = GET_Y_LPARAM(lParam);
		break;
	}
	case WM_MOUSEMOVE:
		if (wParam & MK_LBUTTON) {  // Ensure the left button is held down (dragging)
			int currentX = GET_X_LPARAM(lParam);  // Get the current X position

			if (lastX != -1) {  // Check if we have a valid previous position
				if (currentX > lastX) {
					// Mouse moved to the right, increase prSpeed
					pry += 1.0f;  // You can adjust the increment value as needed
				}
				else if (currentX < lastX) {
					// Mouse moved to the left, decrease prSpeed
					pry -= 1.0f;  // You can adjust the decrement value as needed
				}
			}

			int currentY = GET_Y_LPARAM(lParam);  // Get the current X position

			if (lastY != -1) {  // Check if we have a valid previous position
				if (currentY > lastY) {
					// Mouse moved to the right, increase prSpeed
					prx += 1.0f;  // You can adjust the increment value as needed
				}
				else if (currentY < lastY) {
					// Mouse moved to the left, decrease prSpeed
					prx -= 1.0f;  // You can adjust the decrement value as needed
				}
			}

			// Update the last X position
			lastX = currentX;
			lastY = currentY;
		}
		break;

	case WM_LBUTTONUP: {
		// Reset last X position when dragging ends
		lastX = -1;
		lastY = -1;
		break;
	}

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) PostQuitMessage(0);
		else if (wParam == VK_SPACE) {
			/*prSpeed = 0.0;
			ptX = 0.0;
			ptY = 0.0;
			ptZ = 0.0;
			LUARotateX = 0.0;
			LUARotateY = 0.0;
			LUARotateZ = 0.0;
			LLARotateX = 0.0;
			LLARotateY = 0.0;
			LLARotateZ = 0.0;
			RUARotateX = 0.0;
			RUARotateY = 0.0;
			RUARotateZ = 0.0;
			RLARotateX = 0.0;
			RLARotateY = 0.0;
			RLARotateZ = 0.0;
			TogFinger = false;
			TogWeapon = false;
			TogOnWeapon = false;
			Act1 = false;
			Act2 = false;
			Act3 = false;
			Slash = false;
			VSlash = false;
			SwingFront = false;
			resetleg();
			txwhole = 6;*/
		}
	
		else if (wParam == 'O')
		{
			isOrtho = true;
			tx = 0, ty = 0, ptx = 0, pty = 0, prx = 0, pry = 0;
		}
		else if (wParam == 'P')
		{
			isOrtho = false;
			tx = 0, ty = 0, ptx = 0, pty = 0, prx = 0, pry = 0;
			zoom = -7;
		}
		else if (wParam == 'A')              //move projection left
		{
			if (isOrtho)
			{
				if (ptx > -0.35)
				{
					ptx -= ptSpeed;
				}
			}
			else
			{
				if (ptx > -0.8)
				{
					ptx -= ptSpeed;
				}
			}
		}
		else if (wParam == 'D')              //move projection right
		{
			if (isOrtho)
			{
				if (ptx < 0.35)
				{
					ptx += ptSpeed;
				}
			}
			else
			{
				if (ptx < 0.8)
				{
					ptx += ptSpeed;
				}
			}
		}
		else if (wParam == 'W')              //move projection up
		{
			if (isOrtho)
			{
				if (pty < 0.35)
				{
					pty += ptSpeed;
				}
			}
			else
			{
				if (pty < 0.8)
				{
					pty += ptSpeed;
				}
			}
		}
		else if (wParam == 'S')              //move projection down
		{
			if (isOrtho)
			{
				if (pty > -0.35)
				{
					pty -= ptSpeed;
				}
			}
			else {
				if (pty > -0.8)
				{
					pty -= ptSpeed;
				}
			}
		}
		else if (wParam == 'T')              //move robot nearer to view
		{
			if (!isOrtho)
			{
				if (zoom < -5.5)
				{
					zoom += 0.5;
				}
			}
		}
		else if (wParam == 'G')              //move robot farther to view
		{
			if (!isOrtho)
			{
				if (zoom > -54)
				{
					zoom -= 0.5;
				}
			}
		}
		else if (wParam == 'N')
		{
			change += 1;
			if (change == 5)
				change = 0;
		}

		//light key
		else if (wParam == 'L') {
			isLightOn = !isLightOn;
		}
		else if (wParam == 'U') {
			posD[1] += 0.06;
		}
		else if (wParam == 'J') {
			posD[1] -= 0.06;
		}
		else if (wParam == 'H') {
			posD[0] -= 0.06;
		}
		else if (wParam == 'K') {
			posD[0] += 0.06;
		}
		else if (wParam == 'Y') {
			posD[2] -= 0.06;
		}
		else if (wParam == 'I') {
			posD[2] += 0.06;
		}
		// inside WM_KEYDOWN
		else if (wParam == 'V') showModelLines = !showModelLines;

		
		break;
	
		

	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}
void projection() {
	glMatrixMode(GL_PROJECTION);			 //refer to the projection matrix
	glLoadIdentity();						 //reset the projection matrix
	glTranslatef(ptx, pty, 0);				 //translate x and y for projection			
	glRotatef(prx, 1, 0, 0);
	glRotatef(pry, 0, 1, 0);	             //rotate y for projection	

	if (isOrtho)
	{
		glOrtho(OLeft, ORight, ODown, OUp, ONear, OFar);
	}
	else
	{
		gluPerspective(90, 1, 1, 100);
		glFrustum(-30.0, 30.0, -30.0, 30.0, -10, 100);
		glTranslatef(0, 0, zoom);
	}
}


void lighting() {
	if (isLightOn) {
		glEnable(GL_LIGHTING);     // enable lighting for the whole scene
	}
	else {
		glDisable(GL_LIGHTING);
	}

	// Rotate the light
	glPushMatrix(); // Save the current transformation matrix
	glRotatef(angle, 0.0f, 0.0f, 1.0f); // Rotate around the Y-axis

	//light 0 : white color ambient light at pos(0,0.8,0), above the sphere
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0, GL_POSITION, posA);
	//glEnable(GL_LIGHT0);

	//light 1 : white color diffuse light at pos(0.8,0.0,0), right the sphere
	glLightfv(GL_LIGHT1, GL_DIFFUSE, dif);
	glLightfv(GL_LIGHT1, GL_POSITION, posD);
	glEnable(GL_LIGHT1);

	glPopMatrix();

}



void drawSphere(float r, int slice, int stack) {
	initQuadric();                      // safe no-op if already inited
	// The sphere will inherit the last style set on gQuadric
	gluSphere(gQuadric, r, slice, stack);
}

struct CapsuleKey {
	float r0, r1, h; int slices;
	bool operator<(const CapsuleKey& o) const {
		if (r0 != o.r0) return r0 < o.r0;
		if (r1 != o.r1) return r1 < o.r1;
		if (h != o.h) return h < o.h;
		return slices < o.slices;
	}
};
static std::map<CapsuleKey, GLuint> gCapsuleDL;

static GLuint getCapsuleList(float r0, float r1, float h, int slices) {
	CapsuleKey k{ r0,r1,h,slices };
	auto it = gCapsuleDL.find(k);
	if (it != gCapsuleDL.end()) return it->second;

	initQuadric();
	GLuint id = glGenLists(1);
	glNewList(id, GL_COMPILE);
	gluCylinder(gQuadric, r0, r1, h, slices, 1);
	// caps
	drawSphere(r0, slices, slices);
	glPushMatrix(); glTranslatef(0, 0, h); drawSphere(r1, slices, slices); glPopMatrix();
	glEndList();
	gCapsuleDL[k] = id;
	return id;
}

static void freeCapsuleLists() {
	for (auto& kv : gCapsuleDL) glDeleteLists(kv.second, 1);
	gCapsuleDL.clear();
}	

//--------------------------------------------------------------------

bool initPixelFormat(HDC hdc)
{
	PIXELFORMATDESCRIPTOR pfd;
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

	pfd.cAlphaBits = 8;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 0;

	pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;

	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;

	// choose pixel format returns the number most similar pixel format available
	int n = ChoosePixelFormat(hdc, &pfd);

	// set pixel format returns whether it sucessfully set the pixel format
	if (SetPixelFormat(hdc, n, &pfd))
	{
		return true;
	}
	else
	{
		return false;
	}
}
//--------------------------------------------------------------------
void action() {
	


}
struct EllipseProfile {
	// y in [0..1] (normalized height), a = half-width (X), b = half-depth (Z), in world units
	float y, a, b;
};

// Safe normalize helper
static void norm3(float& x, float& y, float& z) {
	float m = std::sqrt(x * x + y * y + z * z);
	if (m > 1e-8f) { x /= m; y /= m; z /= m; }
}

// Draw a closed surface from an elliptical profile using triangle strips.
// height = total world height, slices = rings around (e.g., 48..96)
// Caps can be added later if you want a closed neck/hip.
void drawLoftedBody(const EllipseProfile* prof, int n, float height, int slices) {
	if (!prof || n < 2) return;
	glEnable(GL_NORMALIZE);

	// Precompute finite differences a'(y) and b'(y) for normals
	std::vector<float> da(n, 0.0f), db(n, 0.0f);
	for (int i = 0;i < n;i++) {
		int i0 = (((0) > (i - 1)) ? (0) : (i - 1));
		int i1 = (((n - 1) < (i + 1)) ? (n - 1) : (i + 1));
		float dy = (prof[i1].y - prof[i0].y);
		if (std::fabs(dy) < 1e-6f) dy = (i > 0 ? prof[i].y - prof[i - 1].y : prof[i + 1].y - prof[i].y);
		if (std::fabs(dy) < 1e-6f) dy = 1.0f;
		da[i] = (prof[i1].a - prof[i0].a) / dy;
		db[i] = (prof[i1].b - prof[i0].b) / dy;
	}

	// Build strips between rings i and i+1
	for (int i = 0; i < n - 1; ++i) {
		float y0n = prof[i].y, y1n = prof[i + 1].y;
		float a0 = prof[i].a, a1 = prof[i + 1].a;
		float b0 = prof[i].b, b1 = prof[i + 1].b;
		float ya = y0n * height, yb = y1n * height;

		glBegin(GL_TRIANGLE_STRIP);
		for (int s = 0; s <= slices; ++s) {
			float t = (float)s / (float)slices;
			float th = t * 6.28318530718f; // 2π
			float c = std::cos(th);
			float sn = std::sin(th);

			// Ring i+1 (top of strip)
			{
				float a = a1, b = b1;
				float x = a * c, y = yb, z = b * sn;

				// Param tangents: Pθ = (-a sinθ, 0, b cosθ), Py = (a' cosθ, H, b' sinθ)
				float Pthx = -a * sn, Pthy = 0.0f, Pthz = b * c;
				float dady = da[i + 1], dbdy = db[i + 1];
				float Pyx = dady * c, Pyy = height, Pyz = dbdy * sn;

				// Normal ∝ Pθ × Py
				float nx = Pthy * Pyz - Pthz * Pyy;
				float ny = Pthz * Pyx - Pthx * Pyz;
				float nz = Pthx * Pyy - Pthy * Pyx;
				norm3(nx, ny, nz);

				glNormal3f(nx, ny, nz);
				glTexCoord2f(t, y1n);
				glVertex3f(x, y, z);
			}

			// Ring i (bottom of strip)
			{
				float a = a0, b = b0;
				float x = a * c, y = ya, z = b * sn;

				float Pthx = -a * sn, Pthy = 0.0f, Pthz = b * c;
				float dady = da[i], dbdy = db[i];
				float Pyx = dady * c, Pyy = height, Pyz = dbdy * sn;

				float nx = Pthy * Pyz - Pthz * Pyy;
				float ny = Pthz * Pyx - Pthx * Pyz;
				float nz = Pthx * Pyy - Pthy * Pyx;
				norm3(nx, ny, nz);

				glNormal3f(nx, ny, nz);
				glTexCoord2f(t, y0n);
				glVertex3f(x, y, z);
			}
		}
		glEnd();
	}
}

// Draw an ellipsoid by scaling your GLU sphere
void drawEllipsoid(float rx, float ry, float rz, int seg = 24) {
	glPushMatrix();
	glScalef(rx, ry, rz);
	drawSphere(1.0f, seg, seg);
	glPopMatrix();
}

// Sample torso half-depth b(y) from your profile so we can stick details on the surface (front = +Z)
float sampleHalfDepthB(const EllipseProfile* prof, int n, float yn01) {
	if (!prof || n < 2) return 1.0f;
	if (yn01 <= prof[0].y) return prof[0].b;
	if (yn01 >= prof[n - 1].y) return prof[n - 1].b;
	for (int i = 0; i < n - 1; ++i) {
		if (yn01 >= prof[i].y && yn01 <= prof[i + 1].y) {
			float t = (yn01 - prof[i].y) / (prof[i + 1].y - prof[i].y);
			return prof[i].b * (1.0f - t) + prof[i + 1].b * t;
		}
	}
	return prof[n - 1].b;
}

// Draw a guide polyline on the torso surface at fixed theta (e.g., front center = PI/2)
void drawTorsoLineAtTheta(const EllipseProfile* prof, int n, float height, float theta, int steps = 32) {
	//glDisable(GL_LIGHTING);
	glLineWidth(2.0f);
	glColor3f(0, 0, 0);
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < steps; ++i) {
		float yn = (float)i / (float)(steps - 1);
		// linear interpolate a and b
		float a, b;
		if (yn <= prof[0].y) { a = prof[0].a; b = prof[0].b; }
		else if (yn >= prof[n - 1].y) { a = prof[n - 1].a; b = prof[n - 1].b; }
		else {
			for (int k = 0;k < n - 1;++k) {
				if (yn >= prof[k].y && yn <= prof[k + 1].y) {
					float t = (yn - prof[k].y) / (prof[k + 1].y - prof[k].y);
					a = prof[k].a * (1 - t) + prof[k + 1].a * t;
					b = prof[k].b * (1 - t) + prof[k + 1].b * t;
					break;
				}
			}
		}
		float c = cosf(theta), s = sinf(theta);
		float x = a * c;
		float y = yn * height;
		float z = b * s;
		glVertex3f(x, y, z);
	}
	glEnd();
	//glEnable(GL_LIGHTING);
}

// Interpolate half-width a(y) like we already do for b(y)
float sampleHalfWidthA(const EllipseProfile* prof, int n, float yn01) {
	if (!prof || n < 2) return 1.0f;
	if (yn01 <= prof[0].y)   return prof[0].a;
	if (yn01 >= prof[n - 1].y) return prof[n - 1].a;
	for (int i = 0; i < n - 1; ++i) {
		if (yn01 >= prof[i].y && yn01 <= prof[i + 1].y) {
			float t = (yn01 - prof[i].y) / (prof[i + 1].y - prof[i].y);
			return prof[i].a * (1.0f - t) + prof[i + 1].a * t;
		}
	}
	return prof[n - 1].a;
}

// Slight color tweak utility
inline void setSkin(float r, float g, float b) { glColor3f(r, g, b); }

// ---- Skin material presets (light/fair tones) ----
struct SkinPreset {
	GLfloat base[3];    // diffuse/ambient base
	GLfloat areola[3];  // areola ring
	GLfloat nipple[3];  // papilla bump
};

// a few light/fair options — pick one you like
static const SkinPreset SKIN_FAIR_NEUTRAL = {
	{0.93f, 0.78f, 0.70f},   // base
	{0.78f, 0.56f, 0.54f},   // areola
	{0.60f, 0.40f, 0.40f}    // nipple
};
static const SkinPreset SKIN_FAIR_ROSY = {
	{0.94f, 0.80f, 0.76f},
	{0.80f, 0.58f, 0.58f},
	{0.62f, 0.42f, 0.44f}
};
static const SkinPreset SKIN_LIGHT_TAN = {
	{0.89f, 0.75f, 0.63f},
	{0.76f, 0.54f, 0.50f},
	{0.58f, 0.38f, 0.36f}
};

// Apply physically-plausibleish fixed-function material
inline void applySkinMaterial(const GLfloat base[3]) {
	GLfloat amb[4] = { base[0] * 0.55f, base[1] * 0.55f, base[2] * 0.55f, 1.0f };
	GLfloat dif[4] = { base[0],       base[1],       base[2],       1.0f };
	GLfloat spec[4] = { 0.08f, 0.08f, 0.08f, 1.0f }; // soft specular
	GLfloat shin = 12.0f;                         // low shininess

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shin);
}

// Optional: color helper that respects the material
inline void skinColor(GLfloat r, GLfloat g, GLfloat b) { glColor3f(r, g, b); }


// Draw a thin ellipse outline in the local XY plane (slightly offset on +Z)
static void drawEllipseOutlineXY(float rx, float ry, float zOffset, int seg = 42) {
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < seg; ++i) {
		float t = (float)i / (float)seg * 6.28318530718f; // 2π
		glVertex3f(rx * cosf(t), ry * sinf(t), zOffset);
	}
	glEnd();
}


void body() {
	// Pick your light skin preset:
	const SkinPreset SKIN = SKIN_FAIR_NEUTRAL; // or SKIN_FAIR_ROSY / SKIN_LIGHT_TAN

	applySkinMaterial(SKIN.base);
	// If you're relying on glColor, keep it white so material color shows:
	glColor3f(1, 1, 1);
	// Areola
	setSkin(SKIN.areola[0], SKIN.areola[1], SKIN.areola[2]);

	// Nipple (papilla)
	setSkin(SKIN.nipple[0], SKIN.nipple[1], SKIN.nipple[2]);
	skinColor(SKIN.base[0] * 0.98f, SKIN.base[1] * 0.98f, SKIN.base[2] * 0.98f);



	//CC
	// Hips (0.00) → Waist → Chest → Shoulders → Neck base (1.00)
// a = half-width (X), b = half-depth (Z)
	EllipseProfile TORSO[] = {
		{0.00f, 1.40f, 1.10f},  // lower hip
		{0.12f, 1.60f, 1.15f},  // upper hip
		{0.28f, 1.25f, 0.95f},  // waist in
		{0.45f, 1.55f, 1.15f},  // lower ribs
		{0.62f, 1.85f, 1.25f},  // chest
		{0.78f, 2.05f, 1.30f},  // upper chest / shoulders
		{0.90f, 1.80f, 1.10f},  // neck taper starts
		{1.00f, 1.10f, 0.90f}   // neck base
	};
	const int TORSO_N = sizeof(TORSO) / sizeof(TORSO[0]);

	// === PARAMETRIC TORSO ===
	glPushMatrix();
	// Position so hips sit near your “golden belt” height.
	// Your belt center is around y≈0.2..0.5; adjust as you like.
	glTranslatef(0.0f, 0.0f, 0.0f);

	// Material (skin-ish), then outline if you like
	glColor3f(0.90f, 0.70f, 0.65f);
	glDisable(GL_TEXTURE_2D);       // ensure not textured
	//glEnable(GL_LIGHTING);          // shaded
	drawLoftedBody(TORSO, TORSO_N, /*height*/6.2f, /*slices*/72);

	// Optional: wireframe overlay to inspect flow
	//glDisable(GL_LIGHTING);
	if (showModelLines) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glColor3f(0, 0, 0);
		drawLoftedBody(TORSO, TORSO_N, 6.2f, 72);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	//glEnable(GL_LIGHTING);

	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, -0.6f, 0.0f);
	glScalef(1.6f, 0.8f, 1.2f);
	glColor3f(0.83f, 0.69f, 0.22f);
	drawSphere(0.9, 24, 24);     // uses your existing GLU sphere
	glPopMatrix();

	// ---------- ANATOMY DETAILS ON TOP OF THE TORSO ----------
	const float BODY_H = 6.2f;         // must match your drawLoftedBody height
	const int   SLICES = 72;
	// front/oblique guide curves
	if (showModelLines) {
		drawTorsoLineAtTheta(TORSO, TORSO_N, BODY_H, 3.14159265f * 0.5f, 40);
		drawTorsoLineAtTheta(TORSO, TORSO_N, BODY_H, 3.14159265f * 0.32f, 40);
		drawTorsoLineAtTheta(TORSO, TORSO_N, BODY_H, 3.14159265f * 0.68f, 40);

		// lower pec curve
		glLineWidth(1.8f);
		glColor3f(0.15f, 0.15f, 0.15f);
		glBegin(GL_LINE_STRIP);
		// ...
		glEnd();
	}

	// Colors
	const float skinR = 0.90f, skinG = 0.70f, skinB = 0.65f;

	// Reuse the same TORSO[] you already created above in body()

	// === ABS (six-pack) ===
	// We'll place shallow ellipsoids on the FRONT surface (+Z) at three rows.
	// Positions are normalized along height (0..1). Tweak these to match your photos.
	// Abs segments (rows × columns)
	struct AbsSeg { float yn; float xoff; float w; float h; float depth; };

	// Four rows × 2
	AbsSeg abs8[] = {
		{0.28f, -0.55f, 0.50f, 0.30f, 0.15f}, {0.28f, +0.55f, 0.50f, 0.30f, 0.15f},
		{0.36f, -0.50f, 0.52f, 0.32f, 0.16f}, {0.36f, +0.50f, 0.52f, 0.32f, 0.16f},
		{0.44f, -0.48f, 0.55f, 0.34f, 0.17f}, {0.44f, +0.48f, 0.55f, 0.34f, 0.17f},
		{0.52f, -0.45f, 0.60f, 0.36f, 0.18f}, {0.52f, +0.45f, 0.60f, 0.36f, 0.18f},
	};
	int ABS_COUNT = sizeof(abs8) / sizeof(abs8[0]);

	auto placeAbs = [&](const AbsSeg& d) {
		float b = sampleHalfDepthB(TORSO, TORSO_N, d.yn);
		float y = d.yn * BODY_H;
		float z = b * 0.97f;
		float x = d.xoff;

		glPushMatrix();
		glTranslatef(x, y, z);
		glColor3f(skinR, skinG, skinB);
		//glEnable(GL_LIGHTING);
		drawEllipsoid(d.w, d.h, d.depth, 24);

		//if (showModelLines) {
			GLboolean wasLit = glIsEnabled(GL_LIGHTING);
			glDisable(GL_LIGHTING);
			glLineWidth(1.4f);
			glColor3f(0.05f, 0.05f, 0.05f);
			// outline in the ellipsoid's "front" plane (slight +Z so it doesn't z-fight)
			drawEllipseOutlineXY(d.w, d.h, d.depth * 0.04f, 48);
			if (wasLit) glEnable(GL_LIGHTING);
		//}
		glPopMatrix();
		};

	// Render all abs pads
	for (int i = 0;i < ABS_COUNT;i++) placeAbs(abs8[i]);


	// === STERNUM RIDGE (body center line) ===
	// A very thin cylinder along Y near the front; helps read the torso plane.
	// We’ll build it by stacking tiny ellipsoids (keeps consistent with your helpers).
	for (int i = 0; i < 10; ++i) {
		float yn = 0.50f + 0.04f * (i / 9.0f);  // from mid chest to upper chest
		float b = sampleHalfDepthB(TORSO, TORSO_N, yn);
		float y = yn * BODY_H;
		float z = b * 0.995f;                  // almost on the surface
		glPushMatrix();
		glTranslatef(0.0f, y, z);
		glColor3f(skinR, skinG, skinB);
		drawEllipsoid(0.08f, 0.06f, 0.03f, 12);
		glPopMatrix();
	}

	// === DELTOIDS (shoulders) ===
	// Place two ellipsoids near the upper chest/shoulder band.
	// Compute approximate shoulder lateral positions from the profile at yn≈0.78.
	{
		float yn = 0.78f;
		// Interpolate a (half-width) at this height:
		float a_at = TORSO[0].a;
		for (int i = 0;i < TORSO_N - 1;++i) if (yn >= TORSO[i].y && yn <= TORSO[i + 1].y) {
			float t = (yn - TORSO[i].y) / (TORSO[i + 1].y - TORSO[i].y);
			a_at = TORSO[i].a * (1 - t) + TORSO[i + 1].a * t;
			break;
		}
		float b_at = sampleHalfDepthB(TORSO, TORSO_N, yn);
		float y = yn * BODY_H;

		// Left deltoid
		glPushMatrix();
		glTranslatef(-a_at * 0.95f, y, b_at * 0.25f);  // slightly forward
		glRotatef(20.0f, 0, 1, 0);
		glColor3f(skinR, skinG, skinB);
		drawEllipsoid(0.55f, 0.65f, 0.45f, 28);
		glPopMatrix();

		// Right deltoid
		glPushMatrix();
		glTranslatef(+a_at * 0.95f, y, b_at * 0.25f);
		glRotatef(-20.0f, 0, 1, 0);
		glColor3f(skinR, skinG, skinB);
		drawEllipsoid(0.55f, 0.65f, 0.45f, 28);
		glPopMatrix();
	}

	// === OPTIONAL: Guide lines on the torso surface ===
	// Front center meridian (theta = pi/2), and two oblique hints near the edges
	drawTorsoLineAtTheta(TORSO, TORSO_N, BODY_H, 3.14159265f * 0.5f, 40.5); // front
	drawTorsoLineAtTheta(TORSO, TORSO_N, BODY_H, 3.14159265f * 0.32f, 40.5); // front-left
	drawTorsoLineAtTheta(TORSO, TORSO_N, BODY_H, 3.14159265f * 0.68f, 40.5); // front-right
	// ---------- END ANATOMY DETAILS ----------

	// ======== REAL CHEST (pectorals with wider coverage + nipples) ========
	struct PecLobe { float yn; float xsign; float w, h, d; float tiltY; float lift; float fwd; };
	// yn     : normalized vertical placement
	// xsign  : -1 left, +1 right (we mirror lobes)
	// w,h,d  : ellipsoid radii (width, height, depth)
	// tiltY  : yaw toward armpit
	// lift   : extra vertical lift (world units) to shape upper sweep
	// fwd    : push slightly forward (world units) for coverage

	const float sternumY = 0.60f;     // chest vertical anchor
	const float fanUp = 0.05f;     // how much pec sweeps upward near clavicle

	auto placePecLobe = [&](const PecLobe& L) {
		float a = sampleHalfWidthA(TORSO, TORSO_N, L.yn);
		float b = sampleHalfDepthB(TORSO, TORSO_N, L.yn);
		float y = L.yn * BODY_H + L.lift;
		float x = L.xsign * (a * 0.68f);     // reach ~68% of half-width → more lateral coverage
		float z = b * 0.95f + L.fwd;         // hug surface, slight outward bias

		glPushMatrix();
		glTranslatef(x, y, z);
		glRotatef(L.tiltY * L.xsign, 0, 1, 0); // mirror yaw per side
		// Flatten slightly in Z and spread in X to avoid “short ovals”
		glScalef(0.8f, 0.90f, 0.80f);
		setSkin(0.90f, 0.70f, 0.65f);
		drawEllipsoid(L.w, L.h, L.d, 36);
		if (showModelLines) {
			GLboolean wasLit = glIsEnabled(GL_LIGHTING);
			glDisable(GL_LIGHTING);
			glLineWidth(1.6f);
			glColor3f(0.08f, 0.08f, 0.08f);
			// the lobe has a pre-scale; keep outline on the same local XY plane
			drawEllipseOutlineXY(L.w, L.h, L.d * 0.05f, 56);
			if (wasLit) glEnable(GL_LIGHTING);
		}
		glPopMatrix();
		};

	// Three lobes per side: medial (sternal), mid fan, lateral (toward axilla)
	PecLobe PEC_LOBES[] = {
		//        yn     side    w     h     d    tilt  lift   fwd
		{ sternumY + 0.01f, -1,   1.20f, 0.50f, 0.26f,  10.0f, 0.00f,  0.00f }, // L medial
		{ sternumY + 0.03f, -1,   1.30f, 0.55f, 0.27f,  18.0f, fanUp * BODY_H, 0.02f }, // L mid fan (up)
		{ sternumY + 0.00f, -1,   1.10f, 0.48f, 0.25f,  28.0f, 0.00f,  0.02f }, // L lateral (axilla)

		{ sternumY + 0.01f, +1,   1.20f, 0.50f, 0.26f,  10.0f, 0.00f,  0.00f }, // R medial
		{ sternumY + 0.03f, +1,   1.30f, 0.55f, 0.27f,  18.0f, fanUp * BODY_H, 0.02f }, // R mid fan
		{ sternumY + 0.00f, +1,   1.10f, 0.48f, 0.25f,  28.0f, 0.00f,  0.02f }  // R lateral
	};
	for (auto& L : PEC_LOBES) placePecLobe(L);

	// Subtle cleavage ridge
	for (int i = 0;i < 6;i++) {
		float yn = 0.575f + i * 0.018f;
		float b = sampleHalfDepthB(TORSO, TORSO_N, yn);
		float y = yn * BODY_H;
		float z = b * 0.975f;
		glPushMatrix();
		glTranslatef(0.0f, y, z);
		setSkin(0.85f, 0.65f, 0.60f);               // slightly darker
		drawEllipsoid(0.09f, 0.045f, 0.020f, 14);
		glPopMatrix();
	}

	// Lower pec curve (soft guide where pec meets upper abs)
	//glDisable(GL_LIGHTING);
	glLineWidth(1.8f);
	glColor3f(0.15f, 0.15f, 0.15f);
	glBegin(GL_LINE_STRIP);
	for (int s = 0; s <= 28; ++s) {
		float t = (float)s / 28.0f;                  // 0..1 across chest
		float yn = 0.545f + 0.035f * sinf(t * 3.14159f);
		float a = sampleHalfWidthA(TORSO, TORSO_N, yn);
		float b = sampleHalfDepthB(TORSO, TORSO_N, yn);
		float x = (t - 0.5f) * 2.0f * (a * 0.95f);
		float y = yn * BODY_H;
		float z = b * 0.94f;
		glVertex3f(x, y, z);
	}
	glEnd();
	//glEnable(GL_LIGHTING);

	// ======== NIPPLES (areola + papilla) ========
	// Anatomical ballpark: ~0.57–0.60 up the torso height, ~0.5–0.6 of half-width from center.
	struct Nipple { float yn; float xratio; float areolaR; float bumpR; };
	Nipple N = { 0.585f, 0.56f, 0.20f, 0.06f };   // tune xratio for your image 1:1

	auto placeNipple = [&](int side) {
		float yn = N.yn;
		float a = sampleHalfWidthA(TORSO, TORSO_N, yn);
		float b = sampleHalfDepthB(TORSO, TORSO_N, yn);
		float x = (side < 0 ? -1.0f : 1.0f) * (a * N.xratio);
		float y = yn * BODY_H;
		float z = b * 1.25f;

		// Areola (flat-ish disk via very flat ellipsoid)
		glPushMatrix();
		glTranslatef(x, y, z);
		setSkin(0.74f, 0.52f, 0.50f);              // areola tone (slightly darker)
		glScalef(1.0f, 0.85f, 0.55f);
		drawEllipsoid(N.areolaR, N.areolaR * 0.70f, N.areolaR * 0.18f, 20);
		glPopMatrix();

		// Papilla (tiny bump)
		glPushMatrix();
		glTranslatef(x, y, z + N.areolaR * 0.05f);
		setSkin(0.66f, 0.44f, 0.42f);
		drawEllipsoid(N.bumpR * 0.9f, N.bumpR * 0.7f, N.bumpR * 0.5f, 18);
		glPopMatrix();
		};

	// Left (-1) and Right (+1)
	placeNipple(-1);
	placeNipple(+1);
	// ======== END REAL CHEST ========



}

// ============================
// ARM MODULE (natural + mirrored)
// deps from your file: drawSphere(...), drawEllipsoid(...),
//                      applySkinMaterial(...), SkinPreset presets
// ============================

// --- Use torso profile so shoulders sit in the right place ---
static void getTorsoProfile(const EllipseProfile*& prof, int& N, float& BODY_H) {
	static EllipseProfile TORSO[] = {
		{0.00f, 1.40f, 1.10f}, {0.12f, 1.60f, 1.15f}, {0.28f, 1.25f, 0.95f},
		{0.45f, 1.55f, 1.15f}, {0.62f, 1.85f, 1.25f}, {0.78f, 2.05f, 1.30f},
		{0.90f, 1.80f, 1.10f}, {1.00f, 1.10f, 0.90f}
	};
	prof = TORSO; N = (int)(sizeof(TORSO) / sizeof(TORSO[0]));
	BODY_H = 6.2f;
}
static float sampleA(const EllipseProfile* prof, int n, float yn) { return sampleHalfWidthA(prof, n, yn); }
static float sampleB(const EllipseProfile* prof, int n, float yn) { return sampleHalfDepthB(prof, n, yn); }

static void drawEllipseLoopXY(float rx, float ry, float z, int seg = 64) {
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < seg; ++i) {
		float t = (float)i * (6.28318530718f / (float)seg);
		glVertex3f(rx * cosf(t), ry * sinf(t), z);
	}
	glEnd();
}

static void drawCircleXY(float r, float z, int seg = 64) {
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < seg; ++i) {
		float t = (float)i / (float)seg * 6.28318530718f;
		glVertex3f(r * cosf(t), r * sinf(t), z);
	}
	glEnd();
}

static void drawCapsuleMeridiansPolyline(float r0, float rMid, float r1,
	float h, int meridians = 8) {
	// Draw simple polylines passing through (z=0, z=h/2, z=h) at fixed angle.
	// Not a perfect geodesic over spherical caps, but great as a guide.
	const float midZ = h * 0.5f;
	for (int k = 0; k < meridians; ++k) {
		float th = (float)k * (6.28318530718f / (float)meridians);
		float cx = cosf(th), sx = sinf(th);

		glBegin(GL_LINE_STRIP);
		glVertex3f(r0 * cx, r0 * sx, 0.0f);
		glVertex3f(rMid * cx, rMid * sx, midZ);
		glVertex3f(r1 * cx, r1 * sx, h);
		glEnd();
	}
}

static void drawCapsuleWireOverlayZ(float r0, float r1, float h) {
	// Slight lift to avoid z-fighting with the filled mesh
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.0008f);

	// Choose a mid radius for the middle ring (linear blend is fine for guides)
	float rMid = r0 * 0.5f + r1 * 0.5f;

	glLineWidth(1.2f);
	glColor3f(0.04f, 0.04f, 0.04f);

	// 3 rings (bottom, mid, top)
	drawCircleXY(r0, 0.0f, 72);
	drawCircleXY(rMid, h * 0.5f, 72);
	drawCircleXY(r1, h, 72);

	// Meridians
	drawCapsuleMeridiansPolyline(r0, rMid, r1, h, /*meridians*/ 10);

	glPopMatrix();
}

// ===== Your draw functions with overlay hook =====

static void drawCapsuleAlongZ(float r0, float r1, float h, int slices = 28) {
	// Solid (cached display list)
	glCallList(getCapsuleList(r0, r1, h, slices));

	// Optional model lines
	if (showModelLines) {
		GLboolean wasLit = glIsEnabled(GL_LIGHTING);
		glDisable(GL_LIGHTING);
		drawCapsuleWireOverlayZ(r0, r1, h);
		if (wasLit) glEnable(GL_LIGHTING);
	}
}

static void drawCapsuleDownY(float rTop, float rBot, float length) {
	glPushMatrix();
	glRotatef(-90.0f, 1, 0, 0);
	drawCapsuleAlongZ(rTop, rBot, length); // inherits overlay when enabled
	glPopMatrix();
}
// --- Slight darken for creases ---
static void pushDarken(float k = 0.96f) {
	GLfloat amb[4], dif[4], spec[4];
	glGetMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	glGetMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
	glGetMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
	for (int i = 0;i < 3;i++) { amb[i] *= k; dif[i] *= k; }
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dif);
}

// --- Helper: know if lighting is on (to choose glColor vs material) ---
static inline bool lightingEnabled() { return glIsEnabled(GL_LIGHTING) == GL_TRUE; }

// === FINGER (3 segments, natural curl) ===
static void drawFingerSegmented(float scale = 1.0f,
	float curl1 = 20, float curl2 = 25, float curl3 = 15) {


	// proportions
	const float L1 = 0.45f * scale;  // proximal
	const float L2 = 0.40f * scale;  // middle
	const float L3 = 0.35f * scale;  // distal
	const float R1 = 0.11f * scale;
	const float R2 = 0.095f * scale;
	const float R3 = 0.085f * scale;

	// proximal
	drawCapsuleDownY(R1, R1 * 0.9f, L1);
	glTranslatef(0, -L1, 0);
	glRotatef(curl1, 1, 0, 0);

	// middle
	drawCapsuleDownY(R2, R2 * 0.9f, L2);
	glTranslatef(0, -L2, 0);
	glRotatef(curl2, 1, 0, 0);

	// distal
	drawCapsuleDownY(R3, R3 * 0.9f, L3);
	glTranslatef(0, -L3, 0);
	glRotatef(curl3, 1, 0, 0);

	// fingertip pad
	glPushMatrix();
	glTranslatef(0, 0.3, 0);
	glScalef(1.0f, 0.9f, 0.85f);
	drawSphere(R3 * 0.9f, 12, 12);
	glPopMatrix();
}

// === HAND (palm + 4 fingers + thumb) ===
static void drawHandNatural(int side /* -1 left, +1 right */) {
	const SkinPreset& SKIN = SKIN_LIGHT_TAN;         // light tan, as requested
	applySkinMaterial(SKIN.base);                    // ambient/diffuse/specular
	glEnable(GL_NORMALIZE);                          // safe normals after scales
	glDisable(GL_TEXTURE_2D);                        // no textures on skin

	if (glIsEnabled(GL_LIGHTING)) {
		glColor3f(1.0f, 1.0f, 1.0f);                // let material color show
	}
	else {
		glColor3f(SKIN.base[0], SKIN.base[1], SKIN.base[2]);  // flat skin tone
	}

	const float PALM_W = 0.9f;
	const float PALM_H = 0.6f;
	const float PALM_T = 0.35f;

	// Palm block
	glPushMatrix();
	glScalef(PALM_T, PALM_H * 0.4f, PALM_W * 0.6f);
	drawSphere(1.0f, 20, 20);
	glPopMatrix();

	// Move to knuckle line
	glTranslatef(0, -PALM_H * 0.45f, 0);

	// Four fingers
	float zStart = -PALM_W * 0.45f;
	float zStep = PALM_W * 0.3f;
	for (int i = 0; i < 4; i++) {
		float z = zStart + i * zStep;
		float scale = 1.0f - 0.08f * i;  // shorter pinky
		glPushMatrix();
		glTranslatef(0, 0, z);
		drawFingerSegmented(scale, 18 + 2 * i, 22 + i, 12 + i);
		glPopMatrix();
	}

	// Thumb
	glPushMatrix();
	glTranslatef(side * 0.1f, -PALM_H * 0.2f, -PALM_W * -0.2f);
	glRotatef(side * -20.0f, 0, 0, 1);
	drawFingerSegmented(0.85f, 15, 18, 12);
	glPopMatrix();
}


// --- Natural finger (MCP -> PIP -> DIP) ---
static void drawFingerNatural(float splayDeg, float yawDeg,
	float rollDeg, float curl1, float curl2, float curl3,
	float scale = 1.0f)
{
	const float L1 = 0.55f * scale, L2 = 0.45f * scale, L3 = 0.38f * scale;
	const float R1 = 0.115f * scale, R2 = 0.100f * scale, R3 = 0.085f * scale;

	glPushMatrix();
	glRotatef(splayDeg, 0, 1, 0);   // spread
	glRotatef(yawDeg, 0, 1, 0);   // twist
	glRotatef(rollDeg, 0, 0, 1);   // lateral

	// proximal
	drawCapsuleDownY(R1, R1 * 0.92f, L1);
	glTranslatef(0, -L1, 0); glRotatef(curl1, 1, 0, 0);

	// middle
	drawCapsuleDownY(R2, R2 * 0.90f, L2);
	glTranslatef(0, -L2, 0); glRotatef(curl2, 1, 0, 0);

	// distal
	drawCapsuleDownY(R3, R3 * 0.88f, L3);
	glTranslatef(0, -L3, 0);
	glPushMatrix(); // fingertip pad
	glTranslatef(0, -0.03f * scale, 0.02f * scale);
	glScalef(1.0f, 0.75f, 0.85f);
	drawSphere(R3 * 0.95f, 14, 14);
	glPopMatrix();

	glRotatef(curl3, 1, 0, 0);
	glPopMatrix();
}

// --- Hand (palm + 4 fingers + thumb). side = -1 left, +1 right ---

// --- Full arm resting down the side. side = -1 left, +1 right ---
static void drawArmDown(int side)
{
	const EllipseProfile* T; int TN; float BODY_H;
	getTorsoProfile(T, TN, BODY_H);

	// skin material (match body)
	const SkinPreset& SKIN = SKIN_LIGHT_TAN;
	applySkinMaterial(SKIN.base);
	glDisable(GL_TEXTURE_2D);
	if (!lightingEnabled()) glColor3f(SKIN.base[0], SKIN.base[1], SKIN.base[2]); else glColor3f(1, 1, 1);

	// shoulder placement (sample at upper chest band)
	const float SHOULDER_YN = 0.48f;
	const float a = sampleA(T, TN, SHOULDER_YN);
	const float b = sampleB(T, TN, SHOULDER_YN);
	const float yS = SHOULDER_YN * BODY_H;
	const float xS = side * (a * 1.4f);
	const float zS = b * 0.20f;

	// segment dims
	const float UPPER_LEN = 2.40f, FORE_LEN = 2.35f;
	const float SHOULDER_R = 0.42f;
	const float UPPER_R0 = 0.38f, UPPER_R1 = 0.3f;
	const float ELBOW_R = 0.30f;
	const float FORE_R0 = 0.30f, FORE_R1 = 0.22f;
	const float WRIST_R = 0.18f;

	// natural pose
	const float ARM_YAW = 6.0f;  // slight inward
	const float ARM_PITCH = 4.0f; // slight forward

	glPushMatrix();
	glTranslatef(xS, yS, zS);

	// shoulder cap
	glPushMatrix(); glTranslatef(0, 0.08f, 0);
	drawEllipsoid(SHOULDER_R * 1.05f, SHOULDER_R * 0.95f, SHOULDER_R * 1.00f, 28);
	glPopMatrix();

	// aim the arm
	glRotatef(ARM_YAW * (float)side, 0, 1, 0);
	glRotatef(ARM_PITCH, 1, 0, 0);

	// upper arm
	pushDarken(0.985f);
	drawCapsuleDownY(UPPER_R0, UPPER_R1, UPPER_LEN);

	// elbow
	glTranslatef(0, -UPPER_LEN, 0);
	drawSphere(ELBOW_R, 20, 20);

	// forearm (slight pronation)
	glRotatef(8.0f * (float)side, 0, 1, 0);
	drawCapsuleDownY(FORE_R0, FORE_R1, FORE_LEN);

	// Wrist
	glTranslatef(0.0f, -0.5f, 0.0f);
	drawSphere(0.25f, 18, 18);

	// Hand pose
	glTranslatef(0, -0.3f, 0);
	glRotatef(-10, 1, 0, 0);           // wrist flex
	glRotatef(8 * side, 0, 0, 1);      // roll inward

	drawHandNatural(side);

	glPopMatrix();
}

// Public wrappers you call in your scene:
void leftarm() { drawArmDown(-1); }
void rightarm() { drawArmDown(+1); }


// ==================================================================
// ======= LEG HELPERS =====================================================

// Foot: slightly blocky forefoot + rounded heel
// -----------------------------
// FOOT (rounded heel + arch + toes)
// -----------------------------
static void drawFoot(int side /*-1 left, +1 right*/) {
	// world units (tweak to taste)
	const float LEN = 1.55f;  // heel→toe (local -Y)
	const float WIDTH = 0.78f;  // across X
	const float THICK = 0.36f;  // sole thickness (Z)

	glPushMatrix();
	glRotatef(-90, 1, 0, 0);                // make our +Z become -Y (down)

	// slight turnout like a relaxed stance
	glRotatef(6.0f * (float)side, 0, 1, 0);

	// Heel pad (rounded block)
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -THICK * 0.10f);
	glScalef(WIDTH * 0.48f, 0.40f, THICK * 0.55f);
	drawSphere(1.0f, 18, 18);
	glPopMatrix();

	// Arch / midfoot bridge (slightly thinner)
	glPushMatrix();
	glTranslatef(0.0f, -LEN * 0.42f, 0.0f);
	glScalef(WIDTH * 0.38f, LEN * 0.34f, THICK * 0.32f);
	drawSphere(1.0f, 18, 18);
	glPopMatrix();

	// Ball of foot
	glPushMatrix();
	glTranslatef(0.0f, -LEN * 0.78f, 0.0f);
	glScalef(WIDTH * 0.46f, LEN * 0.22f, THICK * 0.35f);
	drawSphere(1.0f, 18, 18);
	glPopMatrix();

	// Toes lump (slight downward taper)
	glPushMatrix();
	glTranslatef(0.0f, -LEN * 0.98f, -THICK * 0.06f);
	glScalef(WIDTH * 0.44f, LEN * 0.10f, THICK * 0.26f);
	drawSphere(1.0f, 16, 16);
	glPopMatrix();

	// Malleolus bumps (ankle bones) hint
	glPushMatrix();
	glTranslatef(+WIDTH * 0.40f * (float)side, -LEN * 0.02f, -THICK * 0.05f);
	drawEllipsoid(0.07f, 0.09f, 0.07f, 12);
	glPopMatrix();

	glPopMatrix();
}

// ---- SARONG (towel) helpers -----------------------------------------------

// A thin elliptical ring around the pelvis (belt)
static void drawSarongBelt(float a, float b, float thickness) {
	const int slices = 48;
	const float h = thickness;
	glPushMatrix();
	// build a short cylinder by sweeping an ellipse
	for (int i = 0; i < slices; ++i) {
		float t0 = (float)i / slices * 6.28318530718f;
		float t1 = (float)(i + 1) / slices * 6.28318530718f;
		float c0 = cosf(t0), s0 = sinf(t0);
		float c1 = cosf(t1), s1 = sinf(t1);
		float x0 = a * c0, z0 = b * s0;
		float x1 = a * c1, z1 = b * s1;

		glBegin(GL_QUADS);
		glNormal3f(c0, 0, s0); glVertex3f(x0, 0, z0);
		glNormal3f(c0, 0, s0); glVertex3f(x0, -h, z0);
		glNormal3f(c1, 0, s1); glVertex3f(x1, -h, z1);
		glNormal3f(c1, 0, s1); glVertex3f(x1, 0, z1);
		glEnd();
	}
	glPopMatrix();
}

// A rectangular cloth panel that hangs down along -Y.
// width is along X (wrap), thickness is along Z.
static void drawSarongPanel(float halfW, float len, float halfT) {
	// very simple double-sided quad box (thin)
	glPushMatrix();
	glBegin(GL_QUADS);
	// front
	glNormal3f(0, 0, 1);
	glVertex3f(-halfW, 0, halfT);
	glVertex3f(halfW, 0, halfT);
	glVertex3f(halfW, -len, halfT);
	glVertex3f(-halfW, -len, halfT);
	// back
	glNormal3f(0, 0, -1);
	glVertex3f(halfW, 0, -halfT);
	glVertex3f(-halfW, 0, -halfT);
	glVertex3f(-halfW, -len, -halfT);
	glVertex3f(halfW, -len, -halfT);
	// left edge
	glNormal3f(-1, 0, 0);
	glVertex3f(-halfW, 0, -halfT);
	glVertex3f(-halfW, 0, halfT);
	glVertex3f(-halfW, -len, halfT);
	glVertex3f(-halfW, -len, -halfT);
	// right edge
	glNormal3f(1, 0, 0);
	glVertex3f(halfW, 0, halfT);
	glVertex3f(halfW, 0, -halfT);
	glVertex3f(halfW, -len, -halfT);
	glVertex3f(halfW, -len, halfT);
	glEnd();
	glPopMatrix();
}

// One straight dangling leg, anchored to torso silhouette.
// side = -1 (left) or +1 (right)
// One straight dangling leg, anchored to torso silhouette, with a sarong panel that follows the leg.
// side = -1 (left) or +1 (right)
// -----------------------------
// LEG DOWN + SARONG-FOLLOWING PANEL
// side = -1 left, +1 right
// -----------------------------
static void drawLegDown(int side, float& outPelvisY, float& outPelvisA, float& outPelvisB) {
	const EllipseProfile* T; int TN; float BODY_H;
	getTorsoProfile(T, TN, BODY_H);

	// match body skin
	const SkinPreset& SKIN = SKIN_LIGHT_TAN;
	applySkinMaterial(SKIN.base);
	glDisable(GL_TEXTURE_2D);
	if (glIsEnabled(GL_LIGHTING)) glColor3f(1, 1, 1); else glColor3f(SKIN.base[0], SKIN.base[1], SKIN.base[2]);

	// anchor to hip ring
	const float HIP_YN = 0.16f;
	const float aHip = sampleA(T, TN, HIP_YN);
	const float bHip = sampleB(T, TN, HIP_YN);
	const float yHip = HIP_YN * BODY_H;
	const float xHip = side * (aHip * 0.50f);
	const float zHip = bHip * 0.12f;

	// expose pelvis ring for belt/back panel
	outPelvisY = yHip;
	outPelvisA = aHip * 0.98f;
	outPelvisB = bHip * 0.98f;

	// proportions
	const float THIGH_LEN = 2.70f;
	const float SHIN_LEN = 2.60f;      // fixed (was 0.65f by mistake)
	const float HIP_R = 0.62f;
	const float THIGH_R0 = 0.62f, THIGH_R1 = 0.46f;
	const float KNEE_R = 0.40f;
	const float CALF_R0 = 0.44f, CALF_R1 = 0.30f; // calf bulge -> ankle
	const float ANKLE_R = 0.24f;

	// relaxed pose
	const float LEG_YAW = 5.0f * (float)side;   // slight turnout
	const float LEG_PITCH = 3.5f;                 // a touch forward

	glPushMatrix();
	glTranslatef(xHip, yHip, zHip);

	// blend deltoid-like hip cap into torso (prevents gap)
	glPushMatrix();
	glTranslatef(0.0f, 0.08f, -0.03f);
	drawEllipsoid(HIP_R * 1.02f, HIP_R * 0.96f, HIP_R * 1.02f, 28);
	glPopMatrix();

	// aim the leg
	glRotatef(LEG_YAW, 0, 1, 0);
	glRotatef(LEG_PITCH, 1, 0, 0);

	// --- sarong panel that follows this leg ---
	glPushMatrix();
	glTranslatef(0.2f * side, 0.0f, 0.2f);
	glRotatef(8.0f * side, 0, 1, 0);
	glColor3f(0.83f, 0.69f, 0.22f);
	drawSarongPanel(/*halfW*/0.53f, /*len*/THIGH_LEN * 0.75f, /*halfT*/0.53f);
	// restore skin color for the leg
	if (glIsEnabled(GL_LIGHTING)) glColor3f(1, 1, 1); else glColor3f(SKIN.base[0], SKIN.base[1], SKIN.base[2]);
	glPopMatrix();
	// -----------------------------------------

	// THIGH (slight inward taper)
	pushDarken(0.985f);
	drawCapsuleDownY(THIGH_R0, THIGH_R1, THIGH_LEN);

	// KNEE joint sphere + patella hint
	glTranslatef(0, -THIGH_LEN, 0);
	drawSphere(KNEE_R, 20, 20);
	glPushMatrix();
	glTranslatef(0.0f, -KNEE_R * 0.15f, KNEE_R * 0.45f);   // patella forward
	drawEllipsoid(0.18f, 0.14f, 0.10f, 16);
	glPopMatrix();

	// CALF / SHIN (calf bulge high, taper to ankle)
	glRotatef(1.5f, 1, 0, 0);
	drawCapsuleDownY(CALF_R0, CALF_R1, SHIN_LEN * 0.55f);
	// slight second segment to reach ankle length and create nicer curve
	glTranslatef(0, -(SHIN_LEN * 0.55f), 0);
	drawCapsuleDownY(CALF_R1, ANKLE_R * 0.95f, SHIN_LEN * 0.45f);

	// ANKLE

	glTranslatef(0, -(SHIN_LEN * -0.05f), 0);
	glScalef(1,2.5f,0.9);

	drawSphere(ANKLE_R, 18, 18);

	// FOOT
	drawFoot(side);

	glPopMatrix();
}

// Elliptical frustum skirt (top ellipse -> bottom ellipse), with optional caps.
static void drawEllipticFrustum(float yTop, float yBot,
	float aTop, float bTop,
	float aBot, float bBot,
	int slices = 64)
{
	glPushMatrix();

	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i <= slices; ++i) {
		float t = (float)i / (float)slices * 6.28318530718f; // 2π
		float c = cosf(t), s = sinf(t);

		// top rim
		glNormal3f(c, 0, s);
		glVertex3f(aTop * c, yTop, bTop * s);

		// bottom rim
		glNormal3f(c, 0, s);
		glVertex3f(aBot * c, yBot, bBot * s);
	}
	glEnd();

	// small top cap (prevents seeing inside when camera looks downward)
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0, 1, 0);
	glVertex3f(0, yTop, 0);
	for (int i = 0; i <= slices; ++i) {
		float t = (float)i / (float)slices * 6.28318530718f;
		glVertex3f(aTop * cosf(t), yTop, bTop * sinf(t));
	}
	glEnd();

	// tiny hem roll (visual thickness at the bottom)
	float hemA = aBot * 1.015f, hemB = bBot * 1.015f;
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i <= slices; ++i) {
		float t = (float)i / (float)slices * 6.28318530718f;
		float c = cosf(t), s = sinf(t);
		glNormal3f(c, 0, s);
		glVertex3f(aBot * c, yBot, bBot * s);
		glNormal3f(c, 0, s);
		glVertex3f(hemA * c, yBot - 0.02f, hemB * s);
	}
	glEnd();

	glPopMatrix();
}
// A skirt that hugs the torso at its top, then flares slightly toward the legs.
static void drawSarongHuggingTorso(float pelvisY)
{
	// color/material — same gold you use for the sarong
	glColor3f(0.83f, 0.69f, 0.22f);
	glDisable(GL_TEXTURE_2D);

	// pull torso profile to compute ellipses
	const EllipseProfile* T; int TN; float BODY_H;
	getTorsoProfile(T, TN, BODY_H);

	// choose the top at the pelvis/hip band; bottom several units down
	float yTop = pelvisY + 0.02f;   // lift a hair to avoid z-fight
	float yBot = pelvisY - 6.0f;    // hem (adjust to taste)

	// normalized heights for sampling
	float ynTop = yTop / BODY_H;
	float ynBot = (((0.0f) > ((((1.0f) < ((pelvisY - 0.12f) / BODY_H)) ? (1.0f) : ((pelvisY - 0.12f) / BODY_H)))) ? (0.0f) : ((((1.0f) < ((pelvisY - 0.12f) / BODY_H)) ? (1.0f) : ((pelvisY - 0.12f) / BODY_H))));

	// sample torso half-width/depth and add small safety inflation to kill gaps
	float aTop = sampleHalfWidthA(T, TN, ynTop) * 1.020f;
	float bTop = sampleHalfDepthB(T, TN, ynTop) * 1.020f;

	// bottom ellipse — a little wider so it covers the thighs
	float aBot = sampleHalfWidthA(T, TN, ynBot) * 1.18f;
	float bBot = sampleHalfDepthB(T, TN, ynBot) * 1.18f;

	// draw the frustum
	drawEllipticFrustum(yTop, yBot, aTop, bTop, aBot, bBot, 72);

	// Optional: show model lines for the sarong (V to toggle)
	if (showModelLines) {
		glDisable(GL_LIGHTING);
		glLineWidth(1.2f);
		glColor3f(0, 0, 0);

		// two outlines: top rim and bottom rim
		auto rim = [](float y, float a, float b, int seg) {
			glBegin(GL_LINE_LOOP);
			for (int i = 0; i < seg; ++i) {
				float t = (float)i / (float)seg * 6.28318530718f;
				glVertex3f(a * cosf(t), y, b * sinf(t));
			}
			glEnd();
			};
		rim(yTop, aTop, bTop, 72);
		rim(yBot, aBot, bBot, 72);

		// a few vertical “seams”
		glBegin(GL_LINES);
		for (int k = 0; k < 8; ++k) {
			float t = (float)k * (6.28318530718f / 8.0f);
			float c = cosf(t), s = sinf(t);
			glVertex3f(aTop * c, yTop, bTop * s);
			glVertex3f(aBot * c, yBot, bBot * s);
		}
		glEnd();

		glEnable(GL_LIGHTING);
	}
}



static float gPelvisY = 0.0f, gPelvisA = 0.5f, gPelvisB = 0.5f;

void leftleg() { drawLegDown(-1, gPelvisY, gPelvisA, gPelvisB); }
void rightleg() { drawLegDown(+1, gPelvisY, gPelvisA, gPelvisB); }


// ===== HEAD (skull + face planes + ears + neck) =====================

// simple ellipse wire in local XY (used for "V" guides)
static void headEllipseXY(float rx, float ry, float z=0.0f, int seg=72){
	glBegin(GL_LINE_LOOP);
	for(int i=0;i<seg;++i){
		float t = (float)i/seg * 6.28318530718f;
		glVertex3f(rx*cosf(t), ry*sinf(t), z);
	}
	glEnd();
}

// longitudinal guides on an ellipsoid (very light, good for modeling)
static void headGuides(float rx, float ry, float rz){
	GLboolean lit = glIsEnabled(GL_LIGHTING);
	glDisable(GL_LIGHTING);
	glLineWidth(1.2f);
	glColor3f(0.04f,0.04f,0.04f);

	// “latitude” rings
	glPushMatrix();                     // equator
	headEllipseXY(rx, rz);              // XY→(x,z); at y=0
	glPopMatrix();

	for(int k=1;k<=3;++k){              // a few above and below
		float f=(float)k/4.0f;
		float yy =  ry*(0.55f*f);       // y offsets
		float s  =  sqrtf(1.0f - (yy/ry)*(yy/ry));
		headEllipseXY(rx*s, rz*s, 0.0f);            // +y ring
		glPushMatrix();
		glTranslatef(0.0f, -yy*2.0f, 0.0f);         // mirror to -y
		headEllipseXY(rx*s, rz*s, 0.0f);
		glPopMatrix();
	}

	// “meridians” (front–back & diagonals)
	for(int m=0;m<6;++m){
		float th = (float)m * (6.28318530718f/6.0f);
		float cx = cosf(th), sx = sinf(th);

		glBegin(GL_LINE_STRIP);
		for(int i=0;i<=48;++i){
			float t = -1.0f + 2.0f*(float)i/48.0f;  // -1..+1 latitude
			float y = ry * t;
			float s = sqrtf(1.0f - (y/ry)*(y/ry));
			float x = rx * (cx*s);
			float z = rz * (sx*s);
			glVertex3f(x,y,z);
		}
		glEnd();
	}

	if(lit) glEnable(GL_LIGHTING);
}

// tiny helper: ear (flattened ellipsoid, slight tilt)
static void drawEar(float side /*-1 left, +1 right*/, float rx, float ry, float rz){
	glPushMatrix();
	glRotatef(10.0f*side, 0,1,0);           // yaw open
	glRotatef(8.0f, 0,0,1);                 // little roll
	glScalef(0.85f,1.0f,0.72f);             // flatten a touch
	drawEllipsoid(rx,ry,rz,22);
	glPopMatrix();
}

// short neck as a capsule that grows upward (+Y)
static void drawNeck(float rTop, float rBot, float len){
	glPushMatrix();
	glRotatef(+90.0f, 1,0,0);               // build along +Z
	glCallList(getCapsuleList(rTop, rBot, len, 28));
	glPopMatrix();


}
// Filled disk facing +Z (for iris/pupil/spec highlight)
static void drawDiskZ(float r, float z, int seg = 40) {
	glBegin(GL_TRIANGLE_FAN);
	glVertex3f(0, 0, z);
	for (int i = 0;i <= seg;++i) {
		float t = (float)i / seg * 6.28318530718f;
		glVertex3f(r * cosf(t), r * sinf(t), z);
	}
	glEnd();
}

// Thin ring facing +Z (an iris ring look)
static void drawRingZ(float r0, float r1, float z, int seg = 40) {
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0;i <= seg;++i) {
		float t = (float)i / seg * 6.28318530718f;
		float c = cosf(t), s = sinf(t);
		glVertex3f(r0 * c, r0 * s, z);
		glVertex3f(r1 * c, r1 * s, z);
	}
	glEnd();
}

// Main head builder. Places head on top of your parametric torso.
void head(){
	// pull torso profile for exact neck placement
	const EllipseProfile* T; int TN; float BODY_H;
	getTorsoProfile(T, TN, BODY_H);

	// sample at the neck base (yn=1)
	const float aNeck = sampleHalfWidthA(T, TN, 1.0f);
	const float bNeck = sampleHalfDepthB(T, TN, 1.0f);
	const float neckY = BODY_H;                 // top of torso module

	// materials/colors consistent with body
	const SkinPreset& SKIN = SKIN_LIGHT_TAN;
	applySkinMaterial(SKIN.base);
	if (glIsEnabled(GL_LIGHTING)) glColor3f(1,1,1);
	else                           glColor3f(SKIN.base[0],SKIN.base[1],SKIN.base[2]);
	glDisable(GL_TEXTURE_2D);

	// overall proportions (tuned to your screenshot scale)
	// --- proportions (bigger skull) ---
	const float NECK_R0 = 0.48f;   // bottom at torso (fatter)
	const float NECK_R1 = 0.42f;   // top toward skull
	const float NECK_L = 0.72f;   // slightly shorter

	const float SKULL_RX = 1.15f;  // wider
	const float SKULL_RY = 1.55f;  // taller
	const float SKULL_RZ = 1.15f;  // deeper


	glPushMatrix();
	// anchor at torso neck hole
	glTranslatef(0.0f, neckY - 7.08f, 0.0f);
	glTranslatef(0.0f, NECK_L + SKULL_RY * 0.28f, 0.0f);

	glTranslatef(0.0f, neckY, 0.0f);

	// slight forward offset so jaw sits a hair in front of torso line
	glTranslatef(0.0f, 0.0f, bNeck*0.04f);

	// ================= Neck =================
	glPushMatrix();
	// center neck, slight forward lean
	glRotatef(3.0f, 1,0,0);
	drawNeck(NECK_R0, NECK_R1, NECK_L);
	glPopMatrix();
	// ================= Collar gasket (overlap into torso) =================
	
		// top just above torso surface, bottom a bit inside the torso hole
		const float yTop = +0.02f;
		const float yBot = -0.28f;

		// start a tad larger than the torso neck ellipse, then flare slightly
		const float aTop = aNeck * 0.835f;
		const float bTop = bNeck * 0.835f;
		const float aBot = aNeck * 0.885f;
		const float bBot = bNeck * 0.885f;

		GLboolean wasTex = glIsEnabled(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_2D);
		if (glIsEnabled(GL_LIGHTING)) glColor3f(1, 1, 1); else glColor3f(SKIN.base[0], SKIN.base[1], SKIN.base[2]);

		glPushMatrix();
		// build the tiny frustum around the neck root (current matrix is at neck base)
		glBegin(GL_TRIANGLE_STRIP);
		const int slices = 48;
		for (int i = 0; i <= slices; ++i) {
			float t = (float)i / (float)slices * 6.28318530718f;
			float c = cosf(t), s = sinf(t);
			glNormal3f(c, 0, s);  glVertex3f(aTop * c, yTop, bTop * s);
			glNormal3f(c, 0, s);  glVertex3f(aBot * c, yBot, bBot * s);
		}
		glEnd();
		glPopMatrix();

		if (wasTex) glEnable(GL_TEXTURE_2D);

		// (optional) show its rim in V wireframe mode
		
	// raise to skull center
	glTranslatef(0.0f, NECK_L + SKULL_RY*0.30f, 0.0f);

	// ================= Skull =================
	glPushMatrix();
	drawEllipsoid(SKULL_RX, SKULL_RY, SKULL_RZ, 36);
	//if (showModelLines) headGuides(SKULL_RX, SKULL_RY, SKULL_RZ);
	glPopMatrix();

	const float eyeY = -SKULL_RY * 0.28f;
	const float eyeZ = SKULL_RZ * 0.86f;     // on the face plane
	const float eyeDX = SKULL_RX * 0.44f;     // left/right offset
	const float sclRX = 0.25f, sclRY = 0.18f, sclRZ = 0.14f;

	auto oneEye = [&](float side) {
		glPushMatrix();
		glTranslatef(side * eyeDX, eyeY, eyeZ);

		// Sclera (eyeball front)
		glColor3f(0.97f, 0.97f, 0.96f);
		drawEllipsoid(sclRX, sclRY, sclRZ, 26);

		// Iris ring + pupil (slightly in front to avoid Z-fighting)
		const float zF = sclRZ * 0.98f;
		glColor3f(0.18f, 0.40f, 0.55f);                // iris base (blue-green)
		drawRingZ(0.085f, 0.145f, zF, 40);
		glColor3f(0.02f, 0.02f, 0.02f);                // pupil
		drawDiskZ(0.070f, zF + 0.002f, 36);
		glColor3f(1, 1, 1);                            // spec highlight
		glPushMatrix(); glTranslatef(-0.04f, 0.04f, 0.0f);
		drawDiskZ(0.028f, zF + 0.0035f, 20);
		glPopMatrix();

		// Eyelids = very thin ellipsoids (upper and lower)
		glColor3f(0.90f, 0.70f, 0.65f);                // skin
		// upper lid
		glPushMatrix();
		glTranslatef(0.0f, +sclRY * 0.35f, zF * 0.80f);
		glScalef(1.05f, 0.30f, 0.60f);
		drawEllipsoid(sclRX * 0.95f, sclRY * 0.95f, sclRZ * 0.55f, 20);
		glPopMatrix();
		// lower lid
		glPushMatrix();
		glTranslatef(0.0f, -sclRY * 0.30f, zF * 0.72f);
		glScalef(1.03f, 0.28f, 0.55f);
		drawEllipsoid(sclRX * 0.92f, sclRY * 0.92f, sclRZ * 0.50f, 18);
		glPopMatrix();

		// Optional guide line for lash/crease (V)
		//if (showModelLines) {
			GLboolean lit = glIsEnabled(GL_LIGHTING);
			glDisable(GL_LIGHTING);
			glLineWidth(1.3f); glColor3f(0, 0, 0);
			glPushMatrix(); glTranslatef(0, +sclRY * 0.40f, zF * 0.70f);
			headEllipseXY(sclRX * 0.95f, sclRY * 0.45f, 0.0f, 42);
			glPopMatrix();
			if (lit) glEnable(GL_LIGHTING);
		//}

		glPopMatrix();
		};

	oneEye(-1.0f);
	oneEye(+1.0f);

	// ================= Nose – alar wings + nostrils =================
	{
		// Alar wings (soft pads left/right of tip)
		glColor3f(0.90f, 0.70f, 0.65f);
		glPushMatrix();
		glTranslatef(-SKULL_RX * 0.18f, -SKULL_RY * 0.38f, SKULL_RZ * 0.80f);
		drawEllipsoid(0.14f, 0.10f, 0.09f, 16);
		glPopMatrix();
		glPushMatrix();
		glTranslatef(+SKULL_RX * 0.18f, -SKULL_RY * 0.38f, SKULL_RZ * 0.80f);
		drawEllipsoid(0.14f, 0.10f, 0.09f, 16);
		glPopMatrix();

		// Nostrils (dark, slightly tucked under)
		glColor3f(0.08f, 0.06f, 0.06f);
		glPushMatrix();
		glTranslatef(-SKULL_RX * 0.12f, -SKULL_RY * 0.45f, SKULL_RZ * 0.78f);
		glRotatef(18.0f, 1, 0, 0);
		drawEllipsoid(0.055f, 0.030f, 0.025f, 14);
		glPopMatrix();

		glPushMatrix();
		glTranslatef(+SKULL_RX * 0.12f, -SKULL_RY * 0.45f, SKULL_RZ * 0.78f);
		glRotatef(18.0f, 1, 0, 0);
		drawEllipsoid(0.055f, 0.030f, 0.025f, 14);
		glPopMatrix();

		// Optional bridge crease (V)
		if (showModelLines) {
			GLboolean lit = glIsEnabled(GL_LIGHTING);
			glDisable(GL_LIGHTING);
			glLineWidth(1.2f); glColor3f(0, 0, 0);
			glBegin(GL_LINE_STRIP);
			for (int i = 0;i <= 10;++i) {
				float t = i / 10.0f;
				float y = -SKULL_RY * (0.18f + 0.14f * t);
				float z = SKULL_RZ * (0.70f + 0.12f * t);
				glVertex3f(0.0f, y, z);
			}
			glEnd();
			if (lit) glEnable(GL_LIGHTING);
		}
	}
	// ================= Cheeks (plumper + soft tint) =================
	{
		glColor3f(0.92f, 0.72f, 0.67f);   // base
		// left
		glPushMatrix();
		glTranslatef(-SKULL_RX * 0.58f, -SKULL_RY * 0.18f, SKULL_RZ * 0.35f);
		drawEllipsoid(0.35f, 0.27f, 0.22f, 20);
		glPopMatrix();
		// right
		glPushMatrix();
		glTranslatef(+SKULL_RX * 0.58f, -SKULL_RY * 0.18f, SKULL_RZ * 0.35f);
		drawEllipsoid(0.35f, 0.27f, 0.22f, 20);
		glPopMatrix();

		// subtle blush
		glColor3f(0.96f, 0.76f, 0.74f);
		glPushMatrix();
		glTranslatef(-SKULL_RX * 0.58f, -SKULL_RY * 0.20f, SKULL_RZ * 0.39f);
		glScalef(1.2f, 0.8f, 0.8f);
		drawEllipsoid(0.18f, 0.11f, 0.04f, 14);
		glPopMatrix();
		glPushMatrix();
		glTranslatef(+SKULL_RX * 0.58f, -SKULL_RY * 0.20f, SKULL_RZ * 0.39f);
		glScalef(1.2f, 0.8f, 0.8f);
		drawEllipsoid(0.18f, 0.11f, 0.04f, 14);
		glPopMatrix();
	}
	// ================= Mouth + Lips =================
	{
		const float mY = -SKULL_RY * 0.50f;
		const float mZ = SKULL_RZ * 0.82f;

		// Upper lip
		glColor3f(0.79f, 0.54f, 0.54f);
		glPushMatrix();
		glTranslatef(0.0f, mY + 0.03f, mZ);
		glScalef(1.30f, 0.55f, 0.60f);
		drawEllipsoid(0.28f, 0.11f, 0.06f, 18);
		glPopMatrix();

		// Lower lip
		glColor3f(0.82f, 0.57f, 0.57f);
		glPushMatrix();
		glTranslatef(0.0f, mY - 0.02f, mZ - 0.01f);
		glScalef(1.20f, 0.50f, 0.55f);
		drawEllipsoid(0.26f, 0.10f, 0.06f, 18);
		glPopMatrix();

		// Mouth slit (dark ellipse, very thin)
		glColor3f(0.05f, 0.04f, 0.04f);
		glPushMatrix();
		glTranslatef(0.0f, mY, mZ + 0.002f);
		drawEllipseOutlineXY(0.24f, 0.055f, 0.0f, 42);   // outline
		drawDiskZ(0.22f, 0.0f, 36);                      // slight fill
		glPopMatrix();

		// Philtrum hint (V)
		if (showModelLines) {
			GLboolean lit = glIsEnabled(GL_LIGHTING);
			glDisable(GL_LIGHTING);
			glLineWidth(1.2f); glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex3f(0.0f, mY + 0.10f, mZ - 0.02f);
			glVertex3f(0.0f, mY + 0.03f, mZ - 0.00f);
			glEnd();
			if (lit) glEnable(GL_LIGHTING);
		}
	}


	// ================= Facial volumes =================
	// Cheek pads
	glPushMatrix();
	glTranslatef(-SKULL_RX*0.60f, -SKULL_RY*0.05f,  SKULL_RZ*0.25f);
	drawEllipsoid(0.32f, 0.26f, 0.20f, 18);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(+SKULL_RX*0.60f, -SKULL_RY*0.05f,  SKULL_RZ*0.25f);
	drawEllipsoid(0.32f, 0.26f, 0.20f, 18);
	glPopMatrix();

	// Nose bridge + tip
	glPushMatrix();
	glTranslatef(0.0f, -SKULL_RY*0.22f,  SKULL_RZ*0.82f);
	glScalef(0.60f, 0.50f, 0.55f);
	drawEllipsoid(0.22f, 0.30f, 0.22f, 18);
	glPopMatrix();

	// Brow ridge
	glPushMatrix();
	glTranslatef(0.0f, -SKULL_RY*0.28f,  SKULL_RZ*0.70f);
	glScalef(1.3f, 0.35f, 0.35f);
	drawEllipsoid(0.30f, 0.15f, 0.15f, 16);
	glPopMatrix();

	// Mouth/lips pad
	glPushMatrix();
	glTranslatef(0.0f, -SKULL_RY*0.47f,  SKULL_RZ*0.78f);
	glScalef(1.3f, 0.55f, 0.55f);
	drawEllipsoid(0.25f, 0.10f, 0.08f, 16);
	glPopMatrix();

	// Chin
	glPushMatrix();
	glTranslatef(0.0f, -SKULL_RY*0.62f,  SKULL_RZ*0.55f);
	drawEllipsoid(0.22f, 0.16f, 0.18f, 16);
	glPopMatrix();

	// ================= Ears =================
	// roughly middle of head height, slightly behind face plane
	const float earY = -SKULL_RY*0.10f;
	const float earZ = -SKULL_RZ*0.05f;

	glPushMatrix();                            // Left
	glTranslatef(-SKULL_RX*0.98f, earY, earZ);
	drawEar(-1, 0.22f, 0.30f, 0.16f);
	glPopMatrix();

	glPushMatrix();                            // Right
	glTranslatef(+SKULL_RX*0.98f, earY, earZ);
	drawEar(+1, 0.22f, 0.30f, 0.16f);
	glPopMatrix();

	// ================= Hair cap (adjusted, thicker & larger) =================
	glPushMatrix();
	glColor3f(0.10f, 0.08f, 0.07f);   // hair color (dark brown/black)
	glTranslatef(0.0f, +SKULL_RY * 0.18f, 0.0f);   // lift a little higher
	glScalef(1.05f, 1.03f, 1.05f);                 // slightly larger
	drawEllipsoid(SKULL_RX * 1.00f, SKULL_RY * 0.86f, SKULL_RZ * 1.00f, 28);
	// restore skin color
	if (glIsEnabled(GL_LIGHTING)) glColor3f(1, 1, 1);
	else glColor3f(SKIN.base[0], SKIN.base[1], SKIN.base[2]);
	glPopMatrix();

	glPopMatrix(); // root at torso neck
}


void poseidon() {
	glClearColor(0.75, 0.75, 0.75, 0.0);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);

	projection();

	// in poseidon(), before drawing your model (after clears/projection)
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);   // make sure it's filled
	glDisable(GL_LINE_SMOOTH);                   // just in case
	glDisable(GL_POLYGON_SMOOTH);


	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	lighting();
	glPopMatrix();

	GLuint textureArr[6];

	glPushMatrix();
	glTranslatef(xPosition, yPosition, zPosition);
	glRotatef(rotateY, 0, 1, 0);
	glRotatef(rotateX, 1, 0, 0);


	glPushMatrix();
	glRotatef(rp, 0, 1, 0.0);
	glTranslatef(txwhole, tywhole, tzwhole);

	switch (change) {
	case 1:
		textureArr[0] = loadTexture("a.bmp");
		break;
	case 2:
		textureArr[1] = loadTexture("b.bmp");
		break;
	case 3:
		textureArr[2] = loadTexture("c.bmp");
		break;
	case 4:
		textureArr[3] = loadTexture("d.bmp");
		break;
	}
	head();
	body();
	leftarm();
	rightarm();

	glPushMatrix();
	glTranslatef(0, -4, 0);
	leftleg();
	rightleg();
	glPopMatrix();
	// draw pelvis-anchored belt/back once, using last hip ring values
// NEW: continuous skirt that hugs the body at the top (no gap)
	drawSarongHuggingTorso(gPelvisY);

	
	


	glPopMatrix();

	glPopMatrix();

	glDeleteTextures(1, &textureArr[0]);
	glDeleteTextures(1, &textureArr[1]);
	glDeleteTextures(1, &textureArr[2]);
	glDeleteTextures(1, &textureArr[3]);
	glDeleteTextures(1, &textureArr[4]);
	glDisable(GL_TEXTURE_2D);


	action();





	
}
void display()
{
	//--------------------------------
	//	OpenGL drawing
	//--------------------------------
	poseidon();
	//--------------------------------
	//	End of OpenGL drawing
	//--------------------------------
}
//--------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = WindowProcedure;
	wc.lpszClassName = WINDOW_TITLE;
	wc.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&wc)) return false;

	HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL, NULL, wc.hInstance, NULL);

	//--------------------------------
	//	Initialize window for OpenGL
	//--------------------------------

	HDC hdc = GetDC(hWnd);

	//	initialize pixel format for the window
	initPixelFormat(hdc);

	//	get an openGL context
	HGLRC hglrc = wglCreateContext(hdc);

	//	make context current
	if (!wglMakeCurrent(hdc, hglrc)) return false;
	initQuadric();

	//--------------------------------
	//	End initialization
	//--------------------------------

	ShowWindow(hWnd, nCmdShow);

	MSG msg;
	ZeroMemory(&msg, sizeof(msg));

	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		display();

		SwapBuffers(hdc);
	}
	freeCapsuleLists();
	destroyQuadric();
	UnregisterClass(WINDOW_TITLE, wc.hInstance);

	return true;
}
//--------------------------------------------------------------------