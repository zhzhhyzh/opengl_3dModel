
#include <Windows.h>
#include <windowsx.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <map>


#define M_PI 3.14159265358979323846
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
// movement key state (hold-to-move)
bool gKeyW = false, gKeyA = false, gKeyS = false, gKeyD = false;


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

// ==== Walk system (facing + locomotion + gait) ====
bool  gWalking = false;
float gHeadingDeg = 0.0f;      // 0=front(+Z), 90=left(-X), -90=right(+X), 180=back(-Z)
float gWalkSpeed = 1.6f;      // world units / sec
float gWalkPhase = 0.0f;      // cycles the gait
float gStepHz = 2.2f;      // steps per second (2.2 → brisk walk)
float gArmAmpDeg = 26.0f;     // shoulder swing amplitude
float gLegAmpDeg = 22.0f;     // hip swing amplitude

static DWORD gLastTick = 0;      // for dt in action()

// --- Off-hand (left) grip target, world space ---
static bool  gOffhandActive = false;
static float gOffhandTargetW[3] = { 0,0,0 };

// column-major OpenGL helpers
static inline void mulPointM4(const float M[16], const float p[3], float out[3]) {
	out[0] = M[0] * p[0] + M[4] * p[1] + M[8] * p[2] + M[12];
	out[1] = M[1] * p[0] + M[5] * p[1] + M[9] * p[2] + M[13];
	out[2] = M[2] * p[0] + M[6] * p[1] + M[10] * p[2] + M[14];
}
// inverse for rigid (R|t) matrices (no non‑uniform scale)
static inline void invRigidM4(const float M[16], float Inv[16]) {
	// R^T
	Inv[0] = M[0]; Inv[1] = M[4]; Inv[2] = M[8];
	Inv[4] = M[1]; Inv[5] = M[5]; Inv[6] = M[9];
	Inv[8] = M[2]; Inv[9] = M[6]; Inv[10] = M[10];
	Inv[3] = Inv[7] = Inv[11] = 0.0f; Inv[15] = 1.0f;
	// -R^T * t
	float tx = M[12], ty = M[13], tz = M[14];
	Inv[12] = -(Inv[0] * tx + Inv[4] * ty + Inv[8] * tz);
	Inv[13] = -(Inv[1] * tx + Inv[5] * ty + Inv[9] * tz);
	Inv[14] = -(Inv[2] * tx + Inv[6] * ty + Inv[10] * tz);
}
static inline float rad2deg(float r) { return r * 57.2957795f; }


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
// ===== Action pose offsets (added on top of walk pose) =====
static float actTorsoPitch = 0, actTorsoYaw = 0, actTorsoRoll = 0;

static float actRShoulderPitch = 0, actRShoulderYaw = 0, actRShoulderRoll = 0, actRElbowFlex = 0;
static float actLShoulderPitch = 0, actLShoulderYaw = 0, actLShoulderRoll = 0, actLElbowFlex = 0;

static float actHipPitchR = 0, actHipPitchL = 0;    // at the hips
static float actKneeFlexR = 0, actKneeFlexL = 0;    // at the knees
static inline float clamp01(float t) { return t < 0 ? 0 : t>1 ? 1 : t; }
static inline float lerp(float a, float b, float t) { t = clamp01(t); return a + (b - a) * t; }
static inline float easeInOut(float t) { t = clamp01(t); return t * t * (3.0f - 2.0f * t); } // smoothstep

// ================== WEAPON STATE (Trident) ==================
enum WeaponState { WPN_ON_BACK, WPN_IN_HAND, WPN_EQUIP_ANIM, WPN_Z_COMBO, WPN_X_CHARGING, WPN_X_SWEEP, WPN_C_WATERSKIM };
static WeaponState gWpnState = WPN_ON_BACK;
//static inline float lerp(float a, float b, float t) { if (t < 0)t = 0; if (t > 1)t = 1; return a + (b - a) * t; }

static float gWpnTimer = 0.0f;       // general timer for staged moves
static float gWpnCharge = 0.0f;      // 0..1 charge build for X
static bool  gWpnKeyXDown = false;   // hold detection

// very lightweight water trail (C): a ribbon of recent forward positions
struct TrailPt { float x, y, z, life; };
static std::vector<TrailPt> gWaterTrail;

// forward decls
static void drawTridentGeo();
static void drawTridentHeld();     // draw at right‑hand local frame (hooked inside right arm)
static void drawTridentOnBack();   // draw attached to back
static void updateWeapon(float dt);

// ---- Z‑combo jump control ----
static const float GROUND_Y = 0.0f;  // your root default (keep if you use a different ground)
static float zJumpStartY = 0.0f;     // cached when Z starts
static bool  zImpactSpawned = false; // to spawn FX once
static float gImpactTimer = 0.0f;    // ground shock FX


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

	case WM_KEYUP:
	{
		if (wParam == 'W') gKeyW = false;
		else if (wParam == 'A') gKeyA = false;
		else if (wParam == 'S') gKeyS = false;
		else if (wParam == 'D') gKeyD = false;
		else if (wParam == 'X') { if (gWpnKeyXDown && gWpnState == WPN_X_CHARGING) { gWpnState = WPN_X_SWEEP; gWpnTimer = 0; } gWpnKeyXDown = false; }

		// if no movement keys are down, stop walking
		if (!(gKeyW || gKeyA || gKeyS || gKeyD)) {
			gWalking = false;
			// (optional) freeze the gait at a comfortable pose:
			// gWalkPhase = 0.0f;
		}
		break;
	}


	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) PostQuitMessage(0);
		else if (wParam == VK_SPACE) {
			gHeadingDeg = 0;
			rotateY = 0;
			gWalking = false;
			gKeyW = gKeyA = gKeyS = gKeyD = false;   // clear keys
			xPosition = 0.0f; yPosition = 0.0f; zPosition = 0.05f;
			gWalkPhase = 0.0f;
			gWpnState = WPN_ON_BACK; gWpnTimer = 0.0f; gWpnCharge = 0.0f; gWaterTrail.clear(); gWpnKeyXDown = false;
			actTorsoPitch = actTorsoYaw = actTorsoRoll = 0;
			actRShoulderPitch = actRShoulderYaw = actRShoulderRoll = actRElbowFlex = 0;
			actLShoulderPitch = actLShoulderYaw = actLShoulderRoll = actLElbowFlex = 0;
			actHipPitchR = actHipPitchL = 0; actKneeFlexR = actKneeFlexL = 0;


		}
		else if (wParam == 'W') {
			gKeyW = true;
			gHeadingDeg = 0.0f;   rotateY = 0.0f;   gWalking = true;
		}
		else if (wParam == 'A') {
			gKeyA = true;
			gHeadingDeg = 90.0f;  rotateY = -90.0f; gWalking = true;
		}
		else if (wParam == 'D') {
			gKeyD = true;
			gHeadingDeg = -90.0f; rotateY = 90.0f;  gWalking = true;
		}
		else if (wParam == 'S') {
			gKeyS = true;
			gHeadingDeg = 180.0f; rotateY = 180.0f; gWalking = true;
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
	
		else if (wParam == 'V') showModelLines = !showModelLines;
		else if (wParam == 'B') { if (gWpnState == WPN_ON_BACK) { gWpnState = WPN_EQUIP_ANIM; gWpnTimer = 0; } else { gWpnState = WPN_ON_BACK; gWaterTrail.clear(); } }
		else if (wParam == 'Z') {
			if (gWpnState == WPN_IN_HAND) {
				gWpnState = WPN_Z_COMBO;
				gWpnTimer = 0.0f;
				zJumpStartY = yPosition;   // <-- record current Y for the jump arc
				zImpactSpawned = false;    // <-- we haven’t hit the ground yet
				gImpactTimer = 0.0f;       // <-- reset shock ring FX
			}
		}

		else if (wParam == 'X') { if (gWpnState == WPN_IN_HAND) { gWpnState = WPN_X_CHARGING; gWpnTimer = 0; gWpnCharge = 0; gWpnKeyXDown = true; } }
		else if (wParam == 'C') { if (gWpnState == WPN_IN_HAND) { gWpnState = WPN_C_WATERSKIM; gWpnTimer = 0; gWaterTrail.clear(); } }

		
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
// Radial water‑splash shockwave (XZ ring + upward splash spikes)
// `len` acts as the outer radius of the expanding ring.
static void drawShockwave(float len) {
	if (len < 0.0f) len = 0.0f;

	const int   SEG = 72;            // ring smoothness
	const float R1 = len;           // outer radius
	const float R0 = (len > 0.22f) ? (len - 0.22f) : 0.0f;  // inner radius
	const float Y = 0.02f;         // near‑ground height
	const float A0 = 0.45f * (1.0f - fminf(len * 0.4f, 0.85f)); // fade as it grows

	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// --- 1) Expanding circular ring on the ground (triangle strip) ---
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i <= SEG; ++i) {
		float t = (float)i / (float)SEG;
		float th = t * 6.28318530718f; // 2π
		float c = cosf(th), s = sinf(th);

		glColor4f(0.70f, 0.88f, 1.0f, A0);        // inner edge (brighter)
		glVertex3f(R0 * c, Y, R0 * s);

		glColor4f(0.70f, 0.88f, 1.0f, A0 * 0.15f); // outer edge (fade out)
		glVertex3f(R1 * c, Y, R1 * s);
	}
	glEnd();

	// --- 2) Short upward splash “crowns” around the ring (tri wedges) ---
	// uses a little noise with sin/cos so it wiggles as `len` changes
	glBegin(GL_TRIANGLES);
	const int SPIKES = 24;
	for (int i = 0; i < SPIKES; ++i) {
		float t = (float)i / (float)SPIKES;
		float th = t * 6.28318530718f;
		float c = cosf(th), s = sinf(th);

		float r = 0.5f * (R0 + R1);               // base radius for spike
		float h = 0.35f + 0.55f * (0.5f + 0.5f * sinf(7.0f * th + len * 6.0f));
		float w = 0.08f + 0.04f * cosf(5.0f * th); // small varying width
		float aSp = A0 * 0.75f;

		glColor4f(0.75f, 0.92f, 1.0f, aSp);
		glVertex3f((r - w) * c, Y, (r - w) * s);   // base left
		glVertex3f((r + w) * c, Y, (r + w) * s);   // base right
		glColor4f(0.75f, 0.92f, 1.0f, aSp * 0.85f);
		glVertex3f(r * c, h, r * s);               // tip up
	}
	glEnd();

	glPopAttrib();
}


static void renderWaterTrail() {
	if (gWaterTrail.empty()) return;
	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_LIGHTING); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_QUAD_STRIP);
	for (auto& p : gWaterTrail) {
		float a = (p.life < 0 ? 0 : p.life); if (a > 1)a = 1;
		glColor4f(0.35f, 0.65f, 1.0f, 0.30f * a);
		glVertex3f(p.x - 0.22f, p.y, p.z);
		glVertex3f(p.x + 0.22f, p.y, p.z);
	}
	glEnd();
	glPopAttrib();
}






// call this from updateWeapon(dt) after you advance gWpnTimer etc.
static void computeActionPose()
{
	// reset to neutral each frame
	actTorsoPitch = actTorsoYaw = actTorsoRoll = 0;
	actRShoulderPitch = actRShoulderYaw = actRShoulderRoll = actRElbowFlex = 0;
	actLShoulderPitch = actLShoulderYaw = actLShoulderRoll = actLElbowFlex = 0;
	actHipPitchR = actHipPitchL = 0;
	actKneeFlexR = actKneeFlexL = 0;

	// ===== Z : Raise -> Up thrust -> Down chop (0.75s)
	if (gWpnState == WPN_Z_COMBO) {
		float t = gWpnTimer;

		const float tPrep = 0.18f, tRise = 0.28f, tHang = 0.98f, tFall = 0.30f;
		float wPrep = clamp01(t / tPrep);
		float wRise = clamp01((t - tPrep) / tRise);
		float wHang = clamp01((t - tPrep - tRise) / tHang);
		float wFall = clamp01((t - tPrep - tRise - tHang) / tFall);

		// --- Crouch anticipation ---
		if (t < tPrep) {
			actTorsoPitch += lerp(0, +10, wPrep);
			actHipPitchL += lerp(0, +6, wPrep);
			actHipPitchR += lerp(0, +6, wPrep);
			actKneeFlexL += lerp(0, +14, wPrep);
			actKneeFlexR += lerp(0, +14, wPrep);

			actRShoulderPitch += lerp(0, +18, wPrep);   // wind‑up
			actRElbowFlex += lerp(0, +10, wPrep);
			actLShoulderPitch += -lerp(0, 18, wPrep);
			return;
		}

		// --- Ascend: spear rises to ear height ---
		if (t < tPrep + tRise) {
			float u = (t - tPrep) / tRise;
			// stronger raise
			actRShoulderPitch += lerp(+18, +620, u);   // was ~40
			actRElbowFlex += lerp(+10, +16, u);

			//// Left arm mirrors to keep the two‑hand grip
			//actLShoulderPitch += lerp(-12, +540, u);
			//actLElbowFlex += lerp(+8, +14, u);
			return;
		}
		// --- Hang (by the ear) ---
		if (t < tPrep + tRise + tHang) {
			actRShoulderPitch -= 62.0f;
			actRElbowFlex -= 16.0f;
			actLShoulderPitch -= 27.0f;
			actLElbowFlex -= 7.0f;
			return;
		}
		// --- Fall: big chop ---
		{
			float u = (t - (tPrep + tRise + tHang)) / tFall;
			float s = easeInOut(u);
			actRShoulderPitch -= lerp(+20, +110, s);   // deeper chop
			actRElbowFlex += lerp(8, +28, s);
			actLShoulderPitch += lerp(+12, +95, s);   // off‑hand follows through
			actLElbowFlex += lerp(6, +26, s);
			// torso & legs unchanged (your shock ring still fires near landing)
			actTorsoPitch += lerp(4, +20, s);  // lean into the slam
			actHipPitchL += lerp(0, +8, s);
			actHipPitchR += lerp(0, +6, s);
			actKneeFlexL += lerp(12, +22, s);  // landing bend
			actKneeFlexR += lerp(12, +18, s);
			return;
		}


		
	}

	// ===== X : Charge (hold) then Sweep (release)
	if (gWpnState == WPN_X_CHARGING) {
		float t = easeInOut(fmodf(gWpnTimer, 0.35f) / 0.35f);        // small loop while charging
		float trem = (sinf(gWpnTimer * 20.0f) * 0.5f + 0.5f);            // tremble 0..1

		actRShoulderPitch -= -22.0f;      // draw back
		actRShoulderYaw += 30.0f;
		actRElbowFlex -= 22.0f + 4.0f * t;

		actLShoulderPitch -= 14.0f;      // left hand forward a bit
		actLElbowFlex -= 10.0f;

		//actTorsoPitch += 8.0f;       // crouch
		//actTorsoYaw += 6.0f * t;
		//actKneeFlexL += 14.0f + 4.0f * t;
		//actKneeFlexR += 14.0f + 4.0f * (1.0f - t);

		// tiny shake
		actRShoulderRoll += 3.0f * sinf(gWpnTimer * 18.0f);
		return;
	}
	if (gWpnState == WPN_X_SWEEP) {
		// 0..0.28s sweep left->right with torso twist
		float t = clamp01(gWpnTimer / 0.28f);
		float s = easeInOut(t);
		//actTorsoYaw += lerp(-22, +22, s);
		actTorsoPitch += +6.0f * (1.0f - s);
		actRShoulderYaw -= lerp(-20, -400, s);
		actRShoulderPitch += lerp(-20, -50, s);
		actRElbowFlex += lerp(10, -4, s);
		actLShoulderPitch += lerp(-8, +10, s);
		// plant legs + recoil at end
		/*actKneeFlexL += lerp(12, 16, s);
		actKneeFlexR += lerp(12, 10, s);*/
		return;
	}

	// ===== C : Water Skimming — back & forth slashes while stepping
	if (gWpnState == WPN_C_WATERSKIM) {
		float t = gWpnTimer;              // seconds
		float swing = sinf(t * 9.0f);       // fast lateral
		actTorsoYaw += 14.0f * swing;
		actRShoulderYaw += 26.0f * swing;
		actRShoulderPitch += 10.0f;
		actRElbowFlex += 16.0f;
		actLShoulderPitch += -12.0f * swing;
		// light stepping stance
		actHipPitchL += 8.0f;
		actHipPitchR += 6.0f;
		actKneeFlexL += 10.0f + 4.0f * (swing > 0 ? swing : 0);
		actKneeFlexR += 10.0f + 4.0f * (swing < 0 ? -swing : 0);
		return;
	}
}

static void updateWeapon(float dt) {
	// Equip pop
	if (gWpnState == WPN_EQUIP_ANIM) { gWpnTimer += dt; if (gWpnTimer > 0.20f) { gWpnState = WPN_IN_HAND; gWpnTimer = 0; } return; }
	// ===== Z : Jump → Up‑thrust → Down‑slam =====
	if (gWpnState == WPN_Z_COMBO) {
		gWpnTimer += dt;

		// timeline
		const float tPrep = 0.18f;  // crouch
		const float tRise = 0.28f;  // jump up
		const float tHang = 0.18f;  // short hang
		const float tFall = 0.30f;  // fast drop + slam
		const float T = tPrep + tRise + tHang + tFall;

		float t = gWpnTimer;
		float Y = zJumpStartY;

		if (t < tPrep) {
			// crouch (dip a bit)
			Y = zJumpStartY - lerp(0.0f, 0.22f, t / tPrep);
		}
		else if (t < tPrep + tRise) {
			// rise (accelerating up)
			float u = (t - tPrep) / tRise;    // 0..1
			Y = zJumpStartY - 0.22f + lerp(0.0f, 1.85f, u * u);
		}
		else if (t < tPrep + tRise + tHang) {
			// brief apex
			Y = zJumpStartY + 1.63f;
		}
		else if (t < T) {
			// fall quickly
			float u = (t - (tPrep + tRise + tHang)) / tFall;   // 0..1
			Y = zJumpStartY + 1.63f - lerp(0.0f, 1.75f, u * u);

			// near the end → spawn impact once
			if (u > 0.92f && !zImpactSpawned) {
				zImpactSpawned = true;
				gImpactTimer = 0.28f;    // show shock ring
			}
		}
		else {
			// land & finish
			yPosition = GROUND_Y;
			gWpnState = WPN_IN_HAND;
			gWpnTimer = 0.0f;
			zImpactSpawned = false;
			computeActionPose();         // clears to idle pose
			return;
		}

		yPosition = Y;
		computeActionPose();             // produce limb/torso pose for this frame
		return;
	}

	// X
	if (gWpnState == WPN_X_CHARGING) { gWpnTimer += dt; gWpnCharge = min(1.0f, gWpnCharge + dt * 1.2f); return; }
	if (gWpnState == WPN_X_SWEEP) { gWpnTimer += dt; if (gWpnTimer > 0.58f) { gWpnState = WPN_IN_HAND; gWpnTimer = 0; gWpnCharge = 0; } return; }
	// C
	if (gWpnState == WPN_C_WATERSKIM) {
		gWpnTimer += dt;
		// trail point in front of character
		TrailPt tp; tp.x = xPosition; tp.y = yPosition + 0.6f; tp.z = zPosition + 0.9f; tp.life = 1.0f;
		gWaterTrail.push_back(tp);
		for (auto& p : gWaterTrail) p.life -= dt * 1.4f;
		while (!gWaterTrail.empty() && gWaterTrail.front().life <= 0) gWaterTrail.erase(gWaterTrail.begin());
		if (gWpnTimer > 0.80f) { gWpnState = WPN_IN_HAND; gWpnTimer = 0; }
		return;
	}
}

static void drawGroundImpactRing() {
	if (gImpactTimer <= 0.0f) return;
	float t = 1.0f - clamp01(gImpactTimer / 0.28f);   // 0..1
	float R0 = 0.2f + 0.6f * t;
	float R1 = R0 + 0.10f;

	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_LIGHTING); glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0.65f, 0.80f, 1.0f, 0.45f * (1.0f - t));

	// flat ring on ground (tri strip)
	glPushMatrix();
	glTranslatef(xPosition, GROUND_Y, zPosition + 5.6f);   // a tad in front of feet
	glRotatef(rotateY + actTorsoYaw, 0, 1, 0);

	const int seg = 48;
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0;i <= seg;++i) {
		float a = (float)i / seg * 6.2831853f;
		float c = cosf(a), s = sinf(a);
		glVertex3f(R0 * c, 0.01f, R0 * s);
		glVertex3f(R1 * c, 0.01f, R1 * s);
	}
	glEnd();
	glPopMatrix();
	glPopAttrib();
}

//--------------------------------------------------------------------
void action() {
	// --- dt in seconds ---
	DWORD now = GetTickCount();
	if (gLastTick == 0) gLastTick = now;
	float dt = (now - gLastTick) * (1.0f / 1000.0f);
	gLastTick = now;

	// --- advance gait & translate root when walking ---
	if (gWalking) {
		gWalkPhase += 2.0f * 3.14159265f * gStepHz * dt;  // radians

			float yaw = gHeadingDeg * 3.14159265f / 180.0f;
			float vx = -sinf(yaw);
			float vz = cosf(yaw);
			
			xPosition += vx * gWalkSpeed * dt;
			zPosition += vz * gWalkSpeed * dt;
	}

	updateWeapon(dt);
	if (gImpactTimer > 0.0f) {
		gImpactTimer -= dt;
		if (gImpactTimer < 0.0f) gImpactTimer = 0.0f;
	}

	computeActionPose();   // produces the act* pose numbers every frame


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
	const SkinPreset SKIN = SKIN_LIGHT_TAN; // or SKIN_FAIR_ROSY / SKIN_LIGHT_TAN

	applySkinMaterial(SKIN.base);
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
	glColor3f(SKIN_LIGHT_TAN.base[0],
		SKIN_LIGHT_TAN.base[1],
		SKIN_LIGHT_TAN.base[2]);

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
		glColor3f(SKIN_FAIR_NEUTRAL.base[0],
			SKIN_FAIR_NEUTRAL.base[1],
			SKIN_FAIR_NEUTRAL.base[2]);
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
		glColor3f(SKIN_FAIR_NEUTRAL.base[0],
			SKIN_FAIR_NEUTRAL.base[1],
			SKIN_FAIR_NEUTRAL.base[2]);
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
		glColor3f(SKIN_LIGHT_TAN.base[0],
			SKIN_LIGHT_TAN.base[1],
			SKIN_LIGHT_TAN.base[2]);
		drawEllipsoid(0.55f, 0.65f, 0.45f, 28);
		glPopMatrix();

		// Right deltoid
		glPushMatrix();
		glTranslatef(+a_at * 0.95f, y, b_at * 0.25f);
		glRotatef(-20.0f, 0, 1, 0);
		glColor3f(SKIN_LIGHT_TAN.base[0],
			SKIN_LIGHT_TAN.base[1],
			SKIN_LIGHT_TAN.base[2]);
		drawEllipsoid(0.55f, 0.65f, 0.45f, 28);
		glPopMatrix();

		

		// ===================== RIGHT SHOULDER EPaulette (armor + fringe) =====================
		{
			glPushMatrix();
			glTranslatef(-3.95, 0.65, 0.65);
			glRotatef(-5, 1, 0, 0);
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
			//glDisable(GL_TEXTURE_2D);

			// --- placement: a little above/right/forward of the right deltoid
			const float sx = +a_at * 0.98f;
			const float sy = y + 0.12f;         // slightly above shoulder
			const float sz = b_at * 0.10f;      // a tad forward

			// --- overall proportions
			const float rx_outer = 0.62f;       // ellipse radius across X (outer rim)
			const float rz_outer = 0.42f;       // ellipse radius along Z (outer rim)
			const float rim_thick = 0.06f;      // rim thickness outward
			const float board_t = 0.05f;      // board thickness (vertical)
			const float dome_h = 0.06f;      // slight crown height
			const int   N = 24;         // segmentation

			// small helper: make a thin box with quads
			auto box = [](float x0, float x1, float y0, float y1, float z0, float z1) {
				glBegin(GL_QUADS);
				// bottom
				glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
				// top
				glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y0, z1);
				// sides
				glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0);
				glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0);
				glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
				glVertex3f(x0, y1, z0); glVertex3f(x0, y1, z1); glVertex3f(x0, y0, z1); glVertex3f(x0, y0, z0);
				glEnd();
				};

			glPushMatrix();
			glTranslatef(sx, sy, sz);
			glRotatef(-12.f, 0, 1, 0);    // open a bit to the front
			glRotatef(+8.f, 1, 0, 0);    // slight down tilt

			// colors (gold-ish)
			const GLfloat GOLD[3] = { 0.85f, 0.72f, 0.25f };
			const GLfloat GOLD_D[3] = { 0.60f, 0.50f, 0.18f };

			// --- (A) top board: a shallow dome made of triangles, sitting on a flat board
			// flat board slab
			glColor3fv(GOLD_D);
			{
				// make the slab by “ring” walls (outer wall + inner wall) and top/bottom caps with triangles
				const float rx_in = rx_outer * 0.90f;
				const float rz_in = rz_outer * 0.90f;
				const float y0 = 0.0f, y1 = board_t;

				// outer vertical wall
				for (int i = 0;i < N;i++) {
					float a0 = (2.f * (float)M_PI / N) * i;
					float a1 = (2.f * (float)M_PI / N) * (i + 1);
					float x0 = rx_outer * cosf(a0), z0 = rz_outer * sinf(a0);
					float x1 = rx_outer * cosf(a1), z1 = rz_outer * sinf(a1);
					glBegin(GL_QUADS);
					glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z0);
					glEnd();
				}
				// inner vertical wall
				for (int i = 0;i < N;i++) {
					float a0 = (2.f * (float)M_PI / N) * i;
					float a1 = (2.f * (float)M_PI / N) * (i + 1);
					float x0 = rx_in * cosf(a0), z0 = rz_in * sinf(a0);
					float x1 = rx_in * cosf(a1), z1 = rz_in * sinf(a1);
					glBegin(GL_QUADS);
					glVertex3f(x1, y0, z1); glVertex3f(x0, y0, z0); glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z1);
					glEnd();
				}
				// bottom cap (triangles)
				glColor3fv(GOLD_D);
				for (int i = 0;i < N;i++) {
					float a0 = (2.f * (float)M_PI / N) * i, a1 = (2.f * (float)M_PI / N) * (i + 1);
					float xo0 = rx_outer * cosf(a0), zo0 = rz_outer * sinf(a0);
					float xo1 = rx_outer * cosf(a1), zo1 = rz_outer * sinf(a1);
					float xi0 = rx_in * cosf(a0), zi0 = rz_in * sinf(a0);
					float xi1 = rx_in * cosf(a1), zi1 = rz_in * sinf(a1);
					glBegin(GL_TRIANGLES);
					glVertex3f(xo0, y0, zo0); glVertex3f(xo1, y0, zo1); glVertex3f(xi1, y0, zi1);
					glVertex3f(xo0, y0, zo0); glVertex3f(xi1, y0, zi1); glVertex3f(xi0, y0, zi0);
					glEnd();
				}
				// top dome (triangles) – rises to y1 + dome_h
				glColor3fv(GOLD);
				const float yc = y1 + dome_h;
				for (int i = 0;i < N;i++) {
					float a0 = (2.f * (float)M_PI / N) * i, a1 = (2.f * (float)M_PI / N) * (i + 1);
					float xi0 = rx_in * cosf(a0), zi0 = rz_in * sinf(a0);
					float xi1 = rx_in * cosf(a1), zi1 = rz_in * sinf(a1);
					glBegin(GL_TRIANGLES);
					glVertex3f(0.f, yc, 0.f);
					glVertex3f(xi1, y1, zi1);
					glVertex3f(xi0, y1, zi0);
					glEnd();
				}
			}

			// --- (B) bead/rim (raised outer lip)
			glColor3fv(GOLD);
			{
				const float rxo = rx_outer + rim_thick;
				const float rzo = rz_outer + rim_thick;
				const float yb0 = board_t * 0.50f, yb1 = yb0 + rim_thick * 0.90f;
				for (int i = 0;i < N;i++) {
					float a0 = (2.f * (float)M_PI / N) * i, a1 = (2.f * (float)M_PI / N) * (i + 1);
					float x0i = rx_outer * cosf(a0), z0i = rz_outer * sinf(a0);
					float x1i = rx_outer * cosf(a1), z1i = rz_outer * sinf(a1);
					float x0o = rxo * cosf(a0), z0o = rzo * sinf(a0);
					float x1o = rxo * cosf(a1), z1o = rzo * sinf(a1);
					glBegin(GL_QUADS); // outer wall
					glVertex3f(x0o, yb0, z0o); glVertex3f(x1o, yb0, z1o); glVertex3f(x1o, yb1, z1o); glVertex3f(x0o, yb1, z0o);
					glEnd();
					glBegin(GL_QUADS); // inner wall
					glVertex3f(x1i, yb0, z1i); glVertex3f(x0i, yb0, z0i); glVertex3f(x0i, yb1, z0i); glVertex3f(x1i, yb1, z1i);
					glEnd();
					glBegin(GL_QUADS); // top cap
					glVertex3f(x0i, yb1, z0i); glVertex3f(x1i, yb1, z1i); glVertex3f(x1o, yb1, z1o); glVertex3f(x0o, yb1, z0o);
					glEnd();
				}
			}

			// --- (C) fringe/tassels: narrow boxes hanging along the outer front/side arc
			glColor3fv(GOLD);
			const float tass_w = 0.08f;
			const float tass_t = 0.03f;
			const float tass_h = 0.72f;
			for (int i = 2;i <= N - 4;i++) {                 // skip back side so it doesn’t intersect collar
				float a = (2.f * (float)M_PI / N) * i;
				// only front/outer quadrant
				// (angles around ellipse: i from ~2..N-4 covers front/outer)
				float xo = (rx_outer + rim_thick) * cosf(a);
				float zo = (rz_outer + rim_thick) * sinf(a);
				// tangent direction to place local X of tassel
				float tx = -sinf(a), tz = cosf(a);
				// build a tiny oriented box with quads
				float xL = xo - (tass_w * 0.5f) * tx, zL = zo - (tass_w * 0.5f) * tz;
				float xR = xo + (tass_w * 0.5f) * tx, zR = zo + (tass_w * 0.5f) * tz;
				float yTop = board_t * 0.60f + rim_thick * 0.90f;
				float yBot = yTop - tass_h;

				glBegin(GL_QUADS);
				// front
				glVertex3f(xL, yBot, zL); glVertex3f(xR, yBot, zR); glVertex3f(xR, yTop, zR); glVertex3f(xL, yTop, zL);
				// back
				glVertex3f(xR, yBot, zR); glVertex3f(xL, yBot, zL); glVertex3f(xL, yTop, zL); glVertex3f(xR, yTop, zR);
				// left side
				glVertex3f(xL, yBot, zL - tass_t); glVertex3f(xL, yBot, zL); glVertex3f(xL, yTop, zL); glVertex3f(xL, yTop, zL - tass_t);
				// right side
				glVertex3f(xR, yBot, zR); glVertex3f(xR, yBot, zR - tass_t); glVertex3f(xR, yTop, zR - tass_t); glVertex3f(xR, yTop, zR);
				// bottom
				glVertex3f(xL, yBot, zL); glVertex3f(xR, yBot, zR); glVertex3f(xR, yBot, zR - tass_t); glVertex3f(xL, yBot, zL - tass_t);
				glEnd();
			}

			glPopMatrix();
			glPopAttrib();

			glPopMatrix();

		}
		// =================== END RIGHT SHOULDER EPaulette ===================

		
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
		float x = L.xsign * (a * 0.48f);     // reach ~68% of half-width → more lateral coverage
		float z = b * 0.95f + L.fwd;         // hug surface, slight outward bias

		glPushMatrix();
		glTranslatef(x, y, z);
		glRotatef(L.tiltY * L.xsign, 0, 1, 0); // mirror yaw per side
		// Flatten slightly in Z and spread in X to avoid “short ovals”
		glScalef(0.8f, 0.90f, 0.80f);
		setSkin(SKIN_FAIR_NEUTRAL.base[0],
			SKIN_FAIR_NEUTRAL.base[1],
			SKIN_FAIR_NEUTRAL.base[2]);
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
		float z = b * 1;
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
		float x = (t - 0.5f) * 2.0f * (a * 0.75f);
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
		float z = b * 1.15f;

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
// ===== Helpers (only GL_QUADS / GL_TRIANGLES) =====
static inline void nrm(float x, float y, float z) { glNormal3f(x, y, z); }

// Axis‑aligned box centered at origin
static void quadBox(float w, float h, float d) {
	const float x = w * 0.5f, y = h * 0.5f, z = d * 0.5f;
	glBegin(GL_QUADS);
	// +Z
	nrm(0, 0, 1);  glVertex3f(-x, -y, z); glVertex3f(x, -y, z); glVertex3f(x, y, z); glVertex3f(-x, y, z);
	// -Z
	nrm(0, 0, -1); glVertex3f(x, -y, -z); glVertex3f(-x, -y, -z); glVertex3f(-x, y, -z); glVertex3f(x, y, -z);
	// +X
	nrm(1, 0, 0);  glVertex3f(x, -y, z); glVertex3f(x, -y, -z); glVertex3f(x, y, -z); glVertex3f(x, y, z);
	// -X
	nrm(-1, 0, 0); glVertex3f(-x, -y, -z); glVertex3f(-x, -y, z); glVertex3f(-x, y, z); glVertex3f(-x, y, -z);
	// +Y
	nrm(0, 1, 0);  glVertex3f(-x, y, z); glVertex3f(x, y, z); glVertex3f(x, y, -z); glVertex3f(-x, y, -z);
	// -Y
	nrm(0, -1, 0); glVertex3f(-x, -y, -z); glVertex3f(x, -y, -z); glVertex3f(x, -y, z); glVertex3f(-x, -y, z);
	glEnd();
}

static inline void setGold() { glColor3f(0.95f, 0.82f, 0.30f); }
static inline void setSteel() { glColor3f(0.85f, 0.90f, 0.95f); }
static inline void setBlue() { glColor3f(0.20f, 0.30f, 0.85f); }
static inline void setRuby() { glColor3f(0.85f, 0.10f, 0.10f); }
// ================== Trident attached to back ==================

// ===== Trident colors =====
static inline void colGold() { glColor3f(0.92f, 0.78f, 0.25f); }
static inline void colSteel() { glColor3f(0.78f, 0.82f, 0.88f); }
static inline void colBlue() { glColor3f(0.18f, 0.28f, 0.65f); }
static inline void colLeather() { glColor3f(0.40f, 0.28f, 0.18f); }
static inline void colGem() { glColor3f(0.08f, 0.25f, 0.85f); }

// simple box (you already have quadBox) + thin wedge tip
static void triWedgeTip(float w, float h, float d) {
	// a little pyramid-ish point with triangles
	const float x = w * 0.5f, y = h * 0.5f, z = d * 0.5f;
	glBegin(GL_TRIANGLES);
	// front
	glNormal3f(0, 0, 1); glVertex3f(-x, y, z); glVertex3f(x, y, z); glVertex3f(0, -y, z);
	// back
	glNormal3f(0, 0, -1); glVertex3f(x, y, -z); glVertex3f(-x, y, -z); glVertex3f(0, -y, -z);
	// left
	glNormal3f(-1, 0, 0); glVertex3f(-x, y, -z); glVertex3f(-x, y, z); glVertex3f(0, -y, 0);
	// right
	glNormal3f(1, 0, 0); glVertex3f(x, y, z); glVertex3f(x, y, -z); glVertex3f(0, -y, 0);
	glEnd();
}

// curvy side blade built from a few quads (approx. the reference shape)
static void sideBladePatch(float sign, float L = 1.6f) {
	// sign: -1 = left, +1 = right
	glPushMatrix();
	glScalef(sign, 1, 1);
	// 3 curved sections going outward, forward, then down
	struct Seg { float yaw, roll, len, w0, w1, t; };
	Seg segs[] = {
		{  25,  0,  L * 0.36f, 0.22f, 0.18f, 0.16f },
		{ -18, 10,  L * 0.34f, 0.18f, 0.16f, 0.14f },
		{ -35,-10,  L * 0.42f, 0.16f, 0.06f, 0.10f }
	};
	for (auto& s : segs) {
		glRotatef(s.yaw, 0, 1, 0);
		glRotatef(s.roll, 0, 0, 1);
		colSteel();
		// segment body
		glBegin(GL_QUADS);
		float w0 = s.w0 * 0.5f, w1 = s.w1 * 0.5f, t = s.t * 0.5f;
		// +Z face
		glNormal3f(0, 0, 1);
		glVertex3f(-w0, 0, t); glVertex3f(w0, 0, t);
		glVertex3f(w1, -s.len, t); glVertex3f(-w1, -s.len, t);
		// -Z face
		glNormal3f(0, 0, -1);
		glVertex3f(w0, 0, -t); glVertex3f(-w0, 0, -t);
		glVertex3f(-w1, -s.len, -t); glVertex3f(w1, -s.len, -t);
		// +X
		glNormal3f(1, 0, 0);
		glVertex3f(w0, 0, t); glVertex3f(w0, 0, -t);
		glVertex3f(w1, -s.len, -t); glVertex3f(w1, -s.len, t);
		// -X
		glNormal3f(-1, 0, 0);
		glVertex3f(-w0, 0, -t); glVertex3f(-w0, 0, t);
		glVertex3f(-w1, -s.len, t); glVertex3f(-w1, -s.len, -t);
		glEnd();
		// tiny tip at the end of last segment
		glTranslatef(0, -s.len, 0);
	}
	triWedgeTip(0.10f, 0.30f, 0.08f);
	glPopMatrix();
}

// Central long spear head
static void centerSpear(float L) {
	glPushMatrix();
	colSteel();
	quadBox(0.28f, L * 0.70f, 0.22f);      // main shaft of spear head
	glTranslatef(0, -L * 0.35f, 0);
	triWedgeTip(0.22f, L * 0.35f, 0.18f);  // sharp tip
	glPopMatrix();
}

// ===== Improved trident (takes a length scale) =====
static void drawTridentGeo(float lengthScale = 1.0f) {
	// Handle (longer by default)
	const float handleH = 7.2f * lengthScale;   
	glPushMatrix();
	colBlue();  quadBox(0.22f, handleH, 0.22f);
	// leather bands
	glTranslatef(0, -handleH * 0.42f, 0);
	for (int i = 0;i < 4;++i) {
		glTranslatef(0, -0.18f, 0);
		colLeather(); quadBox(0.26f, 0.10f, 0.26f);
	}
	glPopMatrix();

	// Guard / gold collar + gem
	glPushMatrix();
	glTranslatef(0, +handleH * 0.50f, 0);
	colGold(); quadBox(1.10f, 0.36f, 0.70f);
	glTranslatef(0, +0.14f, 0);
	colGem();  quadBox(0.2f, 0.22f, 0.22f);
	glPopMatrix();

	// Head: center spear + 2 big side blades + 2 small inner hooks
	glPushMatrix();
	glTranslatef(0, handleH * 0.50f + 0.60f, 0);
	glRotatef(180, 1, 0, 0);
	// center spear
	centerSpear(2.2f * lengthScale);

	// side big blades (curvy) — mirror
	glPushMatrix(); glTranslatef(0.85f, 0.25f, 0); sideBladePatch(+1, 2.0f * lengthScale); glPopMatrix();
	glPushMatrix(); glTranslatef(-0.85f, 0.25f, 0); sideBladePatch(-1, 2.0f * lengthScale); glPopMatrix();

	// small inner hooks near collar
	glPushMatrix(); glTranslatef(0.55f, 0.20f, 0); glRotatef(25, 0, 1, 0); quadBox(0.12f, 0.60f, 0.12f); glTranslatef(0, -0.30f, 0); triWedgeTip(0.10f, 0.22f, 0.10f); glPopMatrix();
	glPushMatrix(); glTranslatef(-0.55f, 0.20f, 0); glRotatef(-25, 0, 1, 0); quadBox(0.12f, 0.60f, 0.12f); glTranslatef(0, -0.30f, 0); triWedgeTip(0.10f, 0.22f, 0.10f); glPopMatrix();

	glPopMatrix();
}

static void drawTridentOnBack() {
	const EllipseProfile* T; int TN; float BODY_H;
	getTorsoProfile(T, TN, BODY_H);

	float yn = 0.64f;                     // upper‑mid back
	float a = sampleHalfWidthA(T, TN, yn);
	float b = sampleHalfDepthB(T, TN, yn);
	float y = yn * BODY_H;

	glPushMatrix();
	// start near left shoulder blade, slightly behind skin
	glTranslatef(-a * 0.55f, y + 0.25f, -b * 1.10f);
	// diagonal: top‑left → right‑hip
	glRotatef(14.0f, 1, 0, 0);              // pitch
	glRotatef(25.0f, 0, 1, 0);              // yaw down body
	glRotatef(-58.0f, 0, 0, 1);              // roll diagonal
	drawTridentGeo(/*lengthScale*/1.18f); // a bit longer on the back
	glPopMatrix();
}
static void drawTridentHeldPose() {
	glPushMatrix();
	// grip point ~lower third
	glTranslatef(0.0f, -0.28f, 0.0f);
	// lay along forward (+Z) from the hand
	glRotatef(+90.0f, 1, 0, 0);
	glRotatef(+8.0f, 0, 0, 1);

	// === add motion by state ===
	if (gWpnState == WPN_Z_COMBO) {
		float t = gWpnTimer;
		const float tPrep = 0.18f, tRise = 0.28f, tHang = 0.18f, tFall = 0.30f;

		if (t < tPrep) {
			glRotatef(lerp(0, +22, t / tPrep), 1, 0, 0);       // preload
		}
		else if (t < tPrep + tRise) {
			float u = (t - tPrep) / tRise;
			glRotatef(lerp(+22, +60, u), 1, 0, 0);   // up by face/ear
		}
		else if (t < tPrep + tRise + tHang) {
			glRotatef(60, 1, 0, 0);
		}
		else { // slam
			float u = (t - (tPrep + tRise + tHang)) / tFall;
			glRotatef(lerp(+10, +110, easeInOut(u)), 1, 0, 0);
		}


	}
	else if (gWpnState == WPN_X_CHARGING) {
		// slight tremble + glow length implied elsewhere
		glRotatef(6.0f * sinf(gWpnTimer * 22.0f), 0, 0, 1);
	}
	else if (gWpnState == WPN_X_SWEEP) {
		// wide horizontal sweep
		glRotatef(lerp(-45, +45, gWpnTimer / 0.28f), 0, 1, 0);
	}
	else if (gWpnState == WPN_C_WATERSKIM) {
		// back‑and‑forth short swings
		glRotatef(28.0f * sinf(gWpnTimer * 9.0f), 0, 1, 0);
	}
	// ... existing rotations that orient the weapon in the right hand ...

	GLfloat M[16]; glGetFloatv(GL_MODELVIEW_MATRIX, M);
	float leftGripLocal[3] = { 0.0f, 0.0f, 1.15f };  // along local +Z from the right-hand grip
	mulPointM4(M, leftGripLocal, gOffhandTargetW);
	gOffhandActive = true;

	// draw the actual weapon
	drawTridentGeo(/*lengthScale*/1.15f);

	drawTridentGeo(/*lengthScale*/1.15f);
	glPopMatrix();
}


static void tridentBlade(float L, float W, float T) {
	// center spike
	glPushMatrix();
	setSteel();
	quadBox(W * 0.28f, L, T);
	// tip
	glTranslatef(0, -L * 0.55f, 0);
	glScalef(1.0f, 0.45f, 1.0f);
	quadBox(W * 0.20f, L * 0.45f, T * 0.9f);
	glPopMatrix();

	// side tines (left/right)
	for (int s = -1;s <= 1;s += 2) {
		glPushMatrix();
		glTranslatef(s * W * 0.55f, 0, 0);
		glRotatef(s * 8.0f, 0, 0, 1);
		setSteel();
		quadBox(W * 0.22f, L * 0.85f, T * 0.9f);
		glTranslatef(0, -L * 0.48f, 0);
		quadBox(W * 0.16f, L * 0.35f, T * 0.8f);
		glPopMatrix();
	}
}




typedef void (*DrawFunc)();

static void drawWireOverlay(DrawFunc func) {
	glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_LINE_BIT);
	glDisable(GL_LIGHTING);              // lines not shaded
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_POLYGON_OFFSET_LINE);    // avoid z-fighting
	glPolygonOffset(-1.0f, -1.0f);
	glColor3f(0, 0, 0);                    // black wireframe lines

	if (func) func();                    // call your draw function

	glPopAttrib();
}
// A tapered “rect tube” finger segment along -Y
static void quadFingerSegment(float w0, float w1, float depth, float L) {
	const float x0 = w0 * 0.5f, x1 = w1 * 0.5f, z = depth * 0.5f;
	const float y0 = 0.0f, y1 = -L;

	glBegin(GL_QUADS);
	// dorsal (+Z)
	nrm(0, 0, 1);
	glVertex3f(-x0, y0, z); glVertex3f(x0, y0, z); glVertex3f(x1, y1, z); glVertex3f(-x1, y1, z);
	// palmar (-Z)
	nrm(0, 0, -1);
	glVertex3f(x0, y0, -z); glVertex3f(-x0, y0, -z); glVertex3f(-x1, y1, -z); glVertex3f(x1, y1, -z);
	// radial (+X)
	nrm(1, 0, 0);
	glVertex3f(x0, y0, z); glVertex3f(x0, y0, -z); glVertex3f(x1, y1, -z); glVertex3f(x1, y1, z);
	// ulnar (-X)
	nrm(-1, 0, 0);
	glVertex3f(-x0, y0, -z); glVertex3f(-x0, y0, z); glVertex3f(-x1, y1, z); glVertex3f(-x1, y1, -z);
	glEnd();
}

// A simple triangular fingertip cap (both sides)
static void triFingertip(float w, float depth) {
	const float r = w * 0.5f, z = depth * 0.5f;
	// dorsal (+Z)
	glBegin(GL_TRIANGLES);
	nrm(0, 0, 1);
	glVertex3f(-r, 0, z); glVertex3f(r, 0, z); glVertex3f(0, r * 1.1f, z);
	glEnd();
	// palmar (-Z)
	glBegin(GL_TRIANGLES);
	nrm(0, 0, -1);
	glVertex3f(r, 0, -z); glVertex3f(-r, 0, -z); glVertex3f(0, r * 1.1f, -z);
	glEnd();
}

// ====== DROP‑IN REPLACEMENTS ======

// === FINGER (3 segments, natural curl) ===
static void drawFingerSegmented(float scale = 1.0f,
	float curl1 = 20, float curl2 = 25, float curl3 = 15)
{
	const float L1 = 0.45f * scale;  // proximal
	const float L2 = 0.40f * scale;  // middle
	const float L3 = 0.35f * scale;  // distal

	const float W1 = 0.22f * scale;  // widths (approx twice your R*)
	const float W2 = 0.19f * scale;
	const float W3 = 0.17f * scale;

	const float D = 0.18f * scale;  // thickness

	// Proximal
	quadFingerSegment(W1, W2, D, L1);
	glTranslatef(0, -L1, 0);
	glRotatef(curl1, 1, 0, 0);

	// Middle
	quadFingerSegment(W2, W3, D * 0.95f, L2);
	glTranslatef(0, -L2, 0);
	glRotatef(curl2, 1, 0, 0);

	// Distal
	quadFingerSegment(W3, W3 * 0.9f, D * 0.9f, L3);
	glTranslatef(0, -L3, 0);
	glRotatef(curl3, 1, 0, 0);

	// Tip
	triFingertip(W3 * 0.9f, D * 0.85f);
}

// === HAND (palm + 4 fingers + thumb) ===
static void drawHandNatural(int side /* -1 left, +1 right */)
{
	const float PALM_W = 0.90f;  // across knuckles (Z)
	const float PALM_H = 0.60f;  // wrist->knuckles (Y)
	const float PALM_T = 0.35f;  // thickness (X)

	glEnable(GL_NORMALIZE);

	// 1) Palm block (low‑poly)
	glPushMatrix();
	glTranslatef(0, -PALM_H * 0.2f, 0);
	quadBox(PALM_T, PALM_H * 0.8f, PALM_W * 0.65f);
	glPopMatrix();

	// 2) Move to knuckle line (top of palm in your space)
	glTranslatef(0, -PALM_H * 0.45f, 0);

	// 3) Fingers (index..pinky spaced along Z; pinky shorter)
	const float zStart = -PALM_W * 0.45f;
	const float zStep = PALM_W * 0.30f;

	for (int i = 0;i < 4;++i) {
		const float z = zStart + i * zStep;
		const float scale = 1.0f - 0.08f * (3 - i); // longest: index->middle slight bias; tweak if you prefer
		glPushMatrix();
		glTranslatef(0, 0, z);
		// tiny splay around Z to mimic ref
		glRotatef(-6.0f + i * 4.0f, 0, 0, 1);
		drawFingerSegmented(scale, 18 + 2 * i, 22 + i, 12 + i);
		glPopMatrix();
	}

	// 4) Thumb mount (uses 'side' for left/right)
	glPushMatrix();
	glTranslatef(side * (PALM_T * 0.35f + 0.08f), -PALM_H * 0.20f, PALM_W * 0.20f);
	glRotatef(side * -25.0f, 0, 0, 1);  // splay from palm
	glRotatef(20.0f, 0, 1, 0);          // bring forward
	drawFingerSegmented(0.85f, 15, 18, 12);
	glPopMatrix();
}

// --- Natural finger (MCP -> PIP -> DIP) ---
static void drawFingerNatural(float splayDeg, float yawDeg,
	float rollDeg, float curl1, float curl2, float curl3,
	float scale = 1.0f)
{
	const float L1 = 0.55f * scale, L2 = 0.45f * scale, L3 = 0.38f * scale;
	const float W1 = 0.23f * scale, W2 = 0.20f * scale, W3 = 0.17f * scale;
	const float D = 0.18f * scale;

	glPushMatrix();
	glRotatef(splayDeg, 0, 1, 0); // spread
	glRotatef(yawDeg, 0, 1, 0); // twist (around Y)
	glRotatef(rollDeg, 0, 0, 1); // lateral roll

	// Proximal
	quadFingerSegment(W1, W2, D, L1);
	glTranslatef(0, -L1, 0); glRotatef(curl1, 1, 0, 0);

	// Middle
	quadFingerSegment(W2, W3, D * 0.95f, L2);
	glTranslatef(0, -L2, 0); glRotatef(curl2, 1, 0, 0);

	// Distal + tip
	quadFingerSegment(W3, W3 * 0.9f, D * 0.9f, L3);
	glTranslatef(0, -L3, 0);
	triFingertip(W3 * 0.9f, D * 0.85f);

	glRotatef(curl3, 1, 0, 0);
	glPopMatrix();
}

// ======== QUADS/TRIS ONLY PRIMITIVES ========

static inline void vnorm3f(float x, float y, float z) {
    // unit-length normal (cheap normalize)
    float m = sqrtf(x*x + y*y + z*z); if (m < 1e-6f) m = 1.0f;
    glNormal3f(x/m, y/m, z/m);
}

// Connect two elliptical rings with QUADS, along -Y.
// slices = 16..32 looks nice (keep even for clean loops).
static void connectRingsQuads(int slices,
                              float y0, float rx0, float rz0, float twist0Deg,
                              float y1, float rx1, float rz1, float twist1Deg)
{
    const float t0 = twist0Deg * 3.1415926f / 180.0f;
    const float t1 = twist1Deg * 3.1415926f / 180.0f;

    glBegin(GL_QUADS);
    for (int i = 0; i < slices; ++i) {
        int j = (i + 1) % slices;
        float a0 = (2.0f * 3.1415926f * i) / slices, a1 = (2.0f * 3.1415926f * j) / slices;

        // ring 0
        float c0 = cosf(a0 + t0), s0 = sinf(a0 + t0);
        float x0 = rx0 * c0, z0 = rz0 * s0;
        float nx0 = c0 / (rx0 > 1e-6f ? 1.0f : 1.0f);
        float nz0 = s0 / (rz0 > 1e-6f ? 1.0f : 1.0f);

        float c1 = cosf(a1 + t0), s1 = sinf(a1 + t0);
        float x1 = rx0 * c1, z1 = rz0 * s1;
        float nx1 = c1, nz1 = s1;

        // ring 1
        float C0 = cosf(a0 + t1), S0 = sinf(a0 + t1);
        float X0 = rx1 * C0, Z0 = rz1 * S0;
        float NX0 = C0, NZ0 = S0;

        float C1 = cosf(a1 + t1), S1 = sinf(a1 + t1);
        float X1 = rx1 * C1, Z1 = rz1 * S1;
        float NX1 = C1, NZ1 = S1;

        // QUAD (v0->v1->V1->V0)
        vnorm3f(nx0, 0, nz0); glVertex3f(x0, y0, z0);
        vnorm3f(nx1, 0, nz1); glVertex3f(x1, y0, z1);
        vnorm3f(NX1, 0, NZ1); glVertex3f(X1, y1, Z1);
        vnorm3f(NX0, 0, NZ0); glVertex3f(X0, y1, Z0);
    }
    glEnd();
}

// Make an elliptical “tube section” along -Y with optional twist from start→end.
static void tubeSectionQuads(int slices, float L,
                             float rxStart, float rzStart,
                             float rxEnd,   float rzEnd,
                             float twistDegStart = 0.0f,
                             float twistDegEnd   = 0.0f)
{
    connectRingsQuads(slices, 0.0f, -rxStart, -rzStart, twistDegStart,  // flip signs to keep normals outward
                               -L,   -rxEnd,   -rzEnd,   twistDegEnd);
}

// Simple end cap (TRIANGLES) for an ellipse at Y = y, facing +Y or -Y (dir = +1/-1).
static void ellipseCapTri(int slices, float y, float rx, float rz, int dir)
{
    float ny = (dir > 0 ? 1.0f : -1.0f);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < slices; ++i) {
        int j = (i + 1) % slices;
        float a0 = (2.0f * 3.1415926f * i) / slices;
        float a1 = (2.0f * 3.1415926f * j) / slices;

        vnorm3f(0, ny, 0); glVertex3f(0, y, 0);
        vnorm3f(0, ny, 0); glVertex3f(rx * cosf(a1), y, rz * sinf(a1));
        vnorm3f(0, ny, 0); glVertex3f(rx * cosf(a0), y, rz * sinf(a0));
    }
    glEnd();
}



// --- Full arm (shoulder to wrist). side = -1 left, +1 right
static void drawArmDown(int side)
{
	const EllipseProfile* T; int TN; float BODY_H;
	getTorsoProfile(T, TN, BODY_H);

	// skin
	const SkinPreset& SKIN = SKIN_LIGHT_TAN;
	applySkinMaterial(SKIN.base);
	if (!lightingEnabled()) glColor3f(SKIN.base[0], SKIN.base[1], SKIN.base[2]); else glColor3f(1, 1, 1);

	// placement at shoulder
	const float SHOULDER_YN = 0.78f;
	const float a = sampleA(T, TN, SHOULDER_YN);
	const float b = sampleB(T, TN, SHOULDER_YN);
	const float yS = SHOULDER_YN * BODY_H;
	const float xS = side * (a * 0.95f);
	const float zS = b * 0.25f;

	// proportions (tuned to your screenshots)
	const int   SLICES = 20;     // grid density
	const float UPPER_LEN = 2.40f;  // shoulder -> elbow
	const float FORE_LEN = 2.35f;  // elbow -> wrist

	// radius sets (elliptical; X = thickness, Z = width)
	// Shoulder “cap” → upper arm → elbow → forearm → wrist
	const float SHO_RX = 0.55f, SHO_RZ = 0.50f;    // deltoid cap (round)
	const float HUM_RX0 = 0.42f, HUM_RZ0 = 0.36f;  // proximal humerus (full)
	const float HUM_RX1 = 0.34f, HUM_RZ1 = 0.30f;  // mid humerus (taper)
	const float HUM_RX2 = 0.30f, HUM_RZ2 = 0.27f;  // near elbow (narrowest)

	const float ELB_RX = 0.33f, ELB_RZ = 0.30f;  // elbow olecranon “ball” (lo‑poly)
	const float PRO_RX0 = 0.33f, PRO_RZ0 = 0.28f;  // proximal forearm bulk (brachioradialis)
	const float PRO_RX1 = 0.29f, PRO_RZ1 = 0.26f;  // mid‑forearm
	const float DST_RX = 0.22f, DST_RZ = 0.20f;  // wrist

	// small lengths for shaping transitions
	const float SHO_CAP = 0.35f;     // shoulder cap height
	const float ELB_SEG = 0.30f;     // elbow bulge height
	const float WRIST_SEG = 0.22f;   // wrist collar height

	// pronation twist across the forearm (like your +8°)
	const float TWIST_FORE = 8.0f * (float)side;

	// arm pose
	const float ARM_YAW = 6.0f;  // slight inward
	const float ARM_PITCH = 4.0f;  // slight forward

	glPushMatrix();
	glTranslatef(xS, yS, zS);

	glRotatef(ARM_YAW * (float)side, 0, 1, 0);


	//if (side < 0 && gOffhandActive) {
	//	// Convert world target -> this shoulder's local
	//	GLfloat Msh[16], Inv[16];
	//	glGetFloatv(GL_MODELVIEW_MATRIX, Msh);
	//	invRigidM4(Msh, Inv);
	//	float pL[3]; mulPointM4(Inv, gOffhandTargetW, pL); // local target

	//	// Shoulder aim (yaw around Y, pitch around X)
	//	float yaw = rad2deg(atan2f(pL[0], pL[2]));                  // left/right
	//	float pitch = -rad2deg(atan2f(pL[1], sqrtf(pL[0] * pL[0] + pL[2] * pL[2]))); // up/down
	//	glRotatef(yaw, 0, 1, 0);
	//	glRotatef(pitch, 1, 0, 0);

	//	// Skip additive action rotations for the left arm (we’re aiming it)
	//}
	//else {
	//	// original additive action for left arm:
	//	glRotatef(actLShoulderYaw, 0, 1, 0);
	//	glRotatef(actLShoulderPitch, 1, 0, 0);
	//	glRotatef(actLShoulderRoll, 0, 0, 1);
	//}


	// swing arms opposite to legs: right arm in phase, left arm π out of phase
	float armSwing = sinf(gWalkPhase + (side > 0 ? 0.0f : 3.14159265f)) * gArmAmpDeg;
	glRotatef(ARM_PITCH + armSwing, 1, 0, 0);
	// ===== additive action shoulder pose =====
	if (side > 0) { // right
		glRotatef(actRShoulderYaw, 0, 1, 0);
		glRotatef(actRShoulderPitch, 1, 0, 0);
		glRotatef(actRShoulderRoll, 0, 0, 1);
	}
	else {          // left
		glRotatef(actLShoulderYaw, 0, 1, 0);
		glRotatef(actLShoulderPitch, 1, 0, 0);
		glRotatef(actLShoulderRoll, 0, 0, 1);
	}


	// ---------- SHOULDER CAP (two short dome-like sections, quads only) ----------
	glPushMatrix();
	// Section A: very short bulge from torso to deltoid roundness
	tubeSectionQuads(SLICES, SHO_CAP * 0.5f,
		SHO_RX * 0.75f, SHO_RZ * 0.72f,
		SHO_RX, SHO_RZ, 0, 0);
	glTranslatef(0, -SHO_CAP * 0.5f, 0);

	// Section B: blend into proximal humerus (slightly narrower, starts arm)
	tubeSectionQuads(SLICES, SHO_CAP * 0.5f,
		SHO_RX, SHO_RZ,
		HUM_RX0, HUM_RZ0, 0, 0);
	glPopMatrix();

	// Move down from the shoulder cap
	glTranslatef(0, -SHO_CAP, 0);

	// ---------- UPPER ARM (two longer sections for deltoid→triceps taper) ----------
	// Upper arm A: gentle taper (deltoid bulk → mid humerus)
	tubeSectionQuads(SLICES, UPPER_LEN * 0.55f,
		HUM_RX0, HUM_RZ0,
		HUM_RX1, HUM_RZ1, 0, 0);
	glTranslatef(0, -(UPPER_LEN * 0.55f), 0);

	// Upper arm B: stronger taper toward elbow
	tubeSectionQuads(SLICES, UPPER_LEN * 0.45f,
		HUM_RX1, HUM_RZ1,
		HUM_RX2, HUM_RZ2, 0, 0);
	glTranslatef(0, -(UPPER_LEN * 0.45f), 0);

	// ---------- ELBOW (short “ball” made of two tapered tube slices) ----------
	// Elbow A: expand a touch
	tubeSectionQuads(SLICES, ELB_SEG * 0.5f,
		HUM_RX2, HUM_RZ2,
		ELB_RX, ELB_RZ, 0, 0);
	glTranslatef(0, -(ELB_SEG * 0.5f), 0);
	// Elbow B: contract again (gives the olecranon bump)
	tubeSectionQuads(SLICES, ELB_SEG * 0.5f,
		ELB_RX, ELB_RZ,
		PRO_RX0, PRO_RZ0, 0, 0);
	glTranslatef(0, -(ELB_SEG * 0.5f), 0);
	// --- ELBOW (after you place the elbow sphere) ---
	if (side > 0) {
		glRotatef(actRElbowFlex, 1, 0, 0);
	}
	else {
		float pL[3] = { 0,0,0 };   // local grip target for left arm
		bool havePL = false;

		//if (side < 0 && gOffhandActive) {
		//	// Convert world target -> this shoulder's local
		//	GLfloat Msh[16], Inv[16];
		//	glGetFloatv(GL_MODELVIEW_MATRIX, Msh);
		//	invRigidM4(Msh, Inv);
		//	mulPointM4(Inv, gOffhandTargetW, pL);
		//	havePL = true;

		//	// Shoulder aim
		//	float yaw = rad2deg(atan2f(pL[0], pL[2]));
		//	float pitch = -rad2deg(atan2f(pL[1], sqrtf(pL[0] * pL[0] + pL[2] * pL[2])));
		//	glRotatef(yaw, 0, 1, 0);
		//	glRotatef(pitch, 1, 0, 0);
		//}
		//
		//else {
			//glRotatef(actLElbowFlex, 1, 0, 0);
		//}
	}

	// Add elbow flex from action pose
	if (side > 0) glRotatef(actRElbowFlex, 1, 0, 0);
	else          glRotatef(actLElbowFlex, 1, 0, 0);

	// ---------- FOREARM (subtle pronation twist) ----------
	// Proximal forearm bulk → mid
	tubeSectionQuads(SLICES, FORE_LEN * 0.58f,
		PRO_RX0, PRO_RZ0,
		PRO_RX1, PRO_RZ1,
		0.0f, TWIST_FORE * 0.5f);
	glTranslatef(0, -(FORE_LEN * 0.58f), 0);

	// Mid → distal wrist taper (continue twist)
	tubeSectionQuads(SLICES, FORE_LEN * 0.42f,
		PRO_RX1, PRO_RZ1,
		DST_RX, DST_RZ,
		TWIST_FORE * 0.5f, TWIST_FORE);
	glTranslatef(0, -(FORE_LEN * 0.42f), 0);

	// ---------- WRIST COLLAR (short tightening to match hand root) ----------
	tubeSectionQuads(SLICES, WRIST_SEG,
		DST_RX, DST_RZ,
		DST_RX * 0.94f, DST_RZ * 0.94f,
		TWIST_FORE, TWIST_FORE);
	glTranslatef(0, -WRIST_SEG, 0);

	// ---------- WRIST / HAND ORIENTATION ----------
	float wristFlex = -10.0f;           // neutral bend
	float wristRoll = 8.0f * side;      // small inward roll by default

	if (side > 0) { // weapon hand (right)
		if (gWpnState == WPN_X_SWEEP) {
			// Overhand grip (palm-down), not upside down
			// add a little untwist through the sweep for life
			float t = clamp01(gWpnTimer / 0.28f);
			wristRoll = -70.0f + 20.0f * (t - 0.5f);   // start rolled-in, ease out a bit
			wristFlex = -8.0f;
		}
		else if (gWpnState == WPN_X_CHARGING) {
			// keep overhand while drawing back
			wristRoll = -50.0f;
			wristFlex = -6.0f;
		}
	}

	glRotatef(wristFlex, 1, 0, 0);   // bend
	glRotatef(wristRoll, 0, 0, 1);   // roll controls palm up/down


	// draw hand (your QUAD/TRI version)
	if (side > 0) {
		drawHandNatural(+1);
		if (showModelLines) drawWireOverlay([] {drawHandNatural(+1);});
	}
	else {
		drawHandNatural(-1);
		if (showModelLines) drawWireOverlay([] {drawHandNatural(-1);});

	}

	if (side > 0) {
		if (gWpnState == WPN_IN_HAND || gWpnState == WPN_Z_COMBO || gWpnState == WPN_X_CHARGING || gWpnState == WPN_X_SWEEP || gWpnState == WPN_C_WATERSKIM) {
			drawTridentHeldPose();
		}
	}

	if (side < 0) gOffhandActive = false;   // left arm consumed the target this frame



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

	// ==================== Roman Caligae (GL_QUADS only) ====================
	{
		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_POLYGON_BIT);
		glDisable(GL_TEXTURE_2D);
		// Leather look
		glColor3f(0.35f, 0.22f, 0.15f);
		glPushMatrix();
		glScalef(1, 1.2,1.3);
		glRotatef(180, 0, 0, 1.1);
		glRotatef(5, 1,0, 0);
		glTranslatef(0, 1.4, -0.16);
		const float SOLE_T = THICK * 0.18f;         // outsole thickness
		const float SOLE_Z0 = -SOLE_T;              // bottom of sole
		const float SOLE_Z1 = 0.0f;                 // top of sole (touching foot)
		const float X0 = -WIDTH * 0.50f, X1 = +WIDTH * 0.50f;
		const float Y0 = -LEN * 0.98f, Y1 = +LEN * 0.02f;

		// Helper: box with quads
		auto boxQuads = [](float x0, float x1, float y0, float y1, float z0, float z1)
			{
				glBegin(GL_QUADS);
				// bottom
				glVertex3f(x0, y0, z0); glVertex3f(x1, y0, z0); glVertex3f(x1, y1, z0); glVertex3f(x0, y1, z0);
				// top
				glVertex3f(x0, y0, z1); glVertex3f(x0, y1, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y0, z1);
				// sides
				glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0);
				glVertex3f(x1, y0, z0); glVertex3f(x1, y0, z1); glVertex3f(x1, y1, z1); glVertex3f(x1, y1, z0);
				glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
				glVertex3f(x0, y1, z0); glVertex3f(x0, y1, z1); glVertex3f(x0, y0, z1); glVertex3f(x0, y0, z0);
				glEnd();
			};

		// --- Sole (simple footprint box, slightly tapered at toe/heel with two segments) ---
		// heel block
		boxQuads(X0 * 0.90f, X1 * 0.90f, Y1 - LEN * 0.22f, Y1, SOLE_Z0, SOLE_Z1);
		// mid + toe block (a bit narrower at the very front)
		boxQuads(X0, X1, Y0, Y1 - LEN * 0.22f, SOLE_Z0, SOLE_Z1);
		boxQuads(X0 * 0.85f, X1 * 0.85f, Y0, Y0 + LEN * 0.10f, SOLE_Z0, SOLE_Z1);

		// Raise straps slightly above the sole
		const float STRAP_Z0 = SOLE_Z1 + THICK * 0.08f;
		const float STRAP_Z1 = STRAP_Z0 + THICK * 0.08f;
		const float STRAP_OVERHANG = WIDTH * 0.02f; // small overhang beyond sole edge

		// Helper: a transverse strap band across the foot
		auto strapAcross = [&](float yCenter, float halfWidthScale, float halfDepth)
			{
				const float xs = halfWidthScale * (WIDTH * 0.50f + STRAP_OVERHANG);
				boxQuads(-xs, +xs, yCenter - halfDepth, yCenter + halfDepth, STRAP_Z0 * 8, STRAP_Z1 );
			};

		// Helper: a vertical strap rising from the side (for the cage)
		auto sideUpright = [&](float xEdge, float y0, float y1, float height)
			{
				boxQuads(xEdge - WIDTH * 0.04f, xEdge + WIDTH * 0.04f, y0, y1, STRAP_Z1, STRAP_Z1 + height);
			};

		// --- Transverse instep straps (like caligae) ---
		strapAcross(Y0 + LEN * 0.75f, 0.95f, LEN * 0.022f); // near toes
		strapAcross(Y0 + LEN * 0.62f, 0.93f, LEN * 0.022f);
		strapAcross(Y0 + LEN * 0.50f, 0.92f, LEN * 0.022f);
		strapAcross(Y0 + LEN * 0.38f, 0.90f, LEN * 0.022f);
		strapAcross(Y0 + LEN * 0.27f, 0.88f, LEN * 0.022f);
		strapAcross(Y0 + LEN * 0.18f, 0.86f, LEN * 0.022f);

		// --- Diagonal front strap (iconic caligae look) ---
		{
			const float yA = Y0 + LEN * 0.30f;
			const float yB = Y0 + LEN * 0.55f;
			const float xA = -WIDTH * 0.45f;
			const float xB = +WIDTH * 0.45f;
			const float t = WIDTH * 0.11f; // strap thickness in X
			glBegin(GL_QUADS);
			// build as a thin box slanted in XY
			// bottom
			glVertex3f(xA, yA, STRAP_Z0); glVertex3f(xA + t, yA, STRAP_Z0);
			glVertex3f(xB + t, yB, STRAP_Z0); glVertex3f(xB, yB, STRAP_Z0);
			// top
			glVertex3f(xA, yA, STRAP_Z1); glVertex3f(xB, yB, STRAP_Z1);
			glVertex3f(xB + t, yB, STRAP_Z1); glVertex3f(xA + t, yA, STRAP_Z1);
			// sides
			glVertex3f(xA, yA, STRAP_Z0); glVertex3f(xA, yA, STRAP_Z1);
			glVertex3f(xA + t, yA, STRAP_Z1); glVertex3f(xA + t, yA, STRAP_Z0);

			glVertex3f(xB + t, yB, STRAP_Z0); glVertex3f(xB + t, yB, STRAP_Z1);
			glVertex3f(xB, yB, STRAP_Z1); glVertex3f(xB, yB, STRAP_Z0);

			glVertex3f(xA + t, yA, STRAP_Z0); glVertex3f(xB + t, yB, STRAP_Z0);
			glVertex3f(xB + t, yB, STRAP_Z1); glVertex3f(xA + t, yA, STRAP_Z1);

			glVertex3f(xB, yB, STRAP_Z0); glVertex3f(xA, yA, STRAP_Z0);
			glVertex3f(xA, yA, STRAP_Z1); glVertex3f(xB, yB, STRAP_Z1);
			glEnd();
		}

		// --- Side uprights that connect to ankle area (both sides) ---
		const float cageH = THICK * 0.70f; // rises up the ankle
		sideUpright(+WIDTH * 0.50f, Y0 + LEN * 0.25f, Y0 + LEN * 0.55f, cageH);
		sideUpright(-WIDTH * 0.50f, Y0 + LEN * 0.20f, Y0 + LEN * 0.50f, cageH);
		sideUpright(+WIDTH * 0.50f, Y0 + LEN * 0.05f, Y0 + LEN * 0.30f, cageH * 0.85f);
		sideUpright(-WIDTH * 0.50f, Y0 + LEN * 0.00f, Y0 + LEN * 0.25f, cageH * 0.85f);

		// --- Two ankle bands around the leg (approximated with 16-faced ring of quads) ---
		auto ankleBand = [&](float zBase, float yCenter)
			{
				const int N = 16;
				const float rIn = WIDTH * 0.55f;
				const float rOut = rIn + WIDTH * 0.10f;
				const float zTop = zBase + THICK * 0.10f;

				for (int i = 0;i < N;i++) {
					float a0 = (float)i * (2.0f * (float)M_PI / N);
					float a1 = (float)(i + 1) * (2.0f * (float)M_PI / N);

					float x0i = rIn * cosf(a0), x1i = rIn * cosf(a1);
					float y0i = rIn * sinf(a0), y1i = rIn * sinf(a1);
					float x0o = rOut * cosf(a0), x1o = rOut * cosf(a1);
					float y0o = rOut * sinf(a0), y1o = rOut * sinf(a1);

					// shift to ankle center
					y0i += yCenter; y1i += yCenter; y0o += yCenter; y1o += yCenter;

					glBegin(GL_QUADS);
					// outer wall
					glVertex3f(x0o, y0o, zBase); glVertex3f(x1o, y1o, zBase);
					glVertex3f(x1o, y1o, zTop); glVertex3f(x0o, y0o, zTop);
					// inner wall
					glVertex3f(x1i, y1i, zBase); glVertex3f(x0i, y0i, zBase);
					glVertex3f(x0i, y0i, zTop); glVertex3f(x1i, y1i, zTop);
					// top
					glVertex3f(x0i, y0i, zTop); glVertex3f(x1i, y1i, zTop);
					glVertex3f(x1o, y1o, zTop); glVertex3f(x0o, y0o, zTop);
					// bottom
					glVertex3f(x0o, y0o, zBase); glVertex3f(x1o, y1o, zBase);
					glVertex3f(x1i, y1i, zBase); glVertex3f(x0i, y0i, zBase);
					glEnd();
				}
			};
		// positions a bit above instep, nearer to ankle
		ankleBand(STRAP_Z1 + THICK * 0.40f, Y0 + LEN * 0.02f);
		ankleBand(STRAP_Z1 + THICK * 0.58f, Y0 + LEN * 0.02f);
		glPopMatrix();
		glPopAttrib();
	}
	// ================= end Roman Caligae =================


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
	const float THIGH_R0 = 0.62f, THIGH_R1 = 0.00f;
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
	// Add hip pitch from action
	if (side > 0) glRotatef(actHipPitchR, 1, 0, 0);
	else          glRotatef(actHipPitchL, 1, 0, 0);

	glTranslatef(0, -THIGH_LEN, 0);
	drawSphere(KNEE_R, 20, 20);

	// legs swing opposite to arms: right leg π out of phase vs right arm
	float legSwing = -sinf(gWalkPhase + (side > 0 ? 3.14159265f : 0.0f)) * gLegAmpDeg;
	glRotatef(legSwing, 1, 0, 0);


	// --- sarong panel that follows this leg ---
	glPushMatrix();
	glTranslatef(0.0f * side, 0.0f, 0.2f);
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
	float kneeFlex = (side > 0 ? actKneeFlexR : actKneeFlexL);
	glRotatef(1.5f + kneeFlex, 1, 0, 0);

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

struct V3 { float x, y, z; };
static inline V3 sub(V3 a, V3 b) { return { a.x - b.x,a.y - b.y,a.z - b.z }; }
static inline V3 cross(V3 a, V3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
static inline void nrm(V3 a, V3 b, V3 c) {
	V3 n = cross(sub(b, a), sub(c, a));
	float L = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z) + 1e-8f;
	glNormal3f(n.x / L, n.y / L, n.z / L);
}

static void drawCrownGLT(float r, float t, float bh, float sh,
	int slices = 60, int step = 6)
{
	const float rIn = r;
	const float rOut = r + t;
	const float y0 = 0.0f;     // band bottom
	const float y1 = bh;       // band top
	const float dA = 2.0f * M_PI / (float)slices;

	glBegin(GL_TRIANGLES);

	for (int i = 0;i < slices;i++) {
		int j = (i + 1) % slices;
		float a0 = i * dA, a1 = j * dA;

		// ring points
		V3 o00 = { rOut * cosf(a0), y0, rOut * sinf(a0) };
		V3 o01 = { rOut * cosf(a1), y0, rOut * sinf(a1) };
		V3 o10 = { rOut * cosf(a0), y1, rOut * sinf(a0) };
		V3 o11 = { rOut * cosf(a1), y1, rOut * sinf(a1) };

		V3 i00 = { rIn * cosf(a0), y0, rIn * sinf(a0) };
		V3 i01 = { rIn * cosf(a1), y0, rIn * sinf(a1) };
		V3 i10 = { rIn * cosf(a0), y1, rIn * sinf(a0) };
		V3 i11 = { rIn * cosf(a1), y1, rIn * sinf(a1) };

		// ----- BAND: outer wall (two triangles) -----
		nrm(o00, o01, o11); glVertex3f(o00.x, o00.y, o00.z); glVertex3f(o01.x, o01.y, o01.z); glVertex3f(o11.x, o11.y, o11.z);
		nrm(o00, o11, o10); glVertex3f(o00.x, o00.y, o00.z); glVertex3f(o11.x, o11.y, o11.z); glVertex3f(o10.x, o10.y, o10.z);

		// ----- BAND: inner wall (flip winding) -----
		nrm(i01, i00, i10); glVertex3f(i01.x, i01.y, i01.z); glVertex3f(i00.x, i00.y, i00.z); glVertex3f(i10.x, i10.y, i10.z);
		nrm(i01, i10, i11); glVertex3f(i01.x, i01.y, i01.z); glVertex3f(i10.x, i10.y, i10.z); glVertex3f(i11.x, i11.y, i11.z);

		// ----- BAND: top cap (between inner & outer rims) -----
		nrm(o10, o11, i11); glVertex3f(o10.x, o10.y, o10.z); glVertex3f(o11.x, o11.y, o11.z); glVertex3f(i11.x, i11.y, i11.z);
		nrm(o10, i11, i10); glVertex3f(o10.x, o10.y, o10.z); glVertex3f(i11.x, i11.y, i11.z); glVertex3f(i10.x, i10.y, i10.z);

		// ----- BAND: bottom cap (optional; keeps it solid) -----
		nrm(o01, o00, i00); glVertex3f(o01.x, o01.y, o01.z); glVertex3f(o00.x, o00.y, o00.z); glVertex3f(i00.x, i00.y, i00.z);
		nrm(o01, i00, i01); glVertex3f(o01.x, o01.y, o01.z); glVertex3f(i00.x, i00.y, i00.z); glVertex3f(i01.x, i01.y, i01.z);

		// ----- SPIKES every 'step' segments -----
		if (i % step == 0) {
			float am = 0.5f * (a0 + a1);
			V3 tip = { (rOut - 0.25f * t) * cosf(am), y1 + sh, (rOut - 0.25f * t) * sinf(am) };

			// front wedge (outer edge)
			nrm(o10, o11, tip); glVertex3f(o10.x, o10.y, o10.z); glVertex3f(o11.x, o11.y, o11.z); glVertex3f(tip.x, tip.y, tip.z);
			// back wedge (inner edge) for thickness
			nrm(i11, i10, tip); glVertex3f(i11.x, i11.y, i11.z); glVertex3f(i10.x, i10.y, i10.z); glVertex3f(tip.x, tip.y, tip.z);
			// close left and right sides so it’s watertight
			nrm(o10, i10, tip); glVertex3f(o10.x, o10.y, o10.z); glVertex3f(i10.x, i10.y, i10.z); glVertex3f(tip.x, tip.y, tip.z);
			nrm(i11, o11, tip); glVertex3f(i11.x, i11.y, i11.z); glVertex3f(o11.x, o11.y, o11.z); glVertex3f(tip.x, tip.y, tip.z);
		}
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
	setSkin(SKIN.areola[0], SKIN.areola[1], SKIN.areola[2]);

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


	// ================= King Crown (triangles) =================
	glPushMatrix();
	glColor3f(1.00f, 0.84f, 0.00f);                  // gold
	// sit just above the hair line
	glTranslatef(0.0f, SKULL_RY * 0.95f, 0.0f);

	// build a unit crown and scale to skull ellipse
	glScalef(SKULL_RX * 1.02f, SKULL_RY * 0.30f, SKULL_RZ * 1.02f);
	// parameters: innerR, thickness, bandHeight, spikeHeight
	drawCrownGLT(/*r*/1.00f, /*t*/0.12f, /*bh*/0.35f, /*sh*/0.55f,
		/*slices*/60, /*step*/6);
	glPopMatrix();

	glPopMatrix(); // root at torso neck
}


void poseidon() {
	glClearColor(0.2,0.2,0.2 ,0.0);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);

	projection();

	// in poseidon(), before drawing your model (after clears/projection)
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);   
	glDisable(GL_LINE_SMOOTH);                  
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
	glRotatef(actTorsoYaw, 0, 1, 0);
	glRotatef(actTorsoPitch, 1, 0, 0);
	glRotatef(actTorsoRoll, 0, 0, 1);


	glPushMatrix();
	glTranslatef(0, -4, 0);
	leftleg();
	rightleg();
	glPopMatrix();

	// Weapon on back when not equipped
	if (gWpnState == WPN_ON_BACK || gWpnState == WPN_EQUIP_ANIM)
		drawTridentOnBack();
	
	


	// C water trail (world-space)
	renderWaterTrail();
	if (gWpnState == WPN_X_SWEEP) {
		glPushMatrix();
		glTranslatef(xPosition, yPosition, zPosition);
		glRotatef(rotateY + actTorsoYaw, 0, 1, 0);  
		drawShockwave(6.8f + 1.2f * gWpnCharge);
		glPopMatrix();
	}

	// Z Hit effect
	drawGroundImpactRing();


	// draw pelvis-anchored belt/back once, using last hip ring values, continuous skirt that hugs the body at the top (no gap)
	drawSarongHuggingTorso(gPelvisY);

	
	


	glPopMatrix();

	glPopMatrix();

	glDeleteTextures(1, &textureArr[0]);
	glDeleteTextures(1, &textureArr[1]);
	glDeleteTextures(1, &textureArr[2]);
	glDeleteTextures(1, &textureArr[3]);
	glDeleteTextures(1, &textureArr[4]);
	glDisable(GL_TEXTURE_2D);


	// Walking action
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