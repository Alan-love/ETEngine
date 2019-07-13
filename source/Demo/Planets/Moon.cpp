#include "stdafx.h"
#include "Moon.h"

#include <Engine/Graphics/TextureData.h>
#include <Engine/PlanetTech/Atmosphere.h>


Moon::Moon():Planet()
{
	m_Radius = 1737.1f;
	m_MaxHeight = 10.7f;
}
Moon::~Moon()
{
	delete m_pAtmosphere;
	m_pAtmosphere = nullptr;
}

void Moon::LoadPlanet()
{
	m_pDiffuse = CONTENT::Load<TextureData>("Resources/Textures/PlanetTextures/moon8k.jpg");
	m_pHeight = CONTENT::Load<TextureData>("Resources/Textures/PlanetTextures/MoonHeight.jpg");

	m_pAtmosphere = new Atmosphere("Resources/atmo_earth.json");
	m_pAtmosphere->Initialize();
	SetAtmosphere(m_pAtmosphere, 100);
}