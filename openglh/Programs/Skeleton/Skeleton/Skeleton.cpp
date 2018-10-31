//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2018. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Bárányos András
// Neptun : KAIKQ6
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0
	layout(location = 1) in vec2 vertexUV;			// Attrib Array 1

	out vec2 texCoord;								// output attribute

	void main() {
		texCoord = vertexUV;
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	uniform sampler2D textureUnit;
	uniform int separator;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		if (separator == 0){
			outColor = vec4(color, 1);
		}else{
			outColor = texture(textureUnit, texCoord);
		}	
	}
)";

struct controlPoint {
	vec4 point;
	vec2 speed;
	controlPoint(vec4 p, vec2 s) {
		point = p;
		speed = s;
	}
};

class Camera2D {
	vec2 wCenter; // center in world coordinates
	vec2 wSize;   // width and height in world coordinates
public:
	Camera2D() : wCenter(0, 0), wSize(40, 40) { }

	mat4 V() { return TranslateMatrix(-wCenter); }
	mat4 P() { return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y)); }

	mat4 Vinv() { return TranslateMatrix(wCenter); }
	mat4 Pinv() { return ScaleMatrix(vec2(wSize.x / 2, wSize.y / 2)); }

	void Zoom(float s) { wSize = wSize * s; }
	void Pan(vec2 t) { wCenter = wCenter + t; }
};

GPUProgram gpuProgram; // vertex and fragment shaders
Camera2D camera;

class Cart {
	unsigned int vao, vbo[2];
	vec2 vertices[4], uvs[4];
	Texture * pTexture;
	float sx, sy;		// scaling
	vec2 wTranslate;	// translation
	float phi;			// angle of rotation

public:
	Cart() {
		vertices[0] = vec2(-5, -5); uvs[0] = vec2(0, 0);
		vertices[1] = vec2(5, -5);  uvs[1] = vec2(1, 0);
		vertices[2] = vec2(5, 5);   uvs[2] = vec2(1, 1);
		vertices[3] = vec2(-5, 5);  uvs[3] = vec2(0, 1);

		sx = sy = 1;
	}

	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		glGenBuffers(2, vbo);	// Generate 1 vertex buffer objects

		// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);	   // copy to that part of the memory which will be modified 
		// Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
		// Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

		int width = 128, height = 128;
		std::vector<vec4> image(width * height);
		float a = 30.0f;
		float b = 15.0f;
		vec2 center = vec2(64, 64);
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				float ellipszis = pow(x - center.x, 2) / pow(a, 2) + pow(y - center.y, 2) / pow(b, 2);
				if (ellipszis < 1) {
					image[y * width + x] = vec4(1, 1, 1, 1);
				}
				else {
					image[y * width + x] = vec4(0, 0, 0, 1);
				}
			}
		}

		pTexture = new Texture(width, height, image);
	}

	void Animate(float t) {
		sx = 1;
		sy = 1;
		wTranslate = vec2(0, 0);
		phi = t;
	}

	void rotate(float t) {
		phi = phi + t;
	}

	void move(vec2 vec) {
		wTranslate = vec;
	}

	mat4 M() {
		mat4 Mscale(sx, 0, 0, 0,
			0, sy, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 1); // scaling

		mat4 Mrotate(cosf(phi), sinf(phi), 0, 0,
			-sinf(phi), cosf(phi), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1); // rotation

		mat4 Mtranslate(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 0,
			wTranslate.x, wTranslate.y, 0, 1); // translation

		return Mscale * Mrotate * Mtranslate;	// model transformation
	}

	void Draw() {
		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source

		mat4 MVPTransform = M() * camera.V() * camera.P();

		// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
		MVPTransform.SetUniform(gpuProgram.getId(), "MVP");

		pTexture->SetUniform(gpuProgram.getId(), "textureUnit");

		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw two triangles forming a quad
	}
};

class CatmullRom {
	GLuint vao, vbo; // vertex array object, vertex buffer object
	std::vector<float>  vertexData; // interleaved data of coordinates

public:
	std::vector<controlPoint> controlPoints;
	void Create() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		glEnableVertexAttribArray(0);  // attribute array 0
		// Map attribute array 0 to the vertex data of the interleaved vbo
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), NULL); // attribute array, components/attribute, component type, normalize?, stride, offset
	}

	void AddControlPoint(float x, float y) {
		controlPoint point = controlPoint(vec4(x, y, 0, 1), vec2(0, 0));
		controlPoints.push_back(point);
		printf("Point added x:%3.2f, y:%3.2f\n", x, y);
	}

	void Draw() {
		if (controlPoints.size() > 1) {
			calculateSpeed();
			hermite();

			mat4 MVPTransform = camera.V() * camera.P();
			MVPTransform.SetUniform(gpuProgram.getId(), "MVP");

			vec3 Color = vec3(1, 0, 0);
			Color.SetUniform(gpuProgram.getId(), "color");

			glBindVertexArray(vao);

			// copy data to the GPU
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_DYNAMIC_DRAW);
			
			glDrawArrays(GL_LINE_STRIP, 0, vertexData.size() / 2);
		}
	}

private:
	void calculateSpeed() {

		//first point
		controlPoints[0].speed = vec2(0,0);
		//last point
		controlPoints[controlPoints.size() - 1].speed = vec2(0,0);

		//else
		for (int i = 1; i < controlPoints.size()-1; i++)
		{
			controlPoints[i].speed = vec2((0.5 * (controlPoints[i + 1].point.x - controlPoints[i - 1].point.x)), (0.5 * (controlPoints[i + 1].point.y - controlPoints[i - 1].point.y)));
		}
	}

	void hermite() {
		vertexData.clear();
		for (int i = 0; i < controlPoints.size(); i++) {
			if (i == controlPoints.size() - 1) return;

			float a0x = controlPoints[i].point.x;
			float a0y = controlPoints[i].point.y;

			float a1x = controlPoints[i].speed.x;
			float a1y = controlPoints[i].speed.y;

			float a2x = 3 * controlPoints[i + 1].point.x - 3 * controlPoints[i].point.x - 2 * controlPoints[i].speed.x - controlPoints[i + 1].speed.x;
			float a2y = 3 * controlPoints[i + 1].point.y - 3 * controlPoints[i].point.y - 2 * controlPoints[i].speed.y - controlPoints[i + 1].speed.y;

			float a3x = controlPoints[i].speed.x + controlPoints[i + 1].speed.x - 2 * controlPoints[i + 1].point.x + 2 * controlPoints[i].point.x;
			float a3y = controlPoints[i].speed.y + controlPoints[i + 1].speed.y - 2 * controlPoints[i + 1].point.y + 2 * controlPoints[i].point.y;

			for (float tau = 0; tau < 1; tau += 0.01) {
				float x = a3x * pow(tau, 3.0) + a2x * pow(tau, 2.0) + a1x * tau + a0x;
				float y = a3y * pow(tau, 3.0) + a2y * pow(tau, 2.0) + a1y * tau + a0y;

				vec4 wVertex = vec4(x, y, 0, 1) * camera.Pinv() * camera.Vinv();
				vertexData.push_back(wVertex.x);
				vertexData.push_back(wVertex.y);
			}
		}
	}
};

CatmullRom catmullrom; // CatMull-Rom Spline
Cart cart;
bool cartStart;

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	catmullrom.Create();
	cart.Create();

	cartStart = 0;

	// create program for the GPU
	gpuProgram.Create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	int location = glGetUniformLocation(gpuProgram.getId(), "separator");

	if (cartStart && catmullrom.controlPoints.size() >= 1) {
		if (location >= 0) glUniform1i(location, 1);
		else printf("uniform separator cannot be set\n");
		cart.Draw();
	}

	if (location >= 0) glUniform1i(location, 0);
	else printf("uniform separator cannot be set\n");
	catmullrom.Draw();
	

	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == ' ') {
		if (catmullrom.controlPoints.size() >= 1) {
			vec4 startPoint = vec4(catmullrom.controlPoints[0].point.x, catmullrom.controlPoints[0].point.y, 0, 1) * camera.Pinv() * camera.Vinv();
			cart.move(vec2(startPoint.x, startPoint.y));
		}
		if (catmullrom.controlPoints.size() >= 2) {
			cartStart = true;
		}
		glutPostRedisplay();
	}
	if (key == 'r')
	{
		cart.rotate(3.14 / 2);
		glutPostRedisplay();
	}
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		catmullrom.AddControlPoint(cX, cY);
		glutPostRedisplay();
	}

}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
	float sec = time / 1000.0f;
	//cart.Animate(sec);
	glutPostRedisplay();
}