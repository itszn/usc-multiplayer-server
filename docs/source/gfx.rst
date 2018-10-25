Drawing
========
This chapter describes all the functions available under the `gfx`
table.

Example usage

.. code-block:: lua

    --Draw score
    gfx.BeginPath()
    gfx.LoadSkinFont("NovaMono.ttf")
    gfx.Translate(-5,5) -- upper right margin
    gfx.FillColor(255,255,255)
    gfx.TextAlign(gfx.TEXT_ALIGN_RIGHT + gfx.TEXT_ALIGN_TOP)
    gfx.FontSize(40)
    gfx.Text(string.format("%08d", score),desw,0)

    
Constants
*************************************
Most constants are from the nanovg library.

+------------------------+-------------------------------------------+
|    Lua Name            |         Value                             |
+========================+===========================================+
| TEXT_ALIGN_BASELINE    | NVGalign::NVG_ALIGN_BASELINE              |
+------------------------+-------------------------------------------+
| TEXT_ALIGN_BOTTOM      | NVGalign::NVG_ALIGN_BOTTOM                |
+------------------------+-------------------------------------------+
| TEXT_ALIGN_CENTER      | NVGalign::NVG_ALIGN_CENTER                |
+------------------------+-------------------------------------------+
| TEXT_ALIGN_LEFT        | NVGalign::NVG_ALIGN_LEFT                  |
+------------------------+-------------------------------------------+
| TEXT_ALIGN_MIDDLE      | NVGalign::NVG_ALIGN_MIDDLE                |
+------------------------+-------------------------------------------+
| TEXT_ALIGN_RIGHT       | NVGalign::NVG_ALIGN_RIGHT                 |
+------------------------+-------------------------------------------+
| TEXT_ALIGN_TOP         | NVGalign::NVG_ALIGN_TOP                   |
+------------------------+-------------------------------------------+
| LINE_BEVEL             | NVGlineCap::NVG_BEVEL                     |
+------------------------+-------------------------------------------+
| LINE_BUTT              | NVGlineCap::NVG_BUTT                      |
+------------------------+-------------------------------------------+
| LINE_MITER             | NVGlineCap::NVG_MITER                     |
+------------------------+-------------------------------------------+
| LINE_ROUND             | NVGlineCap::NVG_ROUND                     |
+------------------------+-------------------------------------------+
| LINE_SQUARE            | NVGlineCap::NVG_SQUARE                    |
+------------------------+-------------------------------------------+
| IMAGE_GENERATE_MIPMAPS | NVGimageFlags::NVG_IMAGE_GENERATE_MIPMAPS |
+------------------------+-------------------------------------------+
| IMAGE_REPEATX          | NVGimageFlags::NVG_IMAGE_REPEATX          |
+------------------------+-------------------------------------------+
| IMAGE_REPEATY          | NVGimageFlags::NVG_IMAGE_REPEATY          |
+------------------------+-------------------------------------------+
| IMAGE_FLIPY            | NVGimageFlags::NVG_IMAGE_FLIPY            |
+------------------------+-------------------------------------------+
| IMAGE_PREMULTIPLIED    | NVGimageFlags::NVG_IMAGE_PREMULTIPLIED    |
+------------------------+-------------------------------------------+
| IMAGE_NEAREST          | NVGimageFlags::NVG_IMAGE_NEAREST          |
+------------------------+-------------------------------------------+


BeginPath()
******************************************************

Rect(float x, float y, float w, float h)
******************************************************

FastRect(float x, float y, float w, float h)
******************************************************

Fill()
******************************************************

FillColor(int r, int g, int b, int a = 255)
******************************************************

CreateImage(const char* filename, int imageflags)
******************************************************

CreateSkinImage(const char* filename, int imageflags)
******************************************************

ImagePatternFill(int image, float alpha)
************************************************************

ImageRect(float x, float y, float w, float h, int image, float alpha, float angle)
**********************************************************************************

Text(const char* s, float x, float y)
******************************************************

TextAlign(int align)
******************************************************

FontFace(const char* s)
******************************************************

FontSize(float size)
******************************************************

Translate(float x, float y)
******************************************************

Scale(float x, float y)
******************************************************

Rotate(float angle)
******************************************************

ResetTransform()
******************************************************

LoadFont(const char* name, const char* filename)
******************************************************

LoadSkinFont(const char* name, const char* filename)
******************************************************

FastText(const char* inputText, float x, float y)
**********************************************************************

CreateLabel(const char* text, int size, bool monospace)
*******************************************************

DrawLabel(int labelId, float x, float y, float maxWidth = -1)
**************************************************************
Renders an already created label.

Will resize the label to fit within the maxWidth if maxWidth > 0.

MoveTo(float x, float y)
******************************************************

LineTo(float x, float y)
******************************************************

BezierTo(float c1x, float c1y, float c2x, float c2y, float x, float y)
**********************************************************************

QuadTo(float cx, float cy, float x, float y)
******************************************************

ArcTo(float x1, float y1, float x2, float y2, float radius)
***********************************************************

ClosePath()
******************************************************

MiterLimit(float limit)
******************************************************

StrokeWidth(float size)
******************************************************

LineCap(int cap)
******************************************************

LineJoin(int join)
******************************************************

Stroke()
******************************************************

StrokeColor(int r, int g, int b, int a = 255)
******************************************************

UpdateLabel(int labelId, const char* text, int size)
******************************************************

DrawGauge(float rate, float x, float y, float w, float h, float deltaTime)
**************************************************************************

RoundedRect(float x, float y, float w, float h, float r)
**************************************************************************

RoundedRectVarying(float x, float y, float w, float h, float radTopLeft, float radTopRight, float radBottomRight, float radBottomLeft)
**************************************************************************************************************************************

Ellipse(float cx, float cy, float rx, float ry)
**************************************************************************

Circle(float cx, float cy, float r)
**************************************************************************

SkewX(float angle)
**************************************************************************

SkewY(float angle)
**************************************************************************

LinearGradient(float sx, float sy, float ex, float ey)
**************************************************************************

BoxGradient(float x, float y, float w, float h, float r, float f)
**************************************************************************

RadialGradient(float cx, float cy, float inr, float outr)
**************************************************************************

ImagePattern(float ox, float oy, float ex, float ey, float angle, int image, float alpha)
******************************************************************************************

GradientColors(int ri, int gi, int bi, int ai, int ro, int go, int bo, int ao)
*******************************************************************************

FillPaint(int paint)
**************************************************************************

StrokePaint(int paint)
**************************************************************************

Save()
*******

Restore()
**********

Reset()
********

PathWinding(int dir)
*********************

ForceRender()
**************
Forces the current render queue to be processed which makes it possible to put any
Fast\* and Label drawing calls made before a ForceRender call under regular drawing
functions called after a ForceRender call.

This function might have a more than insignificant performance impact.
under regular 


