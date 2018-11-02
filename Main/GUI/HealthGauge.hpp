#pragma once

class HealthGauge
{
public:
	HealthGauge();
	void Render(Mesh m, float deltaTime);
	Vector2 GetDesiredSize();

	// The fill rate of the gauge
	float rate = 0.5f;
	float colorBorder = 0.7f;
	Color upperColor = Colori(255, 102, 255);
	Color lowerColor = Colori(0, 204, 255);

	Material fillMaterial;
	Material baseMaterial;
	Texture frontTexture;
	Texture fillTexture;
	Texture backTexture;
	Texture maskTexture;
};