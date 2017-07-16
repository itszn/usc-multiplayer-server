#pragma once
#include "ApplicationTickable.hpp"
#include "GameConfig.hpp"

class SettingsScreen : public IApplicationTickable
{
protected:
	SettingsScreen() = default;
public:
	virtual ~SettingsScreen() = default;
	static SettingsScreen* Create();
};

class ButtonBindingScreen : public IApplicationTickable
{
protected:
	ButtonBindingScreen() = default;
public:
	virtual ~ButtonBindingScreen() = default;
	static ButtonBindingScreen* Create(GameConfigKeys key);
};