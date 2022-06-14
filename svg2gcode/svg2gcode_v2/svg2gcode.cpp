// svg2gcode.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <string>
#include <stdint.h>
#include <cmath>

//#include <stdio.h>
//#include <string.h>
//#include <float.h>
//#include <GLFW/glfw3.h>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"



using namespace std;

struct point2d {
    float x;
    float y;
};

class dPoint {

public:
    float x;
    float y;
};

class dShape {

};

//возвращает строку (имя файла) содержащую только символы до "."
string filename2name(string str) {
    int tmp;
    string tmpStrDigit;
    tmp = str.find_first_of('.');
    str.resize(tmp);
    tmpStrDigit = str;

    return tmpStrDigit;
}
//ищет заданный символ в строке и читает цифровые символы и знак "." после него до первого
//нецифрового символа или до конца строки
//возвращает строку после "." (расширение фала)
string filename2type(string str) {
    int tmp;
    string tmpStrDigit;
    tmp = str.find_first_of('.');
    tmpStrDigit = str.substr(tmp + 1);
    return tmpStrDigit;
}

NSVGimage* g_image = NULL;

static unsigned char bgColor[4] = { 205,202,200,255 };
static unsigned char lineColor[4] = { 0,160,192,255 };

static float distPtSeg(float x, float y, float px, float py, float qx, float qy)
{
    float pqx, pqy, dx, dy, d, t;
    pqx = qx - px;
    pqy = qy - py;
    dx = x - px;
    dy = y - py;
    d = pqx * pqx + pqy * pqy;
    t = pqx * dx + pqy * dy;
    if (d > 0) t /= d;
    if (t < 0) t = 0;
    else if (t > 1) t = 1;
    dx = px + t * pqx - x;
    dy = py + t * pqy - y;
    return dx * dx + dy * dy;
}

static void  cubicBez(float x1, float y1, float x2, float y2,
                      float x3, float y3, float x4, float y4,
                      float tol, int level, string pathfile, vector<point2d>& v)
{
    float x12, y12, x23, y23, x34, y34, x123, y123, x234, y234, x1234, y1234;
    float d;
    point2d pt2d;
    //	vector <point2d> v;

    if (level > 12)  return;

    x12 = (x1 + x2) * 0.5f;
    y12 = (y1 + y2) * 0.5f;
    x23 = (x2 + x3) * 0.5f;
    y23 = (y2 + y3) * 0.5f;
    x34 = (x3 + x4) * 0.5f;
    y34 = (y3 + y4) * 0.5f;
    x123 = (x12 + x23) * 0.5f;
    y123 = (y12 + y23) * 0.5f;
    x234 = (x23 + x34) * 0.5f;
    y234 = (y23 + y34) * 0.5f;
    x1234 = (x123 + x234) * 0.5f;
    y1234 = (y123 + y234) * 0.5f;

    d = distPtSeg(x1234, y1234, x1, y1, x4, y4);
    if (d > tol * tol) {
        cubicBez(x1, y1, x12, y12, x123, y123, x1234, y1234, tol, level + 1, pathfile, v);
        cubicBez(x1234, y1234, x234, y234, x34, y34, x4, y4, tol, level + 1, pathfile, v);
    }
    else {
        pt2d.x = trunc(x4 * 100) / 100;
        pt2d.y = -trunc(y4 * 100) / 100;
        v.push_back(pt2d);
    }
}

void buildGcode(string pathfile,
                uint32_t strokeColor,
                uint32_t colorCut,
                uint32_t colorGrav,
                int CurrentPower,
                int powerCut,
                int powerGrav,
                int CurrentSpeed,
                int speedCut,
                int speedGrav)
{

    NSVGshape* shape;
    NSVGpath* path;

    float* pts;
    int npts;
    char closed;
    float tol;

    int i;

   

    for (shape = g_image->shapes; shape != NULL; shape = shape->next) {

        NSVGpaint stk = shape->stroke;
        strokeColor = stk.color;
       

        if (strokeColor == colorCut) {
            CurrentPower = powerCut;
            CurrentSpeed = speedCut;
        }
        else {
            CurrentPower = powerGrav;
            CurrentSpeed = speedGrav;
        }

        for (path = shape->paths; path != NULL; path = path->next) {
            //	drawPath(path->pts, path->npts, path->closed, 0.0625);

            pts = path->pts;
            npts = path->npts;
            closed = path->closed;
            tol = 0.0625;
            point2d pt2d;
            vector <point2d> v1;

            pt2d.x = trunc(pts[0] * 100) / 100;
            pt2d.y = -trunc(pts[1] * 100) / 100;
            v1.push_back(pt2d);

            /*
                        ofstream fileo;
                        fileo.open(pathfile,ios::app);

                        fileo<<"M5"<<endl;
                        fileo<<"G0"<<"X"<<pts[0]<<"Y"<<pts[1]<<endl;

                        fileo<<"M4"<<" "<<"S"<<CurrentPower<<endl;
                        fileo<<"G1"<<"F"<<CurrentSpeed;
                        fileo.close();
            */
            for (i = 0; i < npts - 1; i += 3) {
                float* p = &pts[i * 2];
                cubicBez(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], tol, 0, pathfile, v1);
            }

            if (closed) {
                //       ofstream fileo;
                //       fileo.open(pathfile,ios::app);
                //       fileo<<"M5"<<endl;
                //       fileo.close();
            }
       

            ofstream fileo;
            fileo.open(pathfile, ios::app);

            fileo << "M5" << endl;
            fileo << "G0" << "X" << v1.back().x << "Y" << v1.back().y << endl;
            fileo << "M4" << " " << "S" << CurrentPower << endl;
            fileo << "G1" << "X" << v1.back().x << "Y" << v1.back().y << "F" << CurrentSpeed << endl;
            v1.pop_back();
            for (i = v1.size() - 1; i >= 0; i--) {
                fileo << "X" << v1[i].x << "Y" << v1[i].y << endl;
            }
            fileo << "M5" << endl;
        }

    }

}


int main(int size, char* str[])
{
    //size- количество аргументов 
    //char *str[] - строковый массив с аргументами

    setlocale(LC_ALL, "rus");//Поддержка русского языка в программе
    setlocale(LC_NUMERIC, "C");//Установка точки вместо запятой при выводе дробных чисел

    //Настройки для лазера
    uint32_t strokeColor = 0x00000001;
    uint32_t colorCut = 0xFF000000;
    uint32_t colorGrav = 0xFF0000FF;
    int CurrentPower=500;
    int powerCut = 650;
    int powerGrav = 110;
    int CurrentSpeed=2000;
    int speedCut = 1000;
    int speedGrav = 10000;
    string pathfile;

    cout << "Конвертер макетов для лазера в формате векторной графики SVG" <<  endl;
    cout << "в формат G-Code для LaserGRBL и других работающих на ARDUINO" << endl;
    cout << "v 1.0 10.02.2021" << endl;
 
    if (size == 1) {
        printf(" Не указан файл для конвертирования \n");
        return -1;
    }
    string nameInputFile;
    if (size == 2) {
        nameInputFile = str[1];
        string name, type;
        name = filename2name(nameInputFile);
        type = filename2type(nameInputFile);
        pathfile = name + ".nc";
        cout << "Input file (SVG):" << nameInputFile << endl;
        cout << "Output file (G-Code):" << pathfile << endl;
    }

    g_image = nsvgParseFromFile(nameInputFile.c_str(), "mm", 96.0f);
    if (g_image == NULL) {
        printf("Could not open SVG image.\n");
        return -1;
    }
  
    ofstream fileo;
   
    fileo.open(pathfile,ios::out);
    
    if (!fileo)
    {
        cout << "Ошибка! Файл не создан\n\n";
        return -1;
    }
   
    fileo.close();

    buildGcode(pathfile, strokeColor, colorCut,  colorGrav, CurrentPower, powerCut, powerGrav, CurrentSpeed, speedCut, speedGrav);

    nsvgDelete(g_image);
        
 //   system("pause");
    return 0;
}
