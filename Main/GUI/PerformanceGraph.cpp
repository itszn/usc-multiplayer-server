#include "stdafx.h"
#include "PerformanceGraph.hpp"

PerformanceGraph::PerformanceGraph()
{

}
void PerformanceGraph::PreRender()
{
}
void PerformanceGraph::Render()
{
	//rd.guiRenderer->RenderButton(rd.area, borderTexture, border);
	//Rect inner = border.Apply(rd.area);
	//rd.guiRenderer->RenderGraph(inner, graphTex, upperColor, lowerColor, colorBorder);
}

Vector2 PerformanceGraph::GetDesiredSize()
{
	return Vector2();
}
