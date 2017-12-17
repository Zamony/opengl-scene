//internal includes
#include "common.h"
#include "ShaderProgram.h"
#include "Camera.h"

//External dependencies
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <random>

#include "SOIL/SOIL.h"
#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

static const GLsizei WIDTH = 512, HEIGHT = 512; //размеры окна
static int filling = 0;
int normals = 0;
static bool keys[1024]; //массив состояний кнопок - нажата/не нажата
static GLfloat lastX = 400, lastY = 300; //исходное положение мыши
static bool firstMouse = true;
static bool g_captureMouse         = true;  // Мышка захвачена нашим приложением или нет?
static bool g_capturedMouseJustNow = false;

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

Camera camera(float3(0.0f, 10.0f, 0.0f));
GLuint texture1;

float shift1 = 0;

class DSA {
private:
    std::vector<float> values;
    
    void setSample(int x, int y, float value)
    {
        values[(x % (size - 1)) + (y % (size - 1)) * size] = value;
    }

    float frand(){
        return (rand()/(float)(RAND_MAX)) * 2 - 1;
    }

    void sampleSquare(int x, int y, int size, float value)
    {
        int hs = size / 2;
    
        float a = sample(x - hs, y - hs);
        float b = sample(x + hs, y - hs);
        float c = sample(x - hs, y + hs);
        float d = sample(x + hs, y + hs);
    
        setSample(x, y, ((a + b + c + d) / 4.0) + value);
    
    }
    
    void sampleDiamond(int x, int y, int size, float value)
    {
        int hs = size / 2;
    
        float a = sample(x - hs, y);
        float b = sample(x + hs, y);
        float c = sample(x, y - hs);
        float d = sample(x, y + hs);
    
        setSample(x, y, ((a + b + c + d) / 4.0) + value);
    }

    void DiamondSquare(int stepsize, float scale)
    {
    
        int halfstep = stepsize / 2;
    
        for (int y = halfstep; y < size + halfstep; y += stepsize)
        {
            for (int x = halfstep; x < size + halfstep; x += stepsize)
            {
                sampleSquare(x, y, stepsize, frand() * scale);
            }
        }
    
        for (int y = 0; y < size; y += stepsize)
        {
            for (int x = 0; x < size; x += stepsize)
            {
                sampleDiamond(x + halfstep, y, stepsize, frand() * scale);
                sampleDiamond(x, y + halfstep, stepsize, frand() * scale);
            }
        }
    
    }
public:
    const int size;

    DSA(int _size):size(_size){
        values.reserve(_size * _size);
    }

    void generateTerrain(){
        srand(0);

        const int featuresize = size;
        for( int y = 0; y < size; y += featuresize)
            for (int x = 0; x < size; x += featuresize)
                setSample(x, y, frand());

        int samplesize = size;
        float scale = 1.0;
        while (samplesize > 1)
        {
        
            DiamondSquare(samplesize, scale);
        
            samplesize /= 2;
            scale /= 2.0;
        }
    }

    float sample(int x, int y){
        return values[(x % (size - 1)) + (y % (size - 1)) * size];
    }

    void normalize(){
      float maxp = 0;
      for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
          float val = abs(sample(i, j));
          maxp = std::max( val, maxp );
        }
      }

      for (int i = 0; i < size; i++){
        for (int j = 0; j < size; j++){
          float val = sample(i, j) / maxp;
          setSample(i, j, val*val);
        }
      }
    }

};

//функция для обработки нажатий на кнопки клавиатуры
void OnKeyboardPressed(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	//std::cout << key << std::endl;
	switch (key)
	{
	case GLFW_KEY_ESCAPE: //на Esc выходим из программы
		if (action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GL_TRUE);
		break;
	case GLFW_KEY_SPACE: //на пробел переключение в каркасный режим и обратно
		if (action == GLFW_PRESS)
		{
			if (filling == 0)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				filling = 1;
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				filling = 0;
			}
		}
		break;
  case GLFW_KEY_1:
    normals = 0;
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    break;
  case GLFW_KEY_2:
    normals = 1;
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    break;
	default:
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

//функция для обработки клавиш мыши
void OnMouseButtonClicked(GLFWwindow* window, int button, int action, int mods)
{
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    g_captureMouse = !g_captureMouse;


  if (g_captureMouse)
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    g_capturedMouseJustNow = true;
  }
  else
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

}

//функция для обработки перемещения мыши
void OnMouseMove(GLFWwindow* window, double xpos, double ypos)
{
  if (firstMouse)
  {
    lastX = float(xpos);
    lastY = float(ypos);
    firstMouse = false;
  }

  GLfloat xoffset = float(xpos) - lastX;
  GLfloat yoffset = lastY - float(ypos);  

  lastX = float(xpos);
  lastY = float(ypos);

  if (g_captureMouse)
    camera.ProcessMouseMove(xoffset, yoffset);
}


void OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
  camera.ProcessMouseScroll(GLfloat(yoffset));
}

void doCameraMovement(Camera &camera, GLfloat deltaTime)
{

  if ( camera.pos.z > 360/(2*6.0f) ){
    camera.pos.z = camera.pos.z - 360/6.0f;
    deltaTime += 0.06;
  }

  if ( camera.pos.x > 360/(2*6.0f) ){
    camera.pos.x = camera.pos.x - 360/6.0f;
    deltaTime += 0.06;
  }

  if ( camera.pos.z < -360/(2*6.0f) ){
    camera.pos.z = camera.pos.z + 360/6.0f;
    deltaTime += 0.06;
  }

  if ( camera.pos.x < -360/(2*6.0f) ){
    camera.pos.x = camera.pos.x + 360/6.0f;
    deltaTime += 0.06;
  }

  if (keys[GLFW_KEY_W])
    camera.ProcessKeyboard(FORWARD, deltaTime);
  if (keys[GLFW_KEY_A])
    camera.ProcessKeyboard(LEFT, deltaTime);
  if (keys[GLFW_KEY_S])
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  if (keys[GLFW_KEY_D])
    camera.ProcessKeyboard(RIGHT, deltaTime);

  //std::cout << camera.pos.x << " " << camera.pos.y << " " << camera.pos.z << "\n";
}


/**
\brief создать triangle strip плоскость и загрузить её в шейдерную программу
\param rows - число строк
\param cols - число столбцов
\param size - размер плоскости
\param vao - vertex array object, связанный с созданной плоскостью
*/
static int createTriStrip(int rows, int cols, float size, GLuint &vao)
{

  int numIndices = 2 * cols*(rows - 1) + rows - 1;

  std::vector<GLfloat> vertices_vec; //вектор атрибута координат вершин
  vertices_vec.reserve(rows * cols * 3);

  std::vector<GLfloat> normals_vec; //вектор атрибута нормалей к вершинам
  normals_vec.reserve(rows * cols * 3);

  std::vector<GLfloat> texcoords_vec; //вектор атрибут текстурных координат вершин
  texcoords_vec.reserve(rows * cols * 2);

  std::vector<float3> normals_vec_tmp(rows * cols, float3(0.0f, 0.0f, 0.0f)); //временный вектор нормалей, используемый для расчетов

  std::vector<int3> faces;         //вектор граней (треугольников), каждая грань - три индекса вершин, её составляющих; используется для удобства расчета нормалей
  faces.reserve(numIndices / 3);

  std::vector<GLuint> indices_vec; //вектор индексов вершин для передачи шейдерной программе
  indices_vec.reserve(numIndices);

  DSA dsa( 129 );
  dsa.generateTerrain();

  for (int z = 0; z < rows; ++z)
  {
    for (int x = 0; x < cols; ++x)
    {
      //вычисляем координаты каждой из вершин 
      float xx = -size / 2 + x*size / cols;
      float zz = -size / 2 + z*size / rows;
      // float yy = -1.0f;
      //float r = sqrt(xx*xx + zz*zz);

      float yy = 4.8f * dsa.sample(x, z);//5.0f * (r != 0.0f ? sin(r) / r : 1.0f);
      if ( z == 0 && x == 0 ){
        shift1 = yy;  
      }

      vertices_vec.push_back(xx);
      vertices_vec.push_back(yy);
      vertices_vec.push_back(zz);

      texcoords_vec.push_back(x / float(cols - 1) * cols); // вычисляем первую текстурную координату u, для плоскости это просто относительное положение вершины
      texcoords_vec.push_back(z / float(rows - 1) * rows); // аналогично вычисляем вторую текстурную координату v
    }
  }

  // for (int z = 0; z < rows; ++z)
  // {
  //   for (int x = 0; x < cols; ++x)
  //   {
  //     //вычисляем координаты каждой из вершин 
  //     float xx = -size + -size/2 + x*size / cols;
  //     float zz = -size + -size / 2 + z*size / rows;
  //     // float yy = -1.0f;
  //     //float r = sqrt(xx*xx + zz*zz);
  //     float yy = 4.8f * dsa.sample(x, z);//5.0f * (r != 0.0f ? sin(r) / r : 1.0f);

  //     vertices_vec.push_back(xx);
  //     vertices_vec.push_back(yy);
  //     vertices_vec.push_back(zz);

  //     texcoords_vec.push_back(x / float(cols - 1) * cols); // вычисляем первую текстурную координату u, для плоскости это просто относительное положение вершины
  //     texcoords_vec.push_back(z / float(rows - 1) * rows); // аналогично вычисляем вторую текстурную координату v
  //   }
  // }

  //primitive restart - специальный индекс, который обозначает конец строки из треугольников в triangle_strip
  //после этого индекса формирование треугольников из массива индексов начнется заново - будут взяты следующие 3 индекса для первого треугольника
  //и далее каждый последующий индекс будет добавлять один новый треугольник пока снова не встретится primitive restart index

  int primRestart = cols * rows;

  for (int x = 0; x < cols - 1; ++x)
  {
    for (int z = 0; z < rows - 1; ++z)
    {
      int offset = x*cols + z;

      //каждую итерацию добавляем по два треугольника, которые вместе формируют четырехугольник
      if (z == 0) //если мы в начале строки треугольников, нам нужны первые четыре индекса
      {
        indices_vec.push_back(offset + 0);
        indices_vec.push_back(offset + rows);
        indices_vec.push_back(offset + 1);
        indices_vec.push_back(offset + rows + 1);
      }
      else // иначе нам достаточно двух индексов, чтобы добавить два треугольника
      {
        indices_vec.push_back(offset + 1);
        indices_vec.push_back(offset + rows + 1);

        if (z == rows - 2) indices_vec.push_back(primRestart); // если мы дошли до конца строки, вставляем primRestart, чтобы обозначить переход на следующую строку
      }
    }
  }

  ///////////////////////
  //формируем вектор граней(треугольников) по 3 индекса на каждый
  int currFace = 1;
  for (int i = 0; i < indices_vec.size() - 2; ++i)
  {
    int3 face;

    int index0 = indices_vec.at(i);
    int index1 = indices_vec.at(i + 1);
    int index2 = indices_vec.at(i + 2);

    if (index0 != primRestart && index1 != primRestart && index2 != primRestart)
    {
      if (currFace % 2 != 0) //если это нечетный треугольник, то индексы и так в правильном порядке обхода - против часовой стрелки
      {
        face.x = indices_vec.at(i);
        face.y = indices_vec.at(i + 1);
        face.z = indices_vec.at(i + 2);

        currFace++;
      }
      else //если треугольник четный, то нужно поменять местами 2-й и 3-й индекс;
      {    //при отрисовке opengl делает это за нас, но при расчете нормалей нам нужно это сделать самостоятельно
        face.x = indices_vec.at(i);
        face.y = indices_vec.at(i + 2);
        face.z = indices_vec.at(i + 1);

        currFace++;
      }
      faces.push_back(face);
    }
  }


  ///////////////////////
  //расчет нормалей
  for (int i = 0; i < faces.size(); ++i)
  {
    //получаем из вектора вершин координаты каждой из вершин одного треугольника
    float3 A(vertices_vec.at(3 * faces.at(i).x + 0), vertices_vec.at(3 * faces.at(i).x + 1), vertices_vec.at(3 * faces.at(i).x + 2));
    float3 B(vertices_vec.at(3 * faces.at(i).y + 0), vertices_vec.at(3 * faces.at(i).y + 1), vertices_vec.at(3 * faces.at(i).y + 2));
    float3 C(vertices_vec.at(3 * faces.at(i).z + 0), vertices_vec.at(3 * faces.at(i).z + 1), vertices_vec.at(3 * faces.at(i).z + 2));

    //получаем векторы для ребер треугольника из каждой из 3-х вершин
    float3 edge1A(normalize(B - A));
    float3 edge2A(normalize(C - A));

    float3 edge1B(normalize(A - B));
    float3 edge2B(normalize(C - B));

    float3 edge1C(normalize(A - C));
    float3 edge2C(normalize(B - C));

    //нормаль к треугольнику - векторное произведение любой пары векторов из одной вершины
    float3 face_normal = cross(edge1A, edge2A);

    //простой подход: нормаль к вершине = средняя по треугольникам, к которым принадлежит вершина
    normals_vec_tmp.at(faces.at(i).x) += face_normal;
    normals_vec_tmp.at(faces.at(i).y) += face_normal;
    normals_vec_tmp.at(faces.at(i).z) += face_normal;
  }

  //нормализуем векторы нормалей и записываем их в вектор из GLFloat, который будет передан в шейдерную программу
  for (int i = 0; i < normals_vec_tmp.size(); ++i)
  {
    float3 N = normalize(normals_vec_tmp.at(i));

    normals_vec.push_back(N.x);
    normals_vec.push_back(N.y);
    normals_vec.push_back(N.z);
  }

  // ====================
  // Texture 1
  // ====================
  glGenTextures(1, &texture1);
  glBindTexture(GL_TEXTURE_2D, texture1); // All upcoming GL_TEXTURE_2D operations now have effect on our texture object
  // Set our texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Set texture filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Load, create texture and generate mipmaps
  int width, height;
  unsigned char* image = SOIL_load_image("grass.jpg", &width, &height, 0, SOIL_LOAD_RGB);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  SOIL_free_image_data(image);
  glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.


  GLuint vboVertices, vboIndices, vboNormals, vboTexCoords;

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vboVertices);
  glGenBuffers(1, &vboIndices);
  glGenBuffers(1, &vboNormals);
  glGenBuffers(1, &vboTexCoords);


  glBindVertexArray(vao); GL_CHECK_ERRORS;
  {

    //передаем в шейдерную программу атрибут координат вершин
    glBindBuffer(GL_ARRAY_BUFFER, vboVertices); GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, vertices_vec.size() * sizeof(GL_FLOAT), &vertices_vec[0], GL_STATIC_DRAW); GL_CHECK_ERRORS;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), (GLvoid*)0); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(0); GL_CHECK_ERRORS;

    //передаем в шейдерную программу атрибут нормалей
    glBindBuffer(GL_ARRAY_BUFFER, vboNormals); GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, normals_vec.size() * sizeof(GL_FLOAT), &normals_vec[0], GL_STATIC_DRAW); GL_CHECK_ERRORS;
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT), (GLvoid*)0); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(1); GL_CHECK_ERRORS;

    //передаем в шейдерную программу атрибут текстурных координат
    glBindBuffer(GL_ARRAY_BUFFER, vboTexCoords); GL_CHECK_ERRORS;
    glBufferData(GL_ARRAY_BUFFER, texcoords_vec.size() * sizeof(GL_FLOAT), &texcoords_vec[0], GL_STATIC_DRAW); GL_CHECK_ERRORS;
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GL_FLOAT), (GLvoid*)0); GL_CHECK_ERRORS;
    glEnableVertexAttribArray(2); GL_CHECK_ERRORS;

    //передаем в шейдерную программу индексы
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndices); GL_CHECK_ERRORS;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_vec.size() * sizeof(GLuint), &indices_vec[0], GL_STATIC_DRAW); GL_CHECK_ERRORS;

    glEnable(GL_PRIMITIVE_RESTART); GL_CHECK_ERRORS;
    glPrimitiveRestartIndex(primRestart); GL_CHECK_ERRORS;
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);


  return numIndices;
}


int initGL()
{
	int res = 0;

	//грузим функции opengl через glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	//выводим в консоль некоторую информацию о драйвере и контексте opengl
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

  std::cout << "Controls: "<< std::endl;
  std::cout << "press left mose button to capture/release mouse cursor  "<< std::endl;
  std::cout << "press spacebar to alternate between shaded wireframe and fill display modes" << std::endl;
  std::cout << "press ESC to exit" << std::endl;

	return 0;
}

int main(int argc, char** argv)
{
	if(!glfwInit())
    return -1;

	//запрашиваем контекст opengl версии 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); 


  GLFWwindow*  window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL basic sample", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	
	glfwMakeContextCurrent(window); 

	//регистрируем коллбеки для обработки сообщений от пользователя - клавиатура, мышь..
	glfwSetKeyCallback        (window, OnKeyboardPressed);  
	glfwSetCursorPosCallback  (window, OnMouseMove); 
  glfwSetMouseButtonCallback(window, OnMouseButtonClicked);
	glfwSetScrollCallback     (window, OnMouseScroll);
	glfwSetInputMode          (window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

	if(initGL() != 0) 
		return -1;
	
  //Reset any OpenGL errors which could be present for some reason
	GLenum gl_error = glGetError();
	while (gl_error != GL_NO_ERROR)
		gl_error = glGetError();

	//создание шейдерной программы из двух файлов с исходниками шейдеров
	//используется класс-обертка ShaderProgram
	std::unordered_map<GLenum, std::string> shaders;
	shaders[GL_VERTEX_SHADER]   = "vertex.glsl";
	shaders[GL_FRAGMENT_SHADER] = "fragment.glsl";
	ShaderProgram program(shaders); GL_CHECK_ERRORS;

  
  //Создаем и загружаем геометрию поверхности
  GLuint vaoTriStrip;
  int triStripIndices = createTriStrip(6*129, 6*129, 360, vaoTriStrip);


  glViewport(0, 0, WIDTH, HEIGHT);  GL_CHECK_ERRORS;
  glEnable(GL_DEPTH_TEST);  GL_CHECK_ERRORS;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  std::vector<GLfloat> myvertices;
  {
    std::string inputfile = "tree_bad.obj";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
      
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());
      
    if (!err.empty()) { // `err` may contain warning message.
      std::cerr << err << std::endl;
    }

    if (!ret) {
      exit(1);
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
      // Loop over faces(polygon)
      size_t index_offset = 0;
      for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
        int fv = shapes[s].mesh.num_face_vertices[f];

        // Loop over vertices in the face.
        for (size_t v = 0; v < fv; v++) {
          // access to vertex
          tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
          tinyobj::real_t vx = attrib.vertices[3*idx.vertex_index+0];
          tinyobj::real_t vy = attrib.vertices[3*idx.vertex_index+1];
          tinyobj::real_t vz = attrib.vertices[3*idx.vertex_index+2];
          myvertices.push_back(vx);
          myvertices.push_back(vy+shift1);
          myvertices.push_back(vz);
          // tinyobj::real_t nx = attrib.normals[3*idx.normal_index+0];
          // tinyobj::real_t ny = attrib.normals[3*idx.normal_index+1];
          // tinyobj::real_t nz = attrib.normals[3*idx.normal_index+2];
          // tinyobj::real_t tx = attrib.texcoords[2*idx.texcoord_index+0];
          // tinyobj::real_t ty = attrib.texcoords[2*idx.texcoord_index+1];
          // Optional: vertex colors
          // tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
          // tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
          // tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
        }
        index_offset += fv;

        // per-face material
        shapes[s].mesh.material_ids[f];
      }
    }
  }

  GLuint treevbo;
  glGenBuffers(1, &treevbo);
  glBindBuffer(GL_ARRAY_BUFFER, treevbo);
  glBufferData(GL_ARRAY_BUFFER, myvertices.size() * sizeof(GLfloat), &myvertices[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  GLuint treevao;
  glGenVertexArrays(1, &treevao);
  glBindVertexArray(treevao);
  glBindBuffer(GL_ARRAY_BUFFER, treevbo);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);


	//цикл обработки сообщений и отрисовки сцены каждый кадр
	while (!glfwWindowShouldClose(window))
	{
		//считаем сколько времени прошло за кадр
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
    doCameraMovement(camera, deltaTime);

		//очищаем экран каждый кадр
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f); GL_CHECK_ERRORS;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GL_CHECK_ERRORS;

    program.StartUseShader(); GL_CHECK_ERRORS;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glUniform1i(glGetUniformLocation(program.GetProgram(), "ourTexture1"), 0);

		//обновляем матрицы камеры и проекции каждый кадр
    float4x4 view       = camera.GetViewMatrix();
    float4x4 projection = projectionMatrixTransposed(camera.zoom, float(WIDTH) / float(HEIGHT), 0.1f, 1000.0f);

		                //модельная матрица, определяющая положение объекта в мировом пространстве
		float4x4 model; //начинаем с единичной матрицы
		
    program.StartUseShader();

    //загружаем uniform-переменные в шейдерную программу (одинаковые для всех параллельно запускаемых копий шейдера)
    program.SetUniform("view",       view);       GL_CHECK_ERRORS;
    program.SetUniform("projection", projection); GL_CHECK_ERRORS;
    program.SetUniform("model",      model);
    program.SetUniform("draw_normals", normals);
    program.SetUniform("is_grass", 0);

    //рисуем плоскость
    glBindVertexArray(vaoTriStrip);
    glDrawElements(GL_TRIANGLE_STRIP, triStripIndices, GL_UNSIGNED_INT, nullptr); GL_CHECK_ERRORS;
    glBindVertexArray(0); GL_CHECK_ERRORS;

    program.SetUniform("is_grass", 1);

    glBindVertexArray(treevao);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, myvertices.size() / 3);
    glDisableVertexAttribArray(0); GL_CHECK_ERRORS;

    program.StopUseShader();

		glfwSwapBuffers(window); 
	}

	//очищаем vao перед закрытием программы
	glDeleteVertexArrays(1, &vaoTriStrip);

	glfwTerminate();
	return 0;
}
