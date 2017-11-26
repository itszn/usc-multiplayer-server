#pragma once
#include <GUI/GUIElement.hpp>

class HealthGauge : public GUIElementBase
{
public:
	HealthGauge();
	virtual void Render(GUIRenderData rd) override;
	virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

	// The fill rate of the gauge
	float rate = 0.5f;

	Material fillMaterial;
	Texture frontTexture;
	Texture fillTexture;
	Texture backTexture;
	Texture maskTexture;
};