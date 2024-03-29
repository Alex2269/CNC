#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include "fontprocessor.h"
#include <sys/stat.h>
#include FT_OUTLINE_H
#include FT_BBOX_H

FT_Library library = NULL;
FT_Face font_face = NULL;

int debug_file;
char mesg[100];

int generate(int num_code_symbol);

uint32_t lv_rand(uint32_t min, uint32_t max)
{
    static uint32_t a = 0x1234ABCD; /*Seed*/

    /*Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"*/
    uint32_t x = a;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    a = x;

    return (a % (max - min + 1)) + min;
}

// helper functions
void write_svg(char *line, int flag)
{
  if(flag)
  {
    write(debug_file,SVG_START_STRING,strlen(SVG_START_STRING));
  }
  else
  {
    write(debug_file,line,strlen(line));
    strcpy(mesg, ""); // clear contents of mesg
  }
}

void error_print(char *mesg, int error)
{
  printf("error code : %d error message : %s\n",error,mesg);
  exit(1);
}

// user struct for state info
typedef struct _user
{
  int state;
} User;

// outlinefunc callbacks
int MoveToFunction(const FT_Vector* to, void* user)
{
  FT_Pos x = to->x;
  FT_Pos y = to->y;

  sprintf(mesg, "M %ld %ld\n" , x, y);
  write_svg(mesg, 0);

  return 0;
}

int LineToFunction(const FT_Vector *to,void *user)
{
  FT_Pos x = to->x;
  FT_Pos y = to->y;
  sprintf(mesg, "L %ld %ld\n", x, y);
  write_svg(mesg,0);

  return 0;
}

int ConicToFunction(const FT_Vector *control, const FT_Vector *to, void *user)
{
  FT_Pos controlX = control->x;
  FT_Pos controlY = control->y;
  FT_Pos x = to->x;
  FT_Pos y = to->y;
  sprintf(mesg, "Q %ld %ld, %ld %ld\n", controlX, controlY, x, y);
  write_svg(mesg,0);

  return 0;
}

int CubicToFunction(const FT_Vector *controlOne, const FT_Vector *controlTwo, const FT_Vector *to, void *user)
{
  FT_Pos controlOneX = controlOne->x;
  FT_Pos controlOneY = controlOne->y;
  FT_Pos controlTwoX = controlTwo->x;
  FT_Pos controlTwoY = controlTwo->y;
  FT_Pos x = to->x;
  FT_Pos y = to->y;

  sprintf(mesg, "C %ld %ld %ld %ld %ld %ld\n",controlOneX, controlOneY, controlTwoX, controlTwoY, x, y);
  write_svg(mesg, 0);

  return 0;
}

char dest[100]=".svg";
char buffer[200];

int main(int argc,char *argv[])
{
  for(int i = 47;i < 258;i++) generate(i); // digital

  return 0;
}

int generate(int num_code_symbol)
{
  FT_GlyphSlot slot = NULL;
  FT_Outline outline;
  FT_Outline_Funcs outlinefuncs;
  FT_Matrix matrix;
  FT_BBox boundingBox;
  FT_Pos m_xMin;
  FT_Pos m_yMin;
  FT_Pos m_width;
  FT_Pos m_height;
  FT_Pos xMin;
  FT_Pos yMin;
  FT_Pos xMax;
  FT_Pos yMax;

  const FT_Fixed multiplier = 65536L;
  User user = {0};

  int error;
  // load library
  if((error = FT_Init_FreeType(&library))) error_print("library initialization\n",error);
  // load default font
  if((error = FT_New_Face(library, PATH_UNIFONT, 0, &font_face))) error_print("failed to load font\n",error);
  // glyph slot
  slot = font_face->glyph;
  // set pixel size, to overide zero initialization to all size object values
  if((error = FT_Set_Pixel_Sizes(font_face, 16, 0))) error_print("font_face size setting\n",error);
  // load glyph to glyph slot
  if((error = FT_Load_Char(font_face, num_code_symbol, FT_LOAD_DEFAULT)) != 0) error_print("glyph not found\n",error);

  // transform
  matrix.xx = 1L * multiplier;
  matrix.xy = 0L * multiplier;
  matrix.yx = 0L * multiplier;
  matrix.yy = -1L * multiplier;

  // outline manupulation
  outline = slot->outline;

  // other attributes necessary for parsing outline
  outlinefuncs.shift = 0;
  outlinefuncs.delta = 0;

  // use outlilne_decompse to get outline data : callbacks
  outlinefuncs.move_to = (FT_Outline_MoveToFunc) MoveToFunction;
  outlinefuncs.line_to = (FT_Outline_LineToFunc) LineToFunction;
  outlinefuncs.conic_to = (FT_Outline_ConicToFunc) ConicToFunction;
  outlinefuncs.cubic_to = (FT_Outline_CubicToFunc) CubicToFunction;

  FT_Outline_Transform(&outline, &matrix);
  FT_Outline_Get_BBox(&outline, &boundingBox);

  xMin = boundingBox.xMin;
  yMin = boundingBox.yMin;
  xMax = boundingBox.xMax;
  yMax = boundingBox.yMax;

  m_xMin = xMin;
  m_yMin = yMin;
  m_width = xMax - xMin;
  m_height = yMax - yMin;

  mkdir("dest_dir", S_IRWXU);

  sprintf(buffer, "./dest_dir/%d%s", num_code_symbol, dest);

  debug_file = open(buffer, O_CREAT | O_WRONLY | O_EXCL,0666);

  if(debug_file == -1)
  {
    // file already exists
    debug_file = open(buffer, O_WRONLY | O_TRUNC,0666);
  }

  write_svg("",1); // write starting lines
  sprintf(mesg, "viewBox = '%ld %ld %ld %ld'>\n <path d = \'\n", m_xMin,m_yMin,m_width,m_height);
  write_svg(mesg,0);
  FT_Outline_Decompose(&outline, &outlinefuncs, (void *) &user);

  write_svg("\n\'", 0);

  write_svg("\n fill=\'none\'", 0);
  write_svg("\n stroke=\'black\'", 0);
  write_svg("\n style=\'fill:#", 0);
  sprintf(buffer, "%x", lv_rand(0x8080ff, 0xffffff));
  write_svg(buffer, 0); // filling random color
  write_svg("\'", 0);
  write_svg("/>\n</svg>", 0);

  return 0;
}



