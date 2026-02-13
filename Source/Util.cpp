#include "../Header/Util.h"

#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <sstream>
#include <iostream>
#include <fstream>

#include <GL/glew.h>


#include "../Header/stb_image.h"
#include <vector>

// Autor: Nedeljko Tesanovic
// Opis: pomocne funkcije za zaustavljanje programa, ucitavanje sejdera, tekstura i kursora
// Smeju se koristiti tokom izrade projekta


int endProgram(std::string message) {
    std::cout << message << std::endl;
    glfwTerminate();
    return -1;
}


void updateViewport(int width, int height)
{
    glViewport(0, 0, width, height);
}

void createRectVAO(unsigned int& VAO, unsigned int& VBO)
{
    float verts[] = {
        -0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void createTexturedQuadVAO(unsigned int& VAO, unsigned int& VBO)
{
    float verts[] = {
        // x,    y,    u,   v
        -0.5f,  0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 0.0f,
         0.5f, -0.5f, 1.0f, 0.0f,
         0.5f,  0.5f, 1.0f, 1.0f

        
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);  
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
        (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}



unsigned int compileShader(GLenum type, const char* source)
{
    //Uzima kod u fajlu na putanji "source", kompajlira ga i vraca sejder tipa "type"
    //Citanje izvornog koda iz fajla
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str(); //Izvorni kod sejdera koji citamo iz fajla na putanji "source"

    int shader = glCreateShader(type); //Napravimo prazan sejder odredjenog tipa (vertex ili fragment)

    int success; //Da li je kompajliranje bilo uspjesno (1 - da)
    char infoLog[512]; //Poruka o gresci (Objasnjava sta je puklo unutar sejdera)
    glShaderSource(shader, 1, &sourceCode, NULL); //Postavi izvorni kod sejdera
    glCompileShader(shader); //Kompajliraj sejder

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success); //Provjeri da li je sejder uspjesno kompajliran
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog); //Pribavi poruku o gresci
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}

unsigned int createShader(const char* vsSource, const char* fsSource)
{
    //Pravi objedinjeni sejder program koji se sastoji od Vertex sejdera ciji je kod na putanji vsSource

    unsigned int program; //Objedinjeni sejder
    unsigned int vertexShader; //Verteks sejder (za prostorne podatke)
    unsigned int fragmentShader; //Fragment sejder (za boje, teksture itd)

    program = glCreateProgram(); //Napravi prazan objedinjeni sejder program

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource); //Napravi i kompajliraj vertex sejder
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource); //Napravi i kompajliraj fragment sejder

    //Zakaci verteks i fragment sejdere za objedinjeni program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program); //Povezi ih u jedan objedinjeni sejder program
    glValidateProgram(program); //Izvrsi provjeru novopecenog programa

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success); //Slicno kao za sejdere
    if (success == GL_FALSE)
    {
        //glGetShaderInfoLog(program, 512, NULL, infoLog);
        glGetProgramInfoLog(program, 512, NULL, infoLog);

        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    //Posto su kodovi sejdera u objedinjenom sejderu, oni pojedinacni programi nam ne trebaju, pa ih brisemo zarad ustede na memoriji
    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}

/*
unsigned loadImageToTexture(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (ImageData != NULL)
    {
        //Slike se osnovno ucitavaju naopako pa se moraju ispraviti da budu uspravne
        stbi__vertical_flip(ImageData, TextureWidth, TextureHeight, TextureChannels);

        // Provjerava koji je format boja ucitane slike
        GLint InternalFormat = -1;
        switch (TextureChannels) {
        case 1: InternalFormat = GL_RED; break;
        case 2: InternalFormat = GL_RG; break;
        case 3: InternalFormat = GL_RGB; break;
        case 4: InternalFormat = GL_RGBA; break;
        default: InternalFormat = GL_RGB; break;
        }

        unsigned int Texture;
        glGenTextures(1, &Texture);
        glBindTexture(GL_TEXTURE_2D, Texture);


        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


       glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, TextureWidth, TextureHeight, 0, InternalFormat, GL_UNSIGNED_BYTE, ImageData);
        glBindTexture(GL_TEXTURE_2D, 0);
        // oslobadjanje memorije zauzete sa stbi_load posto vise nije potrebna
        stbi_image_free(ImageData);
        return Texture;
    }
    else
    {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << std::endl;
        stbi_image_free(ImageData);
        return 0;
    }
}*/



unsigned loadImageToTexture(const char* filePath)
{
    // 1) ako je putanja prazna -> fallback 1x1
    if (!filePath || filePath[0] == '\0') {
        unsigned int tex = 0;
        unsigned char px[4] = { 180,180,180,255 };
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    // ✅ zvanični flip (umesto stbi__vertical_flip)
    stbi_set_flip_vertically_on_load(true);

    int w = 0, h = 0, ch = 0;
    unsigned char* data = stbi_load(filePath, &w, &h, &ch, 0);

    // 2) ako nije ucitano -> fallback 1x1 (NE PUCA)
    if (!data || w <= 0 || h <= 0) {
        std::cout << "[loadImageToTexture] FAILED: " << filePath
            << " reason=" << (data ? "bad size" : stbi_failure_reason())
            << "\n";

        unsigned int tex = 0;
        unsigned char px[4] = { 180,180,180,255 };
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (data) stbi_image_free(data);
        return tex;
    }

    GLenum format = GL_RGB;
    if (ch == 1) format = GL_RED;
    else if (ch == 2) format = GL_RG;
    else if (ch == 3) format = GL_RGB;
    else if (ch == 4) format = GL_RGBA;

    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}



/*
unsigned loadImageToTexture(const char* filePath) {
    stbi_set_flip_vertically_on_load(true); // ✅ zvanično

    int w = 0, h = 0, ch = 0;
    unsigned char* data = stbi_load(filePath, &w, &h, &ch, 0);

    if (!data) {
        std::cout << "Textura nije ucitana! Putanja texture: " << filePath << "\n";
        // stbi_image_free(nullptr) je ok, ali nije ni potrebno
        return 0;
    }

    GLenum format = GL_RGB;
    switch (ch) {
    case 1: format = GL_RED;  break;
    case 2: format = GL_RG;   break;
    case 3: format = GL_RGB;  break;
    case 4: format = GL_RGBA; break;
    default: format = GL_RGB; break;
    }

    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // ✅ wrap za tiling (ti koristiš tileFloor, tileWall...) -> treba REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // ✅ filter + mipmap (manje “mutno/treperi” u daljini)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}*/


/*unsigned loadImageToTexture(const char* filePath)
{
    if (!filePath || filePath[0] == '\0') {
        // fallback: 1x1 bela
        unsigned int tex = 0;
        unsigned char white[4] = { 255,255,255,255 };
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    int w = 0, h = 0, ch = 0;

    // ako koristiš flip u projektu:
    stbi_set_flip_vertically_on_load(true);

    unsigned char* data = stbi_load(filePath, &w, &h, &ch, 0);

    if (!data || w <= 0 || h <= 0) {
        std::cout << "[loadImageToTexture] FAILED: " << filePath
            << "  reason=" << (data ? "bad size" : stbi_failure_reason())
            << "\n";

        // fallback: 1x1 siva (da ne bude baš bela)
        unsigned int tex = 0;
        unsigned char gray[4] = { 180,180,180,255 };
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, gray);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    GLenum format = GL_RGB;
    if (ch == 1) format = GL_RED;
    else if (ch == 3) format = GL_RGB;
    else if (ch == 4) format = GL_RGBA;

    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // ✅ ovo je sad bezbedno jer data != nullptr
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}*/






GLFWcursor* loadImageToCursor(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;

    // ImageData - pointer na piksele slike
    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);
    if (!ImageData) {
        std::cout << "Kursor nije ucitan! Putanja kursora: " << filePath << std::endl;
        return nullptr;
    }

    // Maksimalna željena veličina kursora
    const int targetW = 60;
    const int targetH = 60;

    GLFWcursor* cursor = nullptr;

    if (TextureWidth > targetW || TextureHeight > targetH) {
        int outW = targetW;
        int outH = targetH;

        int channels = TextureChannels;
        if (channels == 0) channels = 4;  // fallback, čisto da ne pukne

        // alociramo buffer za smanjenu sliku
        unsigned char* resized = (unsigned char*)malloc(outW * outH * channels);
        if (!resized) {
            std::cout << "Neuspesna alokacija memorije za resized kursor!" << std::endl;
            stbi_image_free(ImageData);
            return nullptr;
        }

        // za svaki piksel u smanjenoj slici racunamo koji piksel originalne slike odgovara
        for (int y = 0; y < outH; ++y) {
            for (int x = 0; x < outW; ++x) {
                int srcX = x * TextureWidth / outW;
                int srcY = y * TextureHeight / outH;

                unsigned char* srcPixel = ImageData + (srcY * TextureWidth + srcX) * channels;
                unsigned char* dstPixel = resized + (y * outW + x) * channels;

                for (int c = 0; c < channels; ++c) {
                    dstPixel[c] = srcPixel[c];
                }
            }
        }

        GLFWimage image;
        image.width = outW;
        image.height = outH;
        image.pixels = resized;

        // centar kursora
        // Tacka na površini slike kursora koja se ponaša kao hitboks, moze se menjati po potrebi
        int hotspotX = outW / 5;
        int hotspotY = outH / 5;

        cursor = glfwCreateCursor(&image, hotspotX, hotspotY);

        free(resized);
        stbi_image_free(ImageData);
    }
    else {
        //ako slika nije prevelika koristimo je direktno
        GLFWimage image;
        image.width = TextureWidth;
        image.height = TextureHeight;
        image.pixels = ImageData;

        int hotspotX = TextureWidth / 5;
        int hotspotY = TextureHeight / 5;

        cursor = glfwCreateCursor(&image, hotspotX, hotspotY);

        stbi_image_free(ImageData);
    }

    return cursor;
}





void drawRect(unsigned int shader, unsigned int VAO,
    float cx, float cy, float sx, float sy, Color col)
{
    glUseProgram(shader);
    glUniform2f(glGetUniformLocation(shader, "uPos"), cx, cy);
    glUniform2f(glGetUniformLocation(shader, "uScale"), sx, sy);
    glUniform4f(glGetUniformLocation(shader, "uColor"), col.r, col.g, col.b, col.a);

    glBindVertexArray(VAO);    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); 
}



void drawTexturedQuad(unsigned int shader, unsigned int VAO,
    unsigned int texture,
    float cx, float cy, float sx, float sy)   
{
    glUseProgram(shader);
    glUniform2f(glGetUniformLocation(shader, "uPos"), cx, cy);
    glUniform2f(glGetUniformLocation(shader, "uScale"), sx, sy);

    glActiveTexture(GL_TEXTURE0);  
    glBindTexture(GL_TEXTURE_2D, texture); 

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);  
}

void createQuad3DVAO(unsigned int& vao, unsigned int& vbo) {
    float v[] = {
        // pos                // nor        // uv
        -0.5f, 0.0f, -0.5f,   0,1,0,        0,0,
         0.5f, 0.0f, -0.5f,   0,1,0,        1,0,
         0.5f, 0.0f,  0.5f,   0,1,0,        1,1,

         0.5f, 0.0f,  0.5f,   0,1,0,        1,1,
        -0.5f, 0.0f,  0.5f,   0,1,0,        0,1,
        -0.5f, 0.0f, -0.5f,   0,1,0,        0,0
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);

    int stride = (3 + 3 + 2) * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)((3 + 3) * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}


static std::string readTextFileSafe(const char* path)
{
    std::ifstream f(path, std::ios::in);
    if (!f) {
        std::cout << "NE MOGU OTVORITI: " << path << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}




static void printShaderLog(GLuint sh, const char* tag)
{
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len + 1);
        GLsizei written = 0;
        glGetShaderInfoLog(sh, len, &written, log.data());
        std::cout << "[SHADER COMPILE FAIL] " << tag << "\n" << log.data() << std::endl;

    }
}

static void printProgramLog(GLuint p)
{
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);

        std::vector<GLchar> log(len + 1);
        GLsizei written = 0;
        glGetProgramInfoLog(p, len, &written, log.data());

        std::cout << "[PROGRAM LINK FAIL]\n" << log.data() << std::endl;
    }
}




unsigned int createShaderSafe(const char* vsPath, const char* fsPath)
{
    std::string vsSrc = readTextFileSafe(vsPath);
    std::string fsSrc = readTextFileSafe(fsPath);
    if (vsSrc.empty() || fsSrc.empty()) return 0;

    const char* vsrc = vsSrc.c_str();
    const char* fsrc = fsSrc.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsrc, nullptr);
    glCompileShader(vs);
    printShaderLog(vs, vsPath);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsrc, nullptr);
    glCompileShader(fs);
    printShaderLog(fs, fsPath);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    printProgramLog(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

/*GLFWcursor* loadImageToCursor(const char* filePath) {
    int TextureWidth;
    int TextureHeight;
    int TextureChannels;

    unsigned char* ImageData = stbi_load(filePath, &TextureWidth, &TextureHeight, &TextureChannels, 0);

    if (ImageData != NULL)
    {
        GLFWimage image;
        image.width = TextureWidth;
        image.height = TextureHeight;
        image.pixels = ImageData;

        // Tacka na površini slike kursora koja se ponaša kao hitboks, moze se menjati po potrebi
        // Trenutno je gornji levi ugao, odnosno na 20% visine i 20% sirine slike kursora
        int hotspotX = TextureWidth / 5;
        int hotspotY = TextureHeight / 5;

        GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
        stbi_image_free(ImageData);
        return cursor;
    }
    else {
        std::cout << "Kursor nije ucitan! Putanja kursora: " << filePath << std::endl;
        stbi_image_free(ImageData);

    }
}*/



